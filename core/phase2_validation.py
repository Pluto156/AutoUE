from __future__ import annotations

import json
import re
from pathlib import PurePosixPath, PureWindowsPath
from typing import Any, Iterable, Mapping

PHASE2_NODE_ORDER = [
    "SceneAndGameplaySplitter",
    "EntityAbilityBehaviorPlanner",
    "ThinGameplayFlowPlanner",
    "UEApiMCPFeasibilitySearcher",
    "PuerTSRuntimeMappingPlanner",
    "TypeScriptScriptAnalyzer",
    "TypeScriptInteractiveObjectGenerator",
    "TypeScriptCodeGenerator",
    "EvaluateInstructionGenerator",
]
RUNTIME_MAPPING_PATH = "flow/05-puerts-runtime-mapping.json"
BANNED_FLOW_MARKERS = ("RetrieveModel", "PCGGraphComposer", "PCGPlanner", "LLMHttpServer")
BANNED_NATIVE_MARKERS = (
    "AInteractiveObjectBase", "CustomModules", "header_code", "source_code", "cpp_code",
    "ModuleCodeGenerator", "InteractiveObjectCodeGenerator",
)
BANNED_OUTPUT_MARKERS = BANNED_FLOW_MARKERS + BANNED_NATIVE_MARKERS
CXX_FILE_RE = re.compile(r"(?i)\.(?:h|cpp)\b")
TS_IDENT_RE = re.compile(r"^[A-Za-z_$][A-Za-z0-9_$]*$")
ALLOWED_TEMPLATES = {
    "TypeScriptInteractiveObjectGenerator": {"interactive_object"},
    "TypeScriptCodeGenerator": {
        "ability_module",
        "runtime_bootstrap",
        "aid_runtime_orchestrator",
        "aid_character_adapter",
        "aid_gamemode_adapter",
        "aid_camera_setup",
        "scene_manifest_helper",
    },
}
ALLOWED_STAGES = {"Input", "Ability/Action", "SpatialQuery/HitQuery", "Damage/Resource", "Event/Result", "Feedback/HUD", "Cleanup", "Custom"}
ALLOWED_VERDICTS = {"hit", "miss"}
ALLOWED_HIT_TYPES = {"direct_hit", "indirect_hit", "none"}
ALLOWED_CARRIERS = {"template_rendered_ts", "existing_runtime_adapter", "generated_aidev_adapter", "generated_runtime_orchestrator", "blocked"}

class Phase2ValidationError(ValueError):
    pass

def strip_json_fence(text: str) -> str:
    s = text.strip()
    m = re.fullmatch(r"```(?:json)?\s*(.*?)\s*```", s, flags=re.I | re.S)
    return m.group(1).strip() if m else s

def parse_phase2_json(node_name: str, text: str) -> Any:
    try:
        return json.loads(strip_json_fence(text))
    except Exception as exc:
        raise Phase2ValidationError(f"{node_name}: output must be valid JSON: {exc}") from exc

def canonical_json(data: Any) -> str:
    return json.dumps(data, ensure_ascii=False, indent=2)

def walk_strings(value: Any) -> Iterable[str]:
    if isinstance(value, str):
        yield value
    elif isinstance(value, dict):
        for k, v in value.items():
            yield str(k)
            yield from walk_strings(v)
    elif isinstance(value, list):
        for v in value:
            yield from walk_strings(v)

def walk_keys(value: Any) -> Iterable[str]:
    if isinstance(value, dict):
        for k, v in value.items():
            yield str(k)
            yield from walk_keys(v)
    elif isinstance(value, list):
        for v in value:
            yield from walk_keys(v)

def validate_common_phase2_contract(node: str, data: Any) -> None:
    if not isinstance(data, dict):
        raise Phase2ValidationError(f"{node}: output JSON must be an object")
    for text in walk_strings(data):
        if CXX_FILE_RE.search(text):
            raise Phase2ValidationError(f"{node}: native code filename markers are forbidden: {text}")
        for marker in BANNED_OUTPUT_MARKERS:
            if marker in text:
                raise Phase2ValidationError(f"{node}: banned Phase2 marker appears in output: {marker}")

def require_list(node: str, obj: dict[str, Any], key: str, *, non_empty: bool = False) -> list[Any]:
    value = obj.get(key)
    if not isinstance(value, list):
        raise Phase2ValidationError(f"{node}: expected list at {key}")
    if non_empty and not value:
        raise Phase2ValidationError(f"{node}: expected non-empty list at {key}")
    return value

def require_dict(node: str, obj: dict[str, Any], key: str) -> dict[str, Any]:
    value = obj.get(key)
    if not isinstance(value, dict):
        raise Phase2ValidationError(f"{node}: expected object at {key}")
    return value

