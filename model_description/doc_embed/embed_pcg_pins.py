# embed_pcg_pins.py
import re
import os
from langchain_huggingface import HuggingFaceEmbeddings
from langchain_community.vectorstores import FAISS
from langchain.docstore.document import Document
from typing import List, Dict, Any

# ---------------------------------------------------------
# Always use the script directory as the base path (key modification)
# ---------------------------------------------------------
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PINLIST_PATH = os.path.join(SCRIPT_DIR, "PCG_PinList.txt")
VECTOR_INDEX_PATH = os.path.join(SCRIPT_DIR, "PcgPinsEmbed")

# ---------------------------------------------------------
# Parse the PinList file
# ---------------------------------------------------------
def parse_pinlist_file(path: str) -> List[Document]:
    """Read PCG_PinList.txt and split it into a list of Documents"""
    if not os.path.exists(path):
        raise FileNotFoundError(f"File not found: {path}")

    with open(path, "r", encoding="utf-8") as f:
        text = f.read()

    # Use regex to split into blocks, each node becomes one Document
    pattern = r"\[([^\]]+)\]\s*((?:.|\n)*?)(?=\n\[|$)"
    matches = re.findall(pattern, text)

    docs = []
    for name, content in matches:
        docs.append(Document(
            page_content=name + " " + content.strip(),
            metadata={"node_name": name}
        ))

    return docs

# ---------------------------------------------------------
# Build or load the vector store
# ---------------------------------------------------------
def build_or_load_vectorstore() -> FAISS:
    """Build or load the vector store"""
    embedding = HuggingFaceEmbeddings(model_name="BAAI/bge-base-zh-v1.5")

    if not os.path.exists(VECTOR_INDEX_PATH):
        print("Vector store not found. Building...")
        docs = parse_pinlist_file(PINLIST_PATH)
        vector_store = FAISS.from_documents(docs, embedding)
        vector_store.save_local(VECTOR_INDEX_PATH)
        print(f"Vector store built successfully. Loaded {len(docs)} document blocks.")
    else:
        print("Vector store already exists. Loading...")
        vector_store = FAISS.load_local(
            VECTOR_INDEX_PATH,
            embedding,
            allow_dangerous_deserialization=True
        )
        print("Vector store loaded successfully.")

    return vector_store

# ---------------------------------------------------------
# Query interface
# ---------------------------------------------------------
def query_pcg_pins(node_name: str, query_text: str, k: int = 1) -> List[Dict[str, Any]]:
    """
    Query the PCG vector store and return matched documents
    """
    vector_store = build_or_load_vectorstore()
    if not vector_store:
        return []

    results = vector_store.similarity_search(query_text, k=k)
    output = []
    print(f'query for {node_name}: found {len(results)} results')

    for doc in results:
        if doc.metadata.get("node_name") == node_name:
            output.append({
                "node_name": doc.metadata.get("node_name", "Unknown"),
                "section": doc.metadata.get("section", ""),
                "source_file": doc.metadata.get("source_file", ""),
                "content": doc.page_content.strip(),
            })

    return output

# ---------------------------------------------------------
# Entry point
# ---------------------------------------------------------
if __name__ == "__main__":
    results = query_pcg_pins("PCGTransformPointsSettings", "PCGTransformPointsSettings", 5)
    for r in results:
        print("", r["content"])