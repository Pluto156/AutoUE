from __future__ import annotations
import json
from core.BaseLLMNode import BaseLLMNode, GraphState
from core.phase2_validation import validate_phase2_node_output, validate_phase2_output
SCENE_AND_GAMEPLAY_SPLITTER_PROMPT = """SCHEMA: SceneAndGameplaySplitter
Split the user request into scene_description and gameplay_description. Return JSON only.
"""
def SplitUserDescription(state: GraphState, output: str) -> None:
    data = json.loads(validate_phase2_output("SceneAndGameplaySplitter", output))
    state.scene_description = data.get("scene_description", "")
    state.gameplay_description = data.get("gameplay_description", "")
def create_scene_and_gameplay_splitter() -> BaseLLMNode:
    return BaseLLMNode(name="SceneAndGameplaySplitter", prompt=SCENE_AND_GAMEPLAY_SPLITTER_PROMPT, enable_feedback=False, output_validator=validate_phase2_node_output, post_action=SplitUserDescription)
scene_and_gameplay_splitter = create_scene_and_gameplay_splitter()