def require_string(node: str, obj: dict[str, Any], key: str, *, non_empty: bool = False) -> str:
    value = obj.get(key)
    if not isinstance(value, str):
        raise Phase2ValidationError(f"{node}: expected string at {key}")
    if non_empty and not value.strip():
        raise Phase2ValidationError(f"{node}: expected non-empty string at {key}")
    return value

def require_identifier(node: str, obj: dict[str, Any], key: str) -> str:
    value = require_string(node, obj, key, non_empty=True)
    if not TS_IDENT_RE.fullmatch(value):
        raise Phase2ValidationError(f"{node}: {key} must be a TypeScript identifier: {value}")
    return value

def is_safe_relative_path(value: str) -> bool:
    if not value or "\x00" in value:
        return False
    posix = PurePosixPath(value.replace("\\", "/"))
    windows = PureWindowsPath(value)
    if posix.is_absolute() or windows.is_absolute() or windows.drive or windows.root:
        return False
    return ".." not in (set(posix.parts) | set(windows.parts))

def validate_rel_path(node: str, path: str, *, label: str, suffixes: tuple[str, ...]) -> None:
    if not is_safe_relative_path(path):
        raise Phase2ValidationError(f"{node}: {label} must be relative and stay inside output root: {path}")
    if not path.lower().endswith(suffixes):
        raise Phase2ValidationError(f"{node}: {label} must end with {suffixes}: {path}")

def validate_ts_path(node: str, path: str, *, label: str) -> None:
    validate_rel_path(node, path, label=label, suffixes=(".ts", ".tsx"))

def validate_json_path(node: str, path: str, *, label: str) -> None:
    validate_rel_path(node, path, label=label, suffixes=(".json",))

def validate_scene_and_gameplay_splitter(node: str, data: dict[str, Any]) -> None:
    require_string(node, data, "scene_description", non_empty=True)
    require_string(node, data, "gameplay_description", non_empty=True)

def validate_entity_ability_behavior_planner(node: str, data: dict[str, Any]) -> None:
    forbidden = {"files", "path", "file_path", "target_ts_file", "implementation_slots", "content", "ts_files", "template_inputs", "engine_ports", "flow_id", "runtime_mapping_path"}
    bad = sorted(forbidden.intersection(walk_keys(data)))
    if bad:
        raise Phase2ValidationError(f"{node}: planner is definition-only and must not decide files, templates, engine ports, flows, or implementation slots: {bad}")
    for ei, entity in enumerate(require_list(node, data, "entities", non_empty=True)):
        if not isinstance(entity, dict):
            raise Phase2ValidationError(f"{node}: entities[{ei}] must be object")
        for key in ("entity_id", "display_name", "summary"):
            require_string(node, entity, key, non_empty=True)
        for ai, ability in enumerate(require_list(node, entity, "abilities", non_empty=True)):
            if not isinstance(ability, dict):
                raise Phase2ValidationError(f"{node}: abilities[{ai}] must be object")
            for key in ("ability_id", "display_name", "summary"):
                require_string(node, ability, key, non_empty=True)
            for bi, behavior in enumerate(require_list(node, ability, "behaviors", non_empty=True)):
                if not isinstance(behavior, dict):
                    raise Phase2ValidationError(f"{node}: behaviors[{bi}] must be object")
                for key in ("behavior_id", "display_name", "trigger", "execution", "result"):
                    require_string(node, behavior, key, non_empty=True)
                if "source_refs" in behavior:
                    require_list(node, behavior, "source_refs")

def validate_thin_gameplay_flow_planner(node: str, data: dict[str, Any]) -> None:
    for fi, flow in enumerate(require_list(node, data, "flows", non_empty=True)):
        if not isinstance(flow, dict):
            raise Phase2ValidationError(f"{node}: flows[{fi}] must be object")
        for key in ("flow_id", "source_behavior_id", "entity_id", "ability_id"):
            require_string(node, flow, key, non_empty=True)
        ports = set()
        for si, stage in enumerate(require_list(node, flow, "stages", non_empty=True)):
            if not isinstance(stage, dict):
                raise Phase2ValidationError(f"{node}: flows[{fi}].stages[{si}] must be object")
            stage_name = require_string(node, stage, "stage", non_empty=True)
            if stage_name not in ALLOWED_STAGES:
                raise Phase2ValidationError(f"{node}: unknown flow stage: {stage_name}")
            require_string(node, stage, "contract", non_empty=True)
            if "inputs" in stage:
                require_list(node, stage, "inputs")
            if "outputs" in stage:
                require_list(node, stage, "outputs")
            for port in require_list(node, stage, "engine_ports"):
                if not isinstance(port, str) or not port.strip():
                    raise Phase2ValidationError(f"{node}: engine_ports must contain non-empty strings")
                ports.add(port)
        if not ports:
            raise Phase2ValidationError(f"{node}: every flow must declare at least one engine_port")
        if "verification" in flow:
            require_list(node, flow, "verification")

