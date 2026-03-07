# ============================================================
# File: scene_and_gameplay_splitter.py
# Purpose:
#   The first LLM node.
#   Split the user's natural language prompt into two parts:
#       - Scene description (scene_description)
#       - Gameplay feature description (gameplay_description)
# ============================================================

from core.BaseLLMNode import BaseLLMNode
from core.BaseLLMNode import GraphState
import json
import re

# ============================================================
# Prompt
# ============================================================
SCENE_AND_GAMEPLAY_SPLITTER_PROMPT = """
You are a game design and procedural content analysis expert.

Your task:
Decompose the user's natural language input into two clear parts:

1. 【Scene Description】
   - Describe the environment and visual context, such as:
     environment type, terrain, architecture, climate, time of day,
     atmosphere, spatial layout, and major visible elements.
   - The description should be moderately detailed and concrete,
     but should NOT invent content beyond the user's input.

2. 【Gameplay Description】
   - Describe what the player can do in the scene, including:
     player actions, interaction logic, gameplay mechanics,
     involved systems or modules, and high-level objectives.
   - The description should clearly express gameplay intent and flow,
     without adding new mechanics not implied by the user.

【Input Example】
"I want a forest campsite scene where the player can collect wood, light a fire, and build tents."

【Output Example】
{
  "scene_description": "A forest campsite set among dense trees and natural terrain, featuring tents, a campfire area, and a calm outdoor atmosphere, likely during daytime or dusk.",
  "gameplay_description": "The player can explore the campsite, collect wood resources, interact with objects to light a campfire, and build or place tents through construction-related gameplay actions."
}

【Requirements】
- Output must be strictly in JSON format.
- If the user input contains only scene-related content, leave gameplay_description empty.
- If it contains only gameplay-related content, leave scene_description empty.
- Keep the language clear, structured, and moderately detailed.
"""

def SplitUserDescription(state: GraphState, output):
    try:
        cleaned = output.strip()
        cleaned = re.sub(r"^```(?:json)?", "", cleaned.strip())
        cleaned = re.sub(r"```$", "", cleaned.strip())
        data = json.loads(cleaned)
    except Exception as e:
        print(f"[ERROR] Failed to parse model output as JSON: {e}")
        print("[DEBUG] Output content:\n", output)
        return

    state.scene_description = data.get("scene_description", "")
    state.gameplay_description = data.get("gameplay_description", "")


def create_scene_and_gameplay_splitter() -> BaseLLMNode:
    node = BaseLLMNode(
        name="SceneAndGameplaySplitter",
        prompt=SCENE_AND_GAMEPLAY_SPLITTER_PROMPT,
        enable_feedback=False,
        post_action=SplitUserDescription
    )
    return node


# ============================================================
# Node definition
# ============================================================
scene_and_gameplay_splitter = create_scene_and_gameplay_splitter()
