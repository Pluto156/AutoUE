import sys
import os
import json
import re
from typing import Dict, Any
import random
from dotenv import load_dotenv
load_dotenv()
# Keep project path imports (modify if needed)
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.getenv("ROOT_DIR"))

from core.BaseLLMNode import BaseLLMNode
from core.BaseLLMNode import GraphState
from embed.embed_pcg_pins import query_pcg_pins
from embed.embed_pcg_docs import query_documentation, query_parameter_space

# ============================================================
# New PCG Graph Composition Prompt
#    (Includes TransformPoints Z-axis random rotation rules)
# ============================================================
PCG_PLANNER_PROMPT = """
You are an expert in Unreal Engine 5 Procedural Content Generation (PCG) systems.
Your responsibility is to generate a complete PCG graph structure (JSON)
based on the input model information and node definitions.

The system will provide:
- Parameter Space descriptions for each node
- Pin definitions (Input Pins / Output Pins)
- A list of available interactive actor class names that can be used by
  PCGSpawnActorSettings nodes:

InteractiveObjectClassList: {{extra_input}}

=== Key Constraints (MUST be strictly followed) ===

1. Output format (STRICT JSON, no code blocks, no explanatory text):
{
  "model": "<from input model field>",
  "pattern_desc": "<from input pattern_desc field>",
  "nodes": [
    { "type": "PCGGetLandscapeSettings@[0,0]", "parameters": { ... } },
    ...
  ],
  "connections": [
    "0.Out|1.In"
  ]
}

2. For PCGSpawnActorSettings:
- The parameters MUST include a field named "TemplateActor"
- TemplateActor MUST be selected from InteractiveObjectClassList
- Do NOT invent or fabricate class names

3. **PCGTransformPointsSettings special rule (MANDATORY)**:
- When the node type is `PCGTransformPointsSettings`, parameters MUST include:
  - RotationMin (FRotator)
  - RotationMax (FRotator)
- Rotation is ONLY allowed around the Z axis (Yaw):
  - Pitch MUST be 0
  - Roll MUST be 0
- The format of RotationMin / RotationMax MUST be:
{
  "X": 0,
  "Y": 0,
  "Z": <value>
}

Rotation constraints (ENFORCED):
- Only random rotation around Z axis is allowed
- X (Pitch) MUST always be 0
- Y (Roll) MUST always be 0
- No non-zero randomness is allowed on X / Y axes

4. **Connection generation rules (CRITICAL)**:
- You MUST generate connections strictly based on node Pins
  (Input Pins / Output Pins). Do NOT guess connections.
- Connection string format MUST be:
  "A.<OutputPinName>|B.<InputPinName>"
- Connection principles:
  1. For each output pin, find the FIRST compatible input pin in subsequent nodes
  2. Types must be strictly or loosely compatible (Point → Point, Spatial → Spatial)
  3. Prefer pins that appear earlier in the pin list
  4. If no compatible input pin exists, do NOT create a connection
  5. Cross-model connections are NOT allowed
- Connection order MUST be deterministic (increasing output node index)

5. Parameter filling rules:
- parameters may ONLY use fields that exist in the parameter descriptions
- Prefer default values or explicitly listed optional values
- If uncertain, use conservative example values (e.g. 1, true, false)
- Do NOT introduce nonexistent structures or field names

6. Pin parsing requirements:
- Pin content may be plain text or structured JSON
- You MUST parse Input Pin / Output Pin names and types
- If pin information is insufficient, generate only the most obvious Out → In connection

7. Output validity (MANDATORY):
- Output MUST be valid JSON
- The number of nodes MUST match the input, order preserved
- The type field MUST fully preserve "<NodeName>@[x,y]"
- All indices and pin names referenced in connections MUST exist
- Do NOT output any extra fields or explanatory text

=== Example ===
PCGSurfaceSamplerSettings.Out (Point)
→ PCGPointFilterSettings.In (Point)

Connection string:
"0.Out|1.In"

=== Final Reminder ===
- ALL rules must be satisfied simultaneously
- Z-axis random rotation for TransformPoints is MANDATORY
- Conservative, deterministic, and reproducible behavior is preferred

Now wait for the user-provided model JSON and generate the final valid JSON.
"""

InteractiveObjectClassListStr = ""


# ============================================================
# Utility: Assign unique Seed values to all nodes
# ============================================================
def assign_unique_seeds(nodes: list):
    """
    Traverse the nodes array and assign a unique random ID to all "Seed" fields
    in parameters.
    - Random range: 1 ~ 2^31 - 1
    - No duplication within the same model
    """
    used_seeds = set()

    def generate_seed():
        while True:
            v = random.randint(1, 2**31 - 1)
            if v not in used_seeds:
                used_seeds.add(v)
                return v

    for node in nodes:
        params = node.get("parameters", {})
        if not isinstance(params, dict):
            continue

        if "Seed" in params:
            params["Seed"] = generate_seed()

    return nodes


