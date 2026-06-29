from __future__ import annotations

import json
from pathlib import Path

from core.BaseLLMNode import BaseLLMNode, GraphState
from core.phase2_validation import RUNTIME_MAPPING_PATH, parse_phase2_json, validate_phase2_node_output, validate_phase2_output

PUERTS_RUNTIME_MAPPING_PLANNER_PROMPT = """SCHEMA: PuerTSRuntimeMappingPlanner
Map thin gameplay flows and UE API MCP adjudications to PuerTS runtime carriers with explicit implementation_carrier, adapter_or_helper, existing framework decision, and verification evidence. Return JSON only.
"""

def _output_root(state: GraphState) -> Path:
    save_dir = getattr(state, "save_dir", "")
    if not save_dir:
        raise RuntimeError("state.save_dir is required for runtime mapping emission")
    root = Path(save_dir).resolve()
    root.mkdir(parents=True, exist_ok=True)
    return root

def build_runtime_mapping_context(node: BaseLLMNode, state: GraphState, full_input: str) -> None:
    required = {
        "EntityAbilityBehaviorPlanner": state.llm_outputs.get("EntityAbilityBehaviorPlanner", ""),
        "ThinGameplayFlowPlanner": state.llm_outputs.get("ThinGameplayFlowPlanner", ""),
        "UEApiMCPFeasibilitySearcher": state.llm_outputs.get("UEApiMCPFeasibilitySearcher", ""),
    }
    missing = [name for name, value in required.items() if not value.strip()]
    if missing:
        raise RuntimeError(f"PuerTSRuntimeMappingPlanner missing upstream outputs: {missing}")
    node.full_input = "\n\n".join(f"{name} JSON:\n{value}" for name, value in required.items()) + (
        "\n\nCreate one mapping for every behavior. runtime_mapping_path must be "
        + RUNTIME_MAPPING_PATH
        + ". runtime_owner should be the behavior-level generated TypeScript ability file under TypeScript/content/generated/. Include implementation_carrier, existing_framework_candidates, why_not_existing_framework, temporary_or_canonical, migration_path, and adapter_or_helper for every engine port."
    )

def write_runtime_mapping(state: GraphState, output: str) -> None:
    data = parse_phase2_json("PuerTSRuntimeMappingPlanner", validate_phase2_output("PuerTSRuntimeMappingPlanner", output))
    target = _output_root(state) / RUNTIME_MAPPING_PATH
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"[SUCCESS] PuerTS runtime mapping saved to: {target}")

def create_puerts_runtime_mapping_planner() -> BaseLLMNode:
    return BaseLLMNode(
        name="PuerTSRuntimeMappingPlanner",
        prompt=PUERTS_RUNTIME_MAPPING_PLANNER_PROMPT,
        pre_action=build_runtime_mapping_context,
        output_validator=validate_phase2_node_output,
        post_action=write_runtime_mapping,
        enable_feedback=False,
    )
