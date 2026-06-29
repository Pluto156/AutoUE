from __future__ import annotations

import json
from pathlib import Path

from core.BaseLLMNode import BaseLLMNode, GraphState
from core.phase2_validation import parse_phase2_json, validate_phase2_node_output, validate_phase2_output

ENTITY_ABILITY_BEHAVIOR_PLANNER_PROMPT = """SCHEMA: EntityAbilityBehaviorPlanner
Create definition-layer gameplay structure only: behaviors grouped into abilities owned by entities. Analyze behavior-first, but return the machine tree as entities -> abilities -> behaviors. Do not decide implementation files or write code. Return JSON only.
"""


def _output_root(state: GraphState) -> Path:
    save_dir = getattr(state, "save_dir", "")
    if not save_dir:
        raise RuntimeError("state.save_dir is required for 02 structure emission")
    root = Path(save_dir).resolve()
    root.mkdir(parents=True, exist_ok=True)
    return root


def build_planner_context(node: BaseLLMNode, state: GraphState, full_input: str) -> None:
    node.full_input = (
        "User/scene/gameplay input:\n"
        + full_input
        + "\n\nRules: first identify concrete behaviors from the request, then group behaviors into abilities, then assign abilities to entities. "
        "The returned JSON must still use entities[] -> abilities[] -> behaviors[]. "
        "Treat camera, VFX, animation, HUD, input, trap, exit, enemy, and player as concrete entities when they own behavior/state; do not use HUD/VFX/Camera as vague buckets. "
        "Do not include engine_ports, files, templates, runtime owners, or code fields."
    )


def write_02_structure(state: GraphState, output: str) -> None:
    data = parse_phase2_json("EntityAbilityBehaviorPlanner", validate_phase2_output("EntityAbilityBehaviorPlanner", output))
    root = _output_root(state)
    flow_dir = root / "flow"
    flow_dir.mkdir(parents=True, exist_ok=True)
    structure = {
        "schema_version": "autoue-02-structure/v1",
        "analysis_order": "behavior -> ability -> entity",
        "machine_tree": "entities -> abilities -> behaviors",
        "entities": data.get("entities", []),
        "non_goals": data.get("non_goals", []),
    }
    (flow_dir / "02-structure.json").write_text(json.dumps(structure, ensure_ascii=False, indent=2), encoding="utf-8")
    lines = ["# 02 结构化拆解：行为-能力-实体", "", "分析顺序：行为 → 能力 → 实体；机器落档：entities → abilities → behaviors。", ""]
    for entity in data.get("entities", []):
        lines.append(f"## Entity: `{entity.get('entity_id','')}` · {entity.get('display_name','')}")
        lines.append(entity.get("summary", ""))
        lines.append("")
        for ability in entity.get("abilities", []):
            lines.append(f"### Ability: `{ability.get('ability_id','')}` · {ability.get('display_name','')}")
            lines.append(ability.get("summary", ""))
            for behavior in ability.get("behaviors", []):
                lines.append(f"- Behavior `{behavior.get('behavior_id','')}`: {behavior.get('trigger','')} → {behavior.get('execution','')} → {behavior.get('result','')}")
            lines.append("")
    (flow_dir / "02-结构化拆解-行为能力实体.md").write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")
    print(f"[SUCCESS] 02 structure saved under: {flow_dir}")


def create_entity_ability_behavior_planner() -> BaseLLMNode:
    return BaseLLMNode(
        name="EntityAbilityBehaviorPlanner",
        prompt=ENTITY_ABILITY_BEHAVIOR_PLANNER_PROMPT,
        pre_action=build_planner_context,
        output_validator=validate_phase2_node_output,
        post_action=write_02_structure,
        enable_feedback=False,
    )