def validate_ue_api_mcp_feasibility_searcher(node: str, data: dict[str, Any]) -> None:
    for qi, query in enumerate(require_list(node, data, "queries", non_empty=True)):
        if not isinstance(query, dict):
            raise Phase2ValidationError(f"{node}: queries[{qi}] must be object")
        for key in ("engine_port_id", "query", "notes"):
            require_string(node, query, key, non_empty=True)
        validate_json_path(node, require_string(node, query, "raw_path", non_empty=True), label=f"queries[{qi}].raw_path")
        validate_json_path(node, require_string(node, query, "adjudication_path", non_empty=True), label=f"queries[{qi}].adjudication_path")
        verdict = require_string(node, query, "verdict", non_empty=True)
        hit_type = require_string(node, query, "hit_type", non_empty=True)
        if verdict not in ALLOWED_VERDICTS:
            raise Phase2ValidationError(f"{node}: invalid verdict: {verdict}")
        if hit_type not in ALLOWED_HIT_TYPES:
            raise Phase2ValidationError(f"{node}: invalid hit_type: {hit_type}")
        if verdict == "hit" and hit_type == "none":
            raise Phase2ValidationError(f"{node}: hit verdict must use direct_hit or indirect_hit")
        if verdict == "miss" and hit_type != "none":
            raise Phase2ValidationError(f"{node}: miss verdict must use hit_type=none")
        require_list(node, query, "flow_ids", non_empty=True)
        require_list(node, query, "behavior_ids", non_empty=True)
        require_list(node, query, "evidence_symbols")
    require_dict(node, data, "summary")

def validate_puerts_runtime_mapping_planner(node: str, data: dict[str, Any]) -> None:
    runtime_mapping_path = require_string(node, data, "runtime_mapping_path", non_empty=True)
    validate_json_path(node, runtime_mapping_path, label="runtime_mapping_path")
    for mi, mapping in enumerate(require_list(node, data, "mappings", non_empty=True)):
        if not isinstance(mapping, dict):
            raise Phase2ValidationError(f"{node}: mappings[{mi}] must be object")
        for key in ("entity_id", "ability_id", "behavior_id", "flow_id", "runtime_owner", "selected_runtime_owner", "ability_binding"):
            require_string(node, mapping, key, non_empty=True)
        validate_ts_path(node, mapping["runtime_owner"], label=f"mappings[{mi}].runtime_owner")
        carrier = require_string(node, mapping, "implementation_carrier", non_empty=True)
        if carrier not in ALLOWED_CARRIERS:
            raise Phase2ValidationError(f"{node}: invalid implementation_carrier: {carrier}")
        require_list(node, mapping, "thin_contracts", non_empty=True)
        require_list(node, mapping, "verification_evidence", non_empty=True)
        require_list(node, mapping, "existing_framework_candidates")
        require_string(node, mapping, "why_not_existing_framework", non_empty=True)
        require_string(node, mapping, "temporary_or_canonical", non_empty=True)
        require_string(node, mapping, "migration_path", non_empty=True)
        for pi, port in enumerate(require_list(node, mapping, "engine_port_mappings", non_empty=True)):
            if not isinstance(port, dict):
                raise Phase2ValidationError(f"{node}: engine_port_mappings[{pi}] must be object")
            require_string(node, port, "engine_port_id", non_empty=True)
            validate_json_path(node, require_string(node, port, "adjudication_path", non_empty=True), label=f"engine_port_mappings[{pi}].adjudication_path")
            require_string(node, port, "adapter_or_helper", non_empty=True)
            if require_string(node, port, "verdict", non_empty=True) not in ALLOWED_VERDICTS:
                raise Phase2ValidationError(f"{node}: invalid engine port verdict")
            require_list(node, port, "evidence_symbols")
    require_list(node, data, "blocked_mappings")

def validate_typescript_script_analyzer(node: str, data: dict[str, Any]) -> None:
    for key in ("typescript_sources", "implementation_slots", "missing_slots"):
        require_list(node, data, key)
    for si, source in enumerate(data.get("typescript_sources", [])):
        if not isinstance(source, dict):
            raise Phase2ValidationError(f"{node}: typescript_sources[{si}] must be object")
        validate_ts_path(node, require_string(node, source, "path", non_empty=True), label=f"typescript_sources[{si}].path")
    targets = {}
    for si, slot in enumerate(data.get("implementation_slots", [])):
        if not isinstance(slot, dict):
            raise Phase2ValidationError(f"{node}: implementation_slots[{si}] must be object")
        behavior_id = require_string(node, slot, "behavior_id", non_empty=True)
        require_string(node, slot, "entity_id", non_empty=True)
        require_string(node, slot, "flow_id", non_empty=True)
        validate_json_path(node, require_string(node, slot, "runtime_mapping_path", non_empty=True), label=f"implementation_slots[{si}].runtime_mapping_path")
        target = require_string(node, slot, "target_ts_file", non_empty=True)
        validate_ts_path(node, target, label=f"implementation_slots[{si}].target_ts_file")
        previous = targets.get(target)
        if previous and previous != behavior_id:
            raise Phase2ValidationError(f"{node}: one Phase2 template target file must not collapse multiple behaviors: {target} used by {previous} and {behavior_id}")
        targets[target] = behavior_id

