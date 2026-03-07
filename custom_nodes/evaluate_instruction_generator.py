# ============================================================
# File Name: evaluate_instruction_generator.py
# Purpose:
#   Automatically generate "evaluation test instructions for a playable game demo"
#   based on:
#     - interactive_object_code_generator
#     - module_code_generator
#     - scene_description
#     - gameplay_description
#
#   The test instructions are used to simulate player behavior, including:
#     - move_to: move to a specified object
#     - interact: interact with an object
#
#   The output is structured JSON for downstream evaluation models.
# ============================================================

from core.BaseLLMNode import BaseLLMNode
from core.BaseLLMNode import GraphState
import os
import json
import re

# ============================================================
# Prompt Definition (includes scene and gameplay descriptions)
# ============================================================
EVALUATE_INSTRUCTION_GENERATOR_PROMPT = """
You are an expert in generating evaluation test instructions for playable UE5 game demos.

You will receive four inputs:
1. **Scene Description**
2. **Gameplay Description**
3. **Output of interactive_object_code_generator**
   (an objects list containing each interactive object's class name and code)
4. **Output of module_code_generator**
   (a modules list)

Your task:
Based on the above inputs—especially the interactive object definitions—automatically generate
a "test instruction script" for evaluating the game demo.

The test instructions are used to drive an automated AI agent to perform the following actions
inside the demo:

### Available Actions:
1. **move_to(object_name)**
   - Parameter: interactive object name
   - Description: Move the AI agent near the specified object

2. **interact(object_name)**
   - Parameter: interactive object name
   - Description: Trigger the object's interaction logic
     (TriggerInteract → HandleInteraction)

---

# Output Format (must be strictly followed)

Output strict JSON with the following structure:

{
  "evaluation_instructions": [
    {
      "step_id": 1,
      "action": "move_to",
      "target": "ObjectName",
      "description": "The AI moves near the object"
    },
    {
      "step_id": 2,
      "action": "interact",
      "target": "ObjectName",
      "description": "The AI interacts with the object"
    }
  ]
}

---

# Output Requirements:

1. Generate **two instructions for each** interactive object:
   - Step 1: move_to
   - Step 2: interact

2. step_id must start from 1 and increment sequentially.

3. "ObjectName" must be automatically derived from `"class_name"`:
   - Remove the prefix `A`
   - Examples:
     - AChestInteractiveObject → ChestInteractiveObject
     - AEnemyBot → EnemyBot

4. Do NOT generate any C++ / Blueprint / UE code.
   Only output JSON test instructions.

5. The JSON must be fully valid and must not contain any extra explanations,
   comments, or natural language outside the JSON structure.
"""

# ============================================================
# Input Construction Function (includes scene + gameplay)
# ============================================================
def GetInput(node: BaseLLMNode, state: GraphState, full_input: str):
    scene_description = getattr(state, "scene_description", "")
    gameplay_description = getattr(state, "gameplay_description", "")

    interactive_objects = state.llm_outputs.get("InteractiveObjectCodeGenerator", "")
    module_outputs = state.llm_outputs.get("ModuleCodeGenerator", "")

    return (
        f"Scene Description:\n{scene_description}\n\n"
        f"Gameplay Description:\n{gameplay_description}\n\n"
        f"interactive_object_code_generator output:\n{interactive_objects}\n\n"
        f"module_code_generator output:\n{module_outputs}\n"
    )

def SaveInstructionjson(state: GraphState, output: str):
    # ============================
    # ① Retrieve EvaluateInstructionGenerator output from the LLM
    # ============================
    instructions = state.llm_outputs.get("EvaluateInstructionGenerator", "")
    if not instructions:
        print("[ERROR] EvaluateInstructionGenerator produced no instructions output")
        return

    # ============================
    # ② Clean JSON (remove ```json ... ``` wrappers)
    # ============================
    cleaned = instructions.strip()
    cleaned = re.sub(r"^```(?:json)?", "", cleaned, flags=re.IGNORECASE).strip()
    cleaned = re.sub(r"```$", "", cleaned).strip()

    # ============================
    # ③ Parse JSON
    # ============================
    try:
        json_data = json.loads(cleaned)
    except json.JSONDecodeError as e:
        print(f"[ERROR] Failed to parse instructions.json: {e}")
        print("Original content:\n", instructions)
        return

    # ============================
    # ④ Target directory
    # ============================
    target_dir = os.path.join(os.getenv("UE_Content"), "MyPCG", "eval")
    os.makedirs(target_dir, exist_ok=True)

    file_path = os.path.join(target_dir, "instructions.json")

    # ============================
    # ⑤ Write JSON file (UTF-8)
    # ============================
    try:
        with open(file_path, "w", encoding="utf-8") as f:
            json.dump(json_data, f, ensure_ascii=False, indent=4)

        print(f"[SUCCESS] instructions.json saved to: {file_path}")

    except Exception as e:
        print(f"[ERROR] Failed to write instructions.json: {e}")

def create_evaluate_instruction_generator() -> BaseLLMNode:
    node = BaseLLMNode(
        name="EvaluateInstructionGenerator",
        prompt=EVALUATE_INSTRUCTION_GENERATOR_PROMPT,
        enable_feedback=False,
        extra_prompt_action=GetInput,
        post_action=SaveInstructionjson
    )
    return node

# ============================================================
# Node Definition
# ============================================================
evaluate_instruction_generator = BaseLLMNode(
    name="EvaluateInstructionGenerator",
    prompt=EVALUATE_INSTRUCTION_GENERATOR_PROMPT,
    enable_feedback=False,
    extra_prompt_action=GetInput,
    post_action=SaveInstructionjson
)
