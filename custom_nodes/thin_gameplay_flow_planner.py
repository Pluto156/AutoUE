from __future__ import annotations

import json
from pathlib import Path

from core.BaseLLMNode import BaseLLMNode, GraphState
from core.phase2_validation import parse_phase2_json, validate_phase2_node_output, validate_phase2_output

THIN_GAMEPLAY_FLOW_PLANNER_PROMPT = """SCHEMA: ThinGameplayFlowPlanner
Create one thin gameplay flow for every behavior. Return JSON only.
"""

def _output_root(state: GraphState) -> Path:
    save_dir = getattr(state, "save_dir", "")
    if not save_dir:
        raise RuntimeError("state.save_dir is required for thin flow emission")
    root = Path(save_dir).resolve()
    root.mkdir(parents=True, exist_ok=True)
    return root

def build_thin_flow_context(node: BaseLLMNode, state: GraphState, full_input: str) -> None:
    plan = state.llm_outputs.get("EntityAbilityBehaviorPlanner", "")
    if not plan.strip():
        raise RuntimeError("ThinGameplayFlowPlanner requires EntityAbilityBehaviorPlanner output")
    node.full_input = (
        "EntityAbilityBehaviorPlanner JSON:\n" + plan
        + "\n\nCreate flow stages and engine_ports for every behavior. "
        + "Do not choose TypeScript files and do not call MCP yourself."
    )

def write_thin_flow(state: GraphState, output: str) -> None:
    data = parse_phase2_json("ThinGameplayFlowPlanner", validate_phase2_output("ThinGameplayFlowPlanner", output))
    target = _output_root(state) / "flow" / "03-thin-gameplay-flow.json"
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"[SUCCESS] thin gameplay flow saved to: {target}")

def create_thin_gameplay_flow_planner() -> BaseLLMNode:
    return BaseLLMNode(
        name="ThinGameplayFlowPlanner",
        prompt=THIN_GAMEPLAY_FLOW_PLANNER_PROMPT,
        pre_action=build_thin_flow_context,
        output_validator=validate_phase2_node_output,
        post_action=write_thin_flow,
        enable_feedback=False,
    )