def _reject_raw_files(node: str, data: dict[str, Any]) -> None:
    if {"files", "content"}.intersection(walk_keys(data)):
        raise Phase2ValidationError(f"{node}: raw files/content are forbidden; use template_inputs rendered by Python templates")

def _template_inputs(node: str, data: dict[str, Any]) -> set[str]:
    _reject_raw_files(node, data)
    paths = set()
    for ii, item in enumerate(require_list(node, data, "template_inputs", non_empty=True)):
        if not isinstance(item, dict):
            raise Phase2ValidationError(f"{node}: template_inputs[{ii}] must be object")
        template = require_string(node, item, "template", non_empty=True)
        if template not in ALLOWED_TEMPLATES[node]:
            raise Phase2ValidationError(f"{node}: invalid template {template}")
        path = require_string(node, item, "path", non_empty=True)
        validate_ts_path(node, path, label=f"template_inputs[{ii}].path")
        if path in paths:
            raise Phase2ValidationError(f"{node}: duplicate template output path would overwrite a rendered file: {path}")
        paths.add(path)
        for key in ("entity_id", "behavior_id", "flow_id"):
            require_string(node, item, key, non_empty=True)
        validate_json_path(node, require_string(node, item, "runtime_mapping_path", non_empty=True), label=f"template_inputs[{ii}].runtime_mapping_path")
        require_identifier(node, item, "export_name")
        require_identifier(node, item, "interface_name")
        for key in ("action_label", "target_label", "result_label"):
            require_string(node, item, key, non_empty=True)
    return paths

def _traces(node: str, data: dict[str, Any], paths: set[str]) -> None:
    for ti, trace in enumerate(require_list(node, data, "behavior_traces", non_empty=True)):
        if not isinstance(trace, dict):
            raise Phase2ValidationError(f"{node}: behavior_traces[{ti}] must be object")
        for key in ("entity_id", "behavior_id", "flow_id"):
            require_string(node, trace, key, non_empty=True)
        validate_json_path(node, require_string(node, trace, "runtime_mapping_path", non_empty=True), label=f"behavior_traces[{ti}].runtime_mapping_path")
        file_path = require_string(node, trace, "file_path", non_empty=True)
        validate_ts_path(node, file_path, label=f"behavior_traces[{ti}].file_path")
        if file_path not in paths:
            raise Phase2ValidationError(f"{node}: behavior trace references non-rendered template path: {file_path}")

def validate_typescript_interactive_object_generator(node: str, data: dict[str, Any]) -> None:
    paths = _template_inputs(node, data)
    _traces(node, data, paths)
    require_list(node, data, "validation_notes")

def validate_typescript_code_generator(node: str, data: dict[str, Any]) -> None:
    paths = _template_inputs(node, data)
    _traces(node, data, paths)
    for pi, path in enumerate(require_list(node, data, "consumed_interactive_files", non_empty=True)):
        if not isinstance(path, str):
            raise Phase2ValidationError(f"{node}: consumed_interactive_files[{pi}] must be string")
        validate_ts_path(node, path, label=f"consumed_interactive_files[{pi}]")
    require_list(node, data, "validation_notes")

def _eval_trace(node: str, trace: Any, label: str) -> dict[str, Any]:
    if not isinstance(trace, dict):
        raise Phase2ValidationError(f"{node}: {label} must be object")
    for key in ("entity_id", "ability_id", "behavior_id", "flow_id"):
        require_string(node, trace, key, non_empty=True)
    validate_json_path(node, require_string(node, trace, "runtime_mapping_path", non_empty=True), label=f"{label}.runtime_mapping_path")
    for key in ("engine_port_ids", "adjudication_paths", "ts_files"):
        require_list(node, trace, key, non_empty=True)
    for pi, path in enumerate(trace["adjudication_paths"]):
        if not isinstance(path, str):
            raise Phase2ValidationError(f"{node}: {label}.adjudication_paths[{pi}] must be string")
        validate_json_path(node, path, label=f"{label}.adjudication_paths[{pi}]")
    for pi, path in enumerate(trace["ts_files"]):
        if not isinstance(path, str):
            raise Phase2ValidationError(f"{node}: {label}.ts_files[{pi}] must be string")
        validate_ts_path(node, path, label=f"{label}.ts_files[{pi}]")
    for pi, port in enumerate(trace["engine_port_ids"]):
        if not isinstance(port, str) or not port.strip():
            raise Phase2ValidationError(f"{node}: {label}.engine_port_ids[{pi}] must be non-empty string")
    return trace

