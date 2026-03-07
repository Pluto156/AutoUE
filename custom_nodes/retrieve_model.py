# ============================================================
# File: retrieve_model.py
# Purpose:
#   Retrieve the best-matching 3D models from a vector database
#   based on `prompt_for_embedding` produced by KeyElementExtractor.
# ============================================================

from core.BaseLLMNode import BaseLLMNode, GraphState
import re
import json
import os

from model_description.model_retrive.search_model import get_model_id
from model_description.model_retrive.search_model import get_model_description
from model_description.SkechFab import download_models_from_uid_list


# ============================================================
# Node Prompt (Enhanced decision using score + description)
# ============================================================

RETRIEVE_MODEL_PROMPT = """
You are a model retrieval decision assistant in a procedural generation system.

Your task is:
**For each model requirement, select the most appropriate model UID from the candidates.**

You will receive input in the following array format:
[
  {
    "model_name": "OakTree",
    "prompt_for_embedding": "Oak tree, forest terrain, morning light",
    "candidate_models": [
      {
        "uid": "model_uid_1",
        "score": 0.82,
        "description": "A low-poly oak tree model suitable for mobile"
      },
      {
        "uid": "model_uid_2",
        "score": 0.76,
        "description": "A realistic oak tree with detailed bark and leaves"
      }
    ]
  }
]

【Important Decision Rules (Must Follow)】

1. **score represents vector similarity; higher values mean closer semantic match**
2. You must make a decision by considering all three of the following:
   - The user's `prompt_for_embedding` intent
   - The candidate model's `description`
   - The candidate model's `score`
3. If the description clearly matches the semantic intent better, you may choose a model with a slightly lower score
4. If descriptions are similar, prefer the model with the higher score
5. Do not rely only on score, and do not ignore score

【Output Format (Strict JSON, no extra text)】

{
  "selected_models": [
    {
      "model_name": "OakTree",
      "selected_uid": "model_uid_2"
    }
  ]
}
"""


# ============================================================
# pre_action: Retrieve Top-5 candidates + organize score/description
# ============================================================

def PreActionRetrieve(node: BaseLLMNode, state: GraphState, full_input: str):
    """
    Iterate over the models output by KeyElementExtractor.
    For each model, retrieve Top-5 candidates (uid + score + description),
    and pass them to the LLM for final selection.
    """
    raw = state.llm_outputs.get("KeyElementExtractor", "") if state else ""

    cleaned = raw.strip()
    cleaned = re.sub(r"^```(?:json)?", "", cleaned, flags=re.IGNORECASE).strip()
    cleaned = re.sub(r"```$", "", cleaned).strip()

    try:
        data = json.loads(cleaned)
    except Exception as e:
        return f"[JSON Parse Error] {e}"

    models = data.get("models", [])
    input_for_llm = []

    for m in models:
        category = m.get("category", "")
        prompt = m.get("prompt_for_embedding", "")
        model_name = m.get("name", "")

        # Vector search (returns model_id + score)
        top5 = get_model_id(category, prompt, top_k=5)

        candidate_models = []
        for item in top5:
            uid = item.get("model_id")
            score = item.get("score", 0.0)
            description = get_model_description(uid)

            candidate_models.append({
                "uid": uid,
                "score": score,
                "description": description
            })

        input_for_llm.append({
            "model_name": model_name,
            "prompt_for_embedding": prompt,
            "candidate_models": candidate_models
        })

    node.full_input = json.dumps(input_for_llm, ensure_ascii=False, indent=2)


# ============================================================
# post_action: Save JSON + download selected models
# ============================================================

def download_model(state: GraphState, output):
    """
    Parse the selected_uid list from the LLM output,
    save the cleaned JSON, and download the selected models.
    """

    save_path = os.path.join(os.getenv("UE_Save"), "PCG", "RetriveModel.json")

    try:
        cleaned = output.strip()
        cleaned = re.sub(r"^```(?:json)?", "", cleaned)
        cleaned = re.sub(r"```$", "", cleaned)

        os.makedirs(os.path.dirname(save_path), exist_ok=True)
        with open(save_path, "w", encoding="utf-8") as f:
            f.write(cleaned)

        print(f"[INFO] Cleaned JSON saved to: {save_path}")

        data = json.loads(cleaned)

    except Exception as e:
        print(f"[ERROR] Failed to parse model output as JSON: {e}")
        print("[DEBUG] Raw output:\n", output)
        return

    selected = data.get("selected_models", [])
    if not isinstance(selected, list):
        print("[ERROR] Invalid selected_models field")
        return

    uid_list = []
    for item in selected:
        uid = item.get("selected_uid")
        if uid:
            uid_list.append(uid)

    if not uid_list:
        print("[ERROR] No model UIDs to download")
        return

    try:
        print(f"[INFO] Starting download of {len(uid_list)} models: {uid_list}")
        download_models_from_uid_list(uid_list)
        print("[INFO] Model download completed")
    except Exception as e:
        print(f"[ERROR] Model download failed: {e}")


def create_retrieve_model_node() -> BaseLLMNode:
    node = BaseLLMNode(
        name="RetrieveModel",
        prompt=RETRIEVE_MODEL_PROMPT,
        pre_action=PreActionRetrieve,
        post_action=download_model,
        enable_feedback=False
    )
    return node


# ============================================================
# Node definition
# ============================================================

retrieve_model_node = BaseLLMNode(
    name="RetrieveModel",
    prompt=RETRIEVE_MODEL_PROMPT,
    pre_action=PreActionRetrieve,
    post_action=download_model,
    enable_feedback=False
)
