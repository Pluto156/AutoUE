from __future__ import annotations
from core.BaseLLMNode import BaseLLMNode, GraphState
from core.phase2_validation import validate_phase2_node_output
ENTITY_ABILITY_BEHAVIOR_PLANNER_PROMPT = """SCHEMA: EntityAbilityBehaviorPlanner
Create definition-layer gameplay structure only: entities, abilities, behaviors. Do not decide implementation files or write code. Return JSON only.
"""
def build_planner_context(node: BaseLLMNode, state: GraphState, full_input: str) -> str:
    split = state.llm_outputs.get("SceneAndGameplaySplitter", "")
    if not split.strip():
        raise RuntimeError("EntityAbilityBehaviorPlanner requires SceneAndGameplaySplitter output")
    return (
        "SceneAndGameplaySplitter JSON:\n" + split
        + "\n\nState scene_description:\n" + getattr(state, "scene_description", "")
        + "\n\nState gameplay_description:\n" + getattr(state, "gameplay_description", "")
    )
def create_entity_ability_behavior_planner() -> BaseLLMNode:
    return BaseLLMNode(name="EntityAbilityBehaviorPlanner", prompt=ENTITY_ABILITY_BEHAVIOR_PLANNER_PROMPT, output_validator=validate_phase2_node_output, extra_prompt_action=build_planner_context, enable_feedback=False)