def validate_evaluate_instruction_generator(node: str, data: dict[str, Any]) -> None:
    for ii, instruction in enumerate(require_list(node, data, "evaluation_instructions", non_empty=True)):
        if not isinstance(instruction, dict):
            raise Phase2ValidationError(f"{node}: evaluation_instructions[{ii}] must be object")
        if not isinstance(instruction.get("step_id"), int):
            raise Phase2ValidationError(f"{node}: evaluation_instructions[{ii}].step_id must be integer")
        for key in ("action", "target", "description", "driver", "executor_action"):
            require_string(node, instruction, key, non_empty=True)
        require_list(node, instruction, "expected", non_empty=True)
        _eval_trace(node, instruction.get("trace"), f"evaluation_instructions[{ii}].trace")
    for ci, coverage in enumerate(require_list(node, data, "coverage", non_empty=True)):
        _eval_trace(node, coverage, f"coverage[{ci}]")

NODE_VALIDATORS = {
    "SceneAndGameplaySplitter": validate_scene_and_gameplay_splitter,
    "EntityAbilityBehaviorPlanner": validate_entity_ability_behavior_planner,
    "ThinGameplayFlowPlanner": validate_thin_gameplay_flow_planner,
    "UEApiMCPFeasibilitySearcher": validate_ue_api_mcp_feasibility_searcher,
    "PuerTSRuntimeMappingPlanner": validate_puerts_runtime_mapping_planner,
    "TypeScriptScriptAnalyzer": validate_typescript_script_analyzer,
    "TypeScriptInteractiveObjectGenerator": validate_typescript_interactive_object_generator,
    "TypeScriptCodeGenerator": validate_typescript_code_generator,
    "EvaluateInstructionGenerator": validate_evaluate_instruction_generator,
}

def validate_phase2_output(node_name: str, text: str) -> str:
    if node_name not in NODE_VALIDATORS:
        raise Phase2ValidationError(f"Unknown Phase2 node: {node_name}")
    data = parse_phase2_json(node_name, text)
    validate_common_phase2_contract(node_name, data)
    NODE_VALIDATORS[node_name](node_name, data)
    return canonical_json(data)

def parse_validated_phase2_output(node_name: str, text: str) -> dict[str, Any]:
    data = parse_phase2_json(node_name, validate_phase2_output(node_name, text))
    if not isinstance(data, dict):
        raise Phase2ValidationError(f"{node_name}: output JSON must be object")
    return data

def collect_behavior_index(planner: dict[str, Any]) -> dict[str, dict[str, str]]:
    out = {}
    for entity in planner.get("entities", []):
        for ability in entity.get("abilities", []):
            for behavior in ability.get("behaviors", []):
                behavior_id = behavior.get("behavior_id", "")
                if behavior_id:
                    out[behavior_id] = {"entity_id": entity.get("entity_id", ""), "ability_id": ability.get("ability_id", ""), "behavior_id": behavior_id}
    return out

def _known(node: str, index: Mapping[str, Mapping[str, str]], behavior_id: str, *, entity_id: str | None = None, ability_id: str | None = None) -> None:
    if behavior_id not in index:
        raise Phase2ValidationError(f"{node}: references unknown behavior_id: {behavior_id}")
    expected = index[behavior_id]
    if entity_id is not None and entity_id != expected.get("entity_id"):
        raise Phase2ValidationError(f"{node}: behavior {behavior_id} belongs to entity {expected.get('entity_id')}, got {entity_id}")
    if ability_id is not None and ability_id != expected.get("ability_id"):
        raise Phase2ValidationError(f"{node}: behavior {behavior_id} belongs to ability {expected.get('ability_id')}, got {ability_id}")

def _generated(data: Mapping[str, Any]) -> set[str]:
    return {item.get("path", "") for item in data.get("template_inputs", []) if isinstance(item, dict)}

