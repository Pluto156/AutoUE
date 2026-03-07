import os
import json
import numpy as np
from tqdm import tqdm
from openai import OpenAI


# Get the directory of the current file
BASE_DIR = os.path.dirname(os.path.abspath(__file__))

# Build the absolute path to the embs directory
EMB_DIR = os.path.join(BASE_DIR, "embs")


def get_model_description(uid: str):
    """
    Read caption.json using the model UID and return the model description text.

    caption.json format:
    {
        "uid1": "description1",
        "uid2": "description2",
        ...
    }
    """
    caption_path = os.path.join(EMB_DIR, "caption.json")

    if not os.path.exists(caption_path):
        print("caption.json not found:", caption_path)
        return None

    with open(caption_path, "r", encoding="utf-8") as f:
        caption_dict = json.load(f)

    # Convert UID to string before lookup
    uid_str = str(uid)

    return caption_dict.get(uid_str, None)


def get_model_id(category, text, top_k=1):
    # 1. Load category → [real_id] mapping
    category_json_path = os.path.join(EMB_DIR, "category.json")
    with open(category_json_path, "r", encoding="utf-8") as f:
        category_dict = json.load(f)

    if category not in category_dict:
        print("category not found")
        return None

    # 2. Load embeddings and ID mapping
    embeddings_path = os.path.join(EMB_DIR, "embeddings.npy")
    embeddings = np.load(embeddings_path).astype(np.float32)

    id_mapper_path = os.path.join(EMB_DIR, "id_mapper.json")
    with open(id_mapper_path, "r", encoding="utf-8") as f:
        id2real_mapper = json.load(f)

    # id2real_mapper: internal_id (str) -> real_id
    real2id_mapper = {v: k for k, v in id2real_mapper.items()}

    # 3. Call embedding API
    client = OpenAI(
        api_key=os.environ.get("DASHSCOPE_API_KEY"),
        base_url="https://dashscope.aliyuncs.com/compatible-mode/v1"
    )

    try_cnt, cur_cnt = 10, 1
    emb = None
    while cur_cnt <= try_cnt:
        completion = client.embeddings.create(
            model="text-embedding-v4",
            input=text
        )
        if len(completion.data) > 0:
            emb = completion.data[0].embedding
            break
        cur_cnt += 1

    if emb is None:
        print("Embedding is None")
        return None

    # 4. Normalize query vector
    query_emb = np.array(emb, dtype=np.float32)
    query_norm = query_emb / (np.linalg.norm(query_emb) + 1e-8)

    # 5. Vector search within the given category (cosine similarity)
    recall_ids = category_dict[category]
    sims = []
    real_ids = []

    for real_id in tqdm(recall_ids):
        if str(real_id) not in real2id_mapper:
            continue

        internal_id_str = real2id_mapper[str(real_id)]
        internal_id = int(internal_id_str)
        recall_emb = embeddings[internal_id]

        recall_norm = recall_emb / (np.linalg.norm(recall_emb) + 1e-8)
        sim = float(np.dot(query_norm, recall_norm))

        sims.append(sim)
        real_ids.append(real_id)

    if not sims:
        print("no valid recall ids for this category")
        return None

    sims = np.array(sims)
    real_ids = np.array(real_ids)

    # 6. Select top_k results
    top_k = min(top_k, len(sims))
    top_indices = np.argsort(-sims)[:top_k]

    top_real_ids = real_ids[top_indices].tolist()
    top_scores = sims[top_indices].tolist()

    if top_k == 1:
        return {
            "model_id": top_real_ids[0],
            "score": top_scores[0]
        }

    return [
        {"model_id": rid, "score": sc}
        for rid, sc in zip(top_real_ids, top_scores)
    ]


# ----------------------- TEST --------------------------
if __name__ == "__main__":

    # Test vector retrieval
    res = get_model_id(
        "Furniture & Home",
        "Tent, campsite gear, realistic canvas fabric, triangular shape",
        10
    )
    print("Top results:", res)

    # Test model description retrieval
    if res:
        uid = res[0]["model_id"] if isinstance(res, list) else res["model_id"]
        desc = get_model_description(uid)
        print("\nDescription for model", uid, ":")
        print(desc)
