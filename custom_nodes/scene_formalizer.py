from core.BaseLLMNode import BaseLLMNode
from core.BaseLLMNode import GraphState

SCENE_FORMALIZER_PROMPT = """
You are an expert in scene description generation. Based on the user's natural language description, generate a formal scene description JSON.
Please strictly follow the structure below (output must be valid JSON):

{
  "scene_name": "A short scene title",
  "theme": "Theme (e.g., ancient castle, sci-fi city, forest campsite, etc.)",
  "style": "Art style (e.g., realistic, cartoon, low-poly, etc.)",
  "environment": {
    "terrain_type": "Terrain type (mountain, plain, desert, coast, etc.)",
    "climate": "Climate (sunny, rainy, night, foggy, etc.)",
    "lighting": "Lighting type (morning light, sunset, night lighting, dynamic, etc.)"
  },
  "main_elements": [
    "Primary elements such as trees, buildings, roads, water bodies, rocks, etc."
  ],
  "mood": "Overall atmosphere description (peaceful, mysterious, lively, oppressive, etc.)"
}
"""

def GetInput(node: BaseLLMNode, state: GraphState, full_input: str):
    node.full_input = ""
    node.full_input = state.scene_description

def create_scene_formalizer() -> BaseLLMNode:
    node = BaseLLMNode(
        name="SceneFormalizer",
        prompt=SCENE_FORMALIZER_PROMPT,
        pre_action=GetInput,
        enable_feedback=False
    )
    return node

scene_formalizer = BaseLLMNode(
    name="SceneFormalizer",
    prompt=SCENE_FORMALIZER_PROMPT,
    pre_action=GetInput,
    enable_feedback=False
)