def _flow_indexes(thin: Mapping[str, Any], behavior_index: Mapping[str, Mapping[str, str]], coverage: dict[str, set[str]]):
    flow_by_behavior, flow_by_id, ports_by_flow, behavior_by_port = {}, {}, {}, {}
    for flow in thin.get("flows", []):
        behavior_id = flow.get("source_behavior_id", "")
        _known("ThinGameplayFlowPlanner", behavior_index, behavior_id, entity_id=flow.get("entity_id", ""), ability_id=flow.get("ability_id", ""))
        flow_id = flow.get("flow_id", "")
        if behavior_id in flow_by_behavior:
            raise Phase2ValidationError(f"ThinGameplayFlowPlanner: duplicate flow for behavior_id: {behavior_id}")
        flow_by_behavior[behavior_id] = flow
        flow_by_id[flow_id] = flow
        coverage.setdefault(behavior_id, set()).add("ThinGameplayFlowPlanner")
        ports = set()
        for stage in flow.get("stages", []):
            for port in stage.get("engine_ports", []):
                ports.add(port)
                behavior_by_port.setdefault(port, set()).add(behavior_id)
        ports_by_flow[flow_id] = ports
    missing = sorted(set(behavior_index) - set(flow_by_behavior))
    if missing:
        raise Phase2ValidationError(f"ThinGameplayFlowPlanner: missing flow for behavior_id: {missing}")
    return flow_by_behavior, flow_by_id, ports_by_flow, behavior_by_port

