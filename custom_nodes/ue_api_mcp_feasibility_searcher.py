from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Any

from core.BaseLLMNode import BaseLLMNode, GraphState
from core.config import load_runtime_config
from core.mcp_client import call_ue_api_semantic_search, safe_engine_port_filename
from core.phase2_validation import parse_phase2_json, validate_phase2_node_output, validate_phase2_output

UE_API_MCP_FEASIBILITY_SEARCHER_PROMPT = """SCHEMA: UEApiMCPFeasibilitySearcher
Adjudicate UE API MCP raw search results for each engine_port. Return JSON only.
"""

def _output_root(state: GraphState) -> Path:
    save_dir = getattr(state, "save_dir", "")
    if not save_dir:
        raise RuntimeError("state.save_dir is required for UE API MCP evidence emission")
    root = Path(save_dir).resolve()
    root.mkdir(parents=True, exist_ok=True)
    return root



def _query_symbol_hints(engine_port_id: str) -> str:
    pieces = [p for p in re.split(r"[._\-]+", engine_port_id) if p]
    if not pieces:
        return engine_port_id
    camel = "".join(p[:1].upper() + p[1:] for p in pieces)
    owner_method = pieces[0][:1].upper() + pieces[0][1:]
    if len(pieces) > 1:
        owner_method += "." + "".join(p[:1].upper() + p[1:] for p in pieces[1:])
    expanded = " ".join(pieces)
    return f"{engine_port_id}; exact symbol hints: {owner_method}, {camel}, {expanded}"

def _collect_queries(thin_flow: dict[str, Any]) -> list[dict[str, Any]]:
    by_port: dict[str, dict[str, Any]] = {}
    for flow in thin_flow.get("flows", []):
        flow_id = flow.get("flow_id", "")
        behavior_id = flow.get("source_behavior_id", "")
        for stage in flow.get("stages", []):
            contract = stage.get("contract", "")
            for port in stage.get("engine_ports", []):
                query = by_port.setdefault(port, {
                    "engine_port_id": port,
                    "flow_ids": [],
                    "behavior_ids": [],
                    "contracts": [],
                    "query": f"Unreal Engine PuerTS gameplay API for {_query_symbol_hints(port)}. Intent: {contract}",
                })
                if flow_id not in query["flow_ids"]:
                    query["flow_ids"].append(flow_id)
                if behavior_id not in query["behavior_ids"]:
                    query["behavior_ids"].append(behavior_id)
                if contract and contract not in query["contracts"]:
                    query["contracts"].append(contract)
    return list(by_port.values())

def _fixture_search(query: str, port: str) -> dict[str, Any]:
    symbol = {
        "input": "UE.EnhancedInputComponent.BindAction",
        "damage": "UE.GameplayStatics.ApplyDamage",
        "trace": "UE.KismetSystemLibrary.LineTraceSingle",
        "overlap": "UE.KismetSystemLibrary.SphereOverlapActors",
    }
    picked = "UE.KismetSystemLibrary.LineTraceSingle"
    lower = port.lower()
    for key, value in symbol.items():
        if key in lower:
            picked = value
            break
    return {
        "tool": "ue_api_semantic_search",
        "query": query,
        "limit": 5,
        "fixture": True,
        "payload": {
            "results": [
                {
                    "symbol": picked,
                    "entryType": "function",
                    "signature": picked + "(...)",
                    "summary": "scripted smoke UE API evidence",
                    "confidence": 0.99,
                }
            ]
        },
    }



def _compact_payload(raw_result: dict[str, Any]) -> dict[str, Any]:
    payload = raw_result.get("payload") if isinstance(raw_result, dict) else None
    if not isinstance(payload, dict):
        return {"status": "unknown", "candidate_symbols": []}
    candidates = []
    for item in payload.get("results", [])[:5]:
        if not isinstance(item, dict):
            continue
        candidates.append({
            "symbol": item.get("symbol", ""),
            "entryType": item.get("entryType", ""),
            "kind": item.get("kind", ""),
            "signature": item.get("signature", ""),
            "summary": item.get("summary", ""),
            "confidence": item.get("confidence", None),
            "score": item.get("score", None),
            "source": item.get("source", ""),
        })
    return {
        "status": payload.get("status", "ok"),
        "mode": payload.get("mode", "semantic"),
        "count": payload.get("count", len(candidates)),
        "top_confidence": payload.get("confidence", {}).get("topConfidence") if isinstance(payload.get("confidence"), dict) else None,
        "candidate_symbols": candidates,
    }