# ============================================================
# PCG Graph Composition Node
# ============================================================
class PCGGraphComposerNode(BaseLLMNode):
    def call_model(self, full_input: str, state: GraphState):
        print("========== call_model full_input ==========")
        print(full_input)
        print("===========================================")

        try:
            raw = state.llm_outputs.get("PcgGraphComposer", "") if state else ""

            cleaned = raw.strip()
            cleaned = re.sub(r"^```(?:json)?", "", cleaned, flags=re.IGNORECASE).strip()
            cleaned = re.sub(r"```$", "", cleaned).strip()

            models = json.loads(cleaned)
            print(f"[DEBUG] Successfully parsed {len(models)} models.")

        except Exception as e:
            print(f"[ERROR] Failed to parse input JSON: {e}")
            return "{}"

        node_cache: Dict[str, Dict[str, Any]] = {}
        final_data = []

        for idx, m in enumerate(models):
            model_name = m.get("model", f"Model_{idx}")
            pattern_desc = m.get("pattern_desc", "")
            node_list = m.get("nodes", [])
            node_names = [n.split("@")[0] for n in node_list]

            context_text = "node info:\n"
            for nodename in node_names:
                if nodename not in node_cache:
                    node_cache[nodename] = {
                        "param": query_parameter_space(nodename, nodename, 5),
                        "pin": query_pcg_pins(nodename, nodename, 5)
                    }

                param = node_cache[nodename]["param"]
                pin = node_cache[nodename]["pin"]

                if param and pin and param[0]["node_name"] == pin[0]["node_name"]:
                    context_text += (
                        f"{param[0]['node_name']} node info:\n"
                        f"{param[0]['content']}\n"
                        f"{pin[0]['content']}\n\n"
                    )

            single_input = (
                json.dumps(m, ensure_ascii=False)
                + "\n"
                + InteractiveObjectClassListStr
                + "\n"
                + context_text
            )

            response = self.invoke_model_with_token_tracking([
                {"role": "system", "content": self.prompt},
                {"role": "user", "content": single_input}
            ],state)

            result_text = response.content.strip()

            try:
                generated = json.loads(result_text)
            except json.JSONDecodeError:
                generated = {"model": model_name, "raw_output": result_text}
                final_data.append(generated)
                continue

            if "nodes" in generated:
                generated["nodes"] = assign_unique_seeds(generated["nodes"])

            final_data.append(generated)

        output_dir = os.path.join(os.getenv("UE_Save"), "PCG")
        os.makedirs(output_dir, exist_ok=True)
        output_path = os.path.join(output_dir, "ComposedGraph.json")

        with open(output_path, "w", encoding="utf-8") as f:
            json.dump(final_data, f, indent=2, ensure_ascii=False)

        return json.dumps(final_data, indent=2, ensure_ascii=False)


# ============================================================
# extra_input
# ============================================================
def GetInput(node: BaseLLMNode, state: GraphState, full_input: str):
    global InteractiveObjectClassListStr
    raw = state.llm_outputs.get("InteractiveObjectCodeGenerator", "") if state else ""
    cleaned = re.sub(r"^```(?:json)?|```$", "", raw.strip())
    data = json.loads(cleaned) if cleaned else {}
    class_names = [
        o["class_name"]
        for o in data.get("objects", [])
        if "class_name" in o
    ]
    InteractiveObjectClassListStr = (
        "InteractiveObjectClassList:" + ",".join(class_names)
    )


def create_pcg_planner() -> PCGGraphComposerNode:
    node = PCGGraphComposerNode(
        name="PCGPlanner",
        prompt=PCG_PLANNER_PROMPT,
        isDebug=False,
        pre_action=GetInput,
        post_action=save_json,
        enable_feedback=False
    )
    return node


def save_llm_outputs(state: GraphState, demo_output_dir: str):
    llm_outputs = getattr(state, "llm_outputs", None)
    if not llm_outputs:
        print("[WARN] No llm_outputs found, skip saving")
        return

    output_dir = os.path.join(demo_output_dir, "llm_outputs")
    os.makedirs(output_dir, exist_ok=True)

    for key, value in llm_outputs.items():
        file_path = os.path.join(output_dir, f"{key}.txt")
        with open(file_path, "w", encoding="utf-8") as f:
            f.write(str(value))

        print(f"[DEBUG] Saved llm_output: {file_path}")

def save_json(state: GraphState, output):
    """
    Save the cleaned JSON content to disk.
    """
    save_llm_outputs(state,state.save_dir)
    

# ============================================================
# Node instance
# ============================================================
pcg_planner = PCGGraphComposerNode(
    name="PCGPlanner",
    prompt=PCG_PLANNER_PROMPT,
    isDebug=False,
    pre_action=GetInput,
    post_action=save_json,
    enable_feedback=False
)