def _cross(data: Mapping[str, dict[str, Any]], require_all: bool) -> dict[str, Any]:
    behavior_index = collect_behavior_index(data.get("EntityAbilityBehaviorPlanner", {}))
    coverage = {behavior_id: set() for behavior_id in behavior_index}
    flow_by_behavior, flow_by_id, ports_by_flow, behavior_by_port = {}, {}, {}, {}
    if data.get("ThinGameplayFlowPlanner") and behavior_index:
        flow_by_behavior, flow_by_id, ports_by_flow, behavior_by_port = _flow_indexes(data["ThinGameplayFlowPlanner"], behavior_index, coverage)

    mcp_by_port, adjudication_by_path = {}, {}
    if data.get("UEApiMCPFeasibilitySearcher"):
        for query in data["UEApiMCPFeasibilitySearcher"].get("queries", []):
            port = query.get("engine_port_id", "")
            if port in mcp_by_port:
                raise Phase2ValidationError(f"UEApiMCPFeasibilitySearcher: duplicate engine_port_id query: {port}")
            if query.get("verdict") != "hit":
                raise Phase2ValidationError(f"UEApiMCPFeasibilitySearcher: engine_port_id must be hit for Phase2 done: {port}")
            mcp_by_port[port] = query
            adjudication_by_path[query.get("adjudication_path", "")] = query
            for behavior_id in query.get("behavior_ids", []):
                _known("UEApiMCPFeasibilitySearcher", behavior_index, behavior_id)
                coverage.setdefault(behavior_id, set()).add("UEApiMCPFeasibilitySearcher")
            expected = behavior_by_port.get(port, set())
            if expected and not expected.issubset(set(query.get("behavior_ids", []))):
                raise Phase2ValidationError(f"UEApiMCPFeasibilitySearcher: query {port} missing behavior references: {sorted(expected)}")
        if ports_by_flow:
            expected_ports = set().union(*ports_by_flow.values()) if ports_by_flow else set()
            missing = sorted(expected_ports - set(mcp_by_port))
            if missing:
                raise Phase2ValidationError(f"UEApiMCPFeasibilitySearcher: missing MCP query for engine_port_id: {missing}")

    mapping_by_behavior, runtime_mapping_path = {}, ""
    if data.get("PuerTSRuntimeMappingPlanner"):
        mapping_node = data["PuerTSRuntimeMappingPlanner"]
        runtime_mapping_path = mapping_node.get("runtime_mapping_path", "")
        if runtime_mapping_path != RUNTIME_MAPPING_PATH:
            raise Phase2ValidationError(f"PuerTSRuntimeMappingPlanner: runtime_mapping_path must be {RUNTIME_MAPPING_PATH}, got {runtime_mapping_path}")
        if mapping_node.get("blocked_mappings"):
            raise Phase2ValidationError("PuerTSRuntimeMappingPlanner: blocked_mappings must be empty for Phase2 done")
        for mapping in mapping_node.get("mappings", []):
            behavior_id = mapping.get("behavior_id", "")
            _known("PuerTSRuntimeMappingPlanner", behavior_index, behavior_id, entity_id=mapping.get("entity_id", ""), ability_id=mapping.get("ability_id", ""))
            if mapping.get("implementation_carrier") == "blocked":
                raise Phase2ValidationError(f"PuerTSRuntimeMappingPlanner: mapping is blocked for behavior_id: {behavior_id}")
            if behavior_id in mapping_by_behavior:
                raise Phase2ValidationError(f"PuerTSRuntimeMappingPlanner: duplicate mapping for behavior_id: {behavior_id}")
            expected_flow = flow_by_behavior.get(behavior_id, {}).get("flow_id")
            if expected_flow and mapping.get("flow_id") != expected_flow:
                raise Phase2ValidationError(f"PuerTSRuntimeMappingPlanner: behavior {behavior_id} must map to its thin flow")
            mapping_by_behavior[behavior_id] = mapping
            coverage.setdefault(behavior_id, set()).add("PuerTSRuntimeMappingPlanner")
            mapped_ports = {p.get("engine_port_id", "") for p in mapping.get("engine_port_mappings", [])}
            missing_ports = sorted(ports_by_flow.get(mapping.get("flow_id", ""), set()) - mapped_ports)
            if missing_ports:
                raise Phase2ValidationError(f"PuerTSRuntimeMappingPlanner: behavior {behavior_id} missing runtime mappings for engine ports: {missing_ports}")
            for port in mapping.get("engine_port_mappings", []):
                if mcp_by_port and port.get("engine_port_id") not in mcp_by_port:
                    raise Phase2ValidationError(f"PuerTSRuntimeMappingPlanner: references engine_port without MCP query: {port.get('engine_port_id', '')}")
                if adjudication_by_path and port.get("adjudication_path") not in adjudication_by_path:
                    raise Phase2ValidationError(f"PuerTSRuntimeMappingPlanner: references unknown adjudication_path: {port.get('adjudication_path', '')}")
                if port.get("verdict") != "hit":
                    raise Phase2ValidationError(f"PuerTSRuntimeMappingPlanner: engine_port_mappings must be hit: {behavior_id}")
        missing = sorted(set(behavior_index) - set(mapping_by_behavior))
        if missing:
            raise Phase2ValidationError(f"PuerTSRuntimeMappingPlanner: missing mapping for behavior_id: {missing}")

    analyzer_targets, interactive_files, codegen_files = set(), set(), set()
    if data.get("TypeScriptScriptAnalyzer"):
        analyzer = data["TypeScriptScriptAnalyzer"]
        if analyzer.get("missing_slots"):
            raise Phase2ValidationError("TypeScriptScriptAnalyzer: missing_slots must be empty for Phase2 done")
        for slot in analyzer.get("implementation_slots", []):
            behavior_id = slot.get("behavior_id", "")
            _known("TypeScriptScriptAnalyzer", behavior_index, behavior_id, entity_id=slot.get("entity_id", ""))
            mapped = mapping_by_behavior.get(behavior_id)
            if mapped:
                if slot.get("flow_id") != mapped.get("flow_id"):
                    raise Phase2ValidationError(f"TypeScriptScriptAnalyzer: slot flow_id must come from runtime mapping: {behavior_id}")
                if slot.get("runtime_mapping_path") != runtime_mapping_path:
                    raise Phase2ValidationError(f"TypeScriptScriptAnalyzer: slot runtime_mapping_path must be {runtime_mapping_path}")
                if slot.get("target_ts_file") != mapped.get("runtime_owner"):
                    raise Phase2ValidationError(f"TypeScriptScriptAnalyzer: target_ts_file must equal runtime_owner for behavior_id: {behavior_id}")
            analyzer_targets.add(slot.get("target_ts_file", ""))
            coverage.setdefault(behavior_id, set()).add("TypeScriptScriptAnalyzer")

    if data.get("TypeScriptInteractiveObjectGenerator"):
        interactive = data["TypeScriptInteractiveObjectGenerator"]
        interactive_files = _generated(interactive)
        for trace in interactive.get("behavior_traces", []):
            behavior_id = trace.get("behavior_id", "")
            _known("TypeScriptInteractiveObjectGenerator", behavior_index, behavior_id, entity_id=trace.get("entity_id", ""))
            mapped = mapping_by_behavior.get(behavior_id)
            if mapped and (trace.get("flow_id") != mapped.get("flow_id") or trace.get("runtime_mapping_path") != runtime_mapping_path):
                raise Phase2ValidationError(f"TypeScriptInteractiveObjectGenerator: trace must come from runtime mapping: {behavior_id}")
            coverage.setdefault(behavior_id, set()).add("TypeScriptInteractiveObjectGenerator")

    if data.get("TypeScriptCodeGenerator"):
        code = data["TypeScriptCodeGenerator"]
        codegen_files = _generated(code)
        consumed = set(code.get("consumed_interactive_files", []))
        if interactive_files and not consumed.issubset(interactive_files):
            raise Phase2ValidationError("TypeScriptCodeGenerator: consumed_interactive_files must reference TypeScriptInteractiveObjectGenerator files")
        missing_targets = sorted(analyzer_targets - codegen_files)
        if analyzer_targets and missing_targets:
            raise Phase2ValidationError(f"TypeScriptCodeGenerator: must render every analyzer implementation target: {missing_targets}")
        for trace in code.get("behavior_traces", []):
            behavior_id = trace.get("behavior_id", "")
            _known("TypeScriptCodeGenerator", behavior_index, behavior_id, entity_id=trace.get("entity_id", ""))
            mapped = mapping_by_behavior.get(behavior_id)
            if mapped and (trace.get("flow_id") != mapped.get("flow_id") or trace.get("runtime_mapping_path") != runtime_mapping_path):
                raise Phase2ValidationError(f"TypeScriptCodeGenerator: trace must come from runtime mapping: {behavior_id}")
            coverage.setdefault(behavior_id, set()).add("TypeScriptCodeGenerator")

    if data.get("EvaluateInstructionGenerator"):
        rendered = interactive_files | codegen_files
        for item in list(data["EvaluateInstructionGenerator"].get("evaluation_instructions", [])) + list(data["EvaluateInstructionGenerator"].get("coverage", [])):
            trace = item.get("trace") if isinstance(item, dict) and "trace" in item else item
            if not isinstance(trace, dict):
                continue
            behavior_id = trace.get("behavior_id", "")
            _known("EvaluateInstructionGenerator", behavior_index, behavior_id, entity_id=trace.get("entity_id", ""), ability_id=trace.get("ability_id", ""))
            mapped = mapping_by_behavior.get(behavior_id)
            if mapped:
                if trace.get("flow_id") != mapped.get("flow_id") or trace.get("runtime_mapping_path") != runtime_mapping_path:
                    raise Phase2ValidationError(f"EvaluateInstructionGenerator: trace must come from runtime mapping: {behavior_id}")
                expected_ports = {p.get("engine_port_id", "") for p in mapped.get("engine_port_mappings", [])}
                if not expected_ports.issubset(set(trace.get("engine_port_ids", []))):
                    raise Phase2ValidationError(f"EvaluateInstructionGenerator: trace missing engine_port_ids for {behavior_id}: {sorted(expected_ports - set(trace.get('engine_port_ids', [])))}")
                expected_adj = {p.get("adjudication_path", "") for p in mapped.get("engine_port_mappings", [])}
                if not expected_adj.issubset(set(trace.get("adjudication_paths", []))):
                    raise Phase2ValidationError(f"EvaluateInstructionGenerator: trace missing adjudication_paths for {behavior_id}: {sorted(expected_adj - set(trace.get('adjudication_paths', [])))}")
            missing_files = [p for p in trace.get("ts_files", []) if rendered and p not in rendered]
            if missing_files:
                raise Phase2ValidationError(f"EvaluateInstructionGenerator: ts_files were not rendered by TypeScriptInteractiveObjectGenerator or TypeScriptCodeGenerator: {missing_files}")
            coverage.setdefault(behavior_id, set()).add("EvaluateInstructionGenerator")

    evidence = {
        "behavior_ids": sorted(behavior_index),
        "flow_ids": sorted(flow_by_id),
        "engine_port_ids": sorted(mcp_by_port or behavior_by_port),
        "mcp_adjudication_paths": sorted(adjudication_by_path),
        "runtime_mapping_path": runtime_mapping_path,
        "analyzer_targets": sorted(analyzer_targets),
        "interactive_files": sorted(interactive_files),
        "codegen_files": sorted(codegen_files),
    }
    if require_all and behavior_index:
        required = {"ThinGameplayFlowPlanner", "UEApiMCPFeasibilitySearcher", "PuerTSRuntimeMappingPlanner", "TypeScriptScriptAnalyzer", "TypeScriptInteractiveObjectGenerator", "TypeScriptCodeGenerator", "EvaluateInstructionGenerator"}
        missing = {bid: sorted(required - srcs) for bid, srcs in coverage.items() if required - srcs}
        if missing:
            raise Phase2ValidationError(f"Phase2 trace coverage missing for behavior_id: {missing}")
        evidence["behavior_trace_coverage"] = {bid: sorted(srcs) for bid, srcs in sorted(coverage.items())}
    return evidence

def validate_phase2_output_set(outputs: Mapping[str, str]) -> dict[str, Any]:
    missing = [name for name in PHASE2_NODE_ORDER if not outputs.get(name, "").strip()]
    if missing:
        raise Phase2ValidationError(f"missing Phase2 LLM outputs: {missing}")
    data = {name: parse_validated_phase2_output(name, outputs[name]) for name in PHASE2_NODE_ORDER}
    return {"data_by_node": data, "evidence": _cross(data, True)}

def validate_partial_phase2_outputs(outputs: Mapping[str, str]) -> dict[str, Any]:
    data = {name: parse_validated_phase2_output(name, text) for name, text in outputs.items() if name in NODE_VALIDATORS and isinstance(text, str) and text.strip()}
    return {"data_by_node": data, "evidence": _cross(data, False)}

def validate_phase2_node_output(node, state, output: str) -> str:
    canonical = validate_phase2_output(node.name, output)
    prior = dict(getattr(state, "llm_outputs", {}) or {})
    prior[node.name] = canonical
    validate_partial_phase2_outputs(prior)
    return canonical