def _use_fixture(node: BaseLLMNode) -> bool:
    return node.model.__class__.__name__ == "ScriptedSmokeChatModel"

def build_mcp_context(node: BaseLLMNode, state: GraphState, full_input: str) -> None:
    thin_text = state.llm_outputs.get("ThinGameplayFlowPlanner", "")
    if not thin_text.strip():
        raise RuntimeError("UEApiMCPFeasibilitySearcher requires ThinGameplayFlowPlanner output")
    thin = parse_phase2_json("ThinGameplayFlowPlanner", thin_text)
    queries = _collect_queries(thin)
    if not queries:
        raise RuntimeError("UEApiMCPFeasibilitySearcher found no engine_ports to query")
    root = _output_root(state)
    raw_dir = root / "flow" / "04-ue-api-mcp" / "raw"
    raw_dir.mkdir(parents=True, exist_ok=True)
    raw_records = []
    runtime_config = load_runtime_config()
    for query in queries:
        port = query["engine_port_id"]
        safe = safe_engine_port_filename(port)
        raw_path = f"flow/04-ue-api-mcp/raw/{safe}.raw.json"
        adjudication_path = f"flow/04-ue-api-mcp/adjudication/{safe}.json"
        try:
            raw_result = _fixture_search(query["query"], port) if _use_fixture(node) else call_ue_api_semantic_search(query["query"], limit=5, runtime_config=runtime_config)
            raw_error = None
        except Exception as exc:
            raw_result = {"error": str(exc), "tool": "ue_api_semantic_search", "query": query["query"], "engine_port_id": port}
            raw_error = str(exc)
        raw_record = {**query, "raw_path": raw_path, "adjudication_path": adjudication_path, "raw_result": raw_result}
        (root / raw_path).write_text(json.dumps(raw_record, ensure_ascii=False, indent=2), encoding="utf-8")
        raw_records.append({**query, "raw_path": raw_path, "adjudication_path": adjudication_path, "mcp_summary": _compact_payload(raw_result)})
        if raw_error:
            raise RuntimeError(f"UE API MCP query failed for {port}: {raw_error}")
    node.full_input = (
        "ThinGameplayFlowPlanner JSON:\n" + thin_text
        + "\n\nMCP raw search records were written under flow/04-ue-api-mcp/raw. "
        + "Adjudicate each query. Copy engine_port_id, query, flow_ids, behavior_ids, raw_path, and adjudication_path exactly.\n\n"
        + json.dumps({"raw_records": raw_records}, ensure_ascii=False, indent=2)
    )

def write_adjudication_files(state: GraphState, output: str) -> None:
    data = parse_phase2_json("UEApiMCPFeasibilitySearcher", validate_phase2_output("UEApiMCPFeasibilitySearcher", output))
    root = _output_root(state)
    for query in data.get("queries", []):
        target = root / query["adjudication_path"]
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(json.dumps(query, ensure_ascii=False, indent=2), encoding="utf-8")
    summary = root / "flow" / "04-ue-api-mcp" / "summary.json"
    summary.parent.mkdir(parents=True, exist_ok=True)
    summary.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"[SUCCESS] UE API MCP adjudications saved under: {summary.parent}")

def create_ue_api_mcp_feasibility_searcher() -> BaseLLMNode:
    return BaseLLMNode(
        name="UEApiMCPFeasibilitySearcher",
        prompt=UE_API_MCP_FEASIBILITY_SEARCHER_PROMPT,
        pre_action=build_mcp_context,
        output_validator=validate_phase2_node_output,
        post_action=write_adjudication_files,
        enable_feedback=False,
    )
