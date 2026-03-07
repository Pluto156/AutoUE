# ============================================================
# File Name: embed_pcg_docs.py
# Functionality:
#   - Read all Markdown node documents under PCGDoc.
#   - Construct three separate vector indices:
#     - Documentation
#     - Parameter Space
#     - Connection Patterns
#   - Provide three query functions to return matching results.
#   Optimization: Embedding model is only initialized once
# ============================================================

import os
from pathlib import Path
from typing import List, Dict, Any
from functools import lru_cache
from langchain_community.vectorstores import FAISS
from langchain.docstore.document import Document
from langchain_huggingface import HuggingFaceEmbeddings


# =============== Configuration Area ===============
# Ensure paths are relative to the script file
BASE_DIR = Path(__file__).resolve().parent

PCG_DOC_PATH = BASE_DIR / "PCGDoc"                # Markdown documentation directory
VECTOR_INDEX_BASE = str(BASE_DIR / "PcgDocEmbed") # Vector index prefix
EMBED_MODEL = "BAAI/bge-base-zh-v1.5"             # HuggingFace Chinese embedding model
# =====================================


# ============================================================
# 1. Model Cache: Initialize Embedding only once
# ============================================================
@lru_cache(maxsize=1)
def get_embedding() -> HuggingFaceEmbeddings:
    """Initialize HuggingFaceEmbeddings model only once"""
    print("Initializing embedding model (only first time)...")
    return HuggingFaceEmbeddings(model_name=EMBED_MODEL)


# ============================================================
# 2. Markdown Document Parsing
# ============================================================

def extract_sections_from_markdown(path: Path) -> Dict[str, str]:
    """
    Extract the content of each major section from a single Markdown:
    Documentation / Parameter Space / Connection Patterns.
    """
    text = path.read_text(encoding="utf-8")

    sections = {"Documentation": "", "Parameter Space": "", "Connection Patterns": ""}
    current_section = None
    node_name = None

    for line in text.splitlines():
        if line.startswith("# "):
            node_name = line.replace("#", "").strip()
        elif line.startswith("## "):
            header = line.replace("##", "").strip()
            if header in sections:
                current_section = header
            else:
                current_section = None
        elif current_section:
            sections[current_section] += line + "\n"

    return {"node_name": node_name or path.stem, **sections}


def load_pcg_docs_by_section() -> Dict[str, List[Document]]:
    """
    Read all node documents and classify them into Document lists by section.
    Returns:
        {
          "Documentation": [...],
          "Parameter Space": [...],
          "Connection Patterns": [...],
        }
    """
    grouped_docs = {"Documentation": [], "Parameter Space": [], "Connection Patterns": []}

    for md_file in PCG_DOC_PATH.glob("*.md"):
        data = extract_sections_from_markdown(md_file)
        node_name = data["node_name"]

        for section_name in grouped_docs.keys():
            content = data.get(section_name, "").strip()
            if content:  # Only create Document if there's content
                doc = Document(
                    page_content=node_name + " " + section_name + ":\n " + content,
                    metadata={
                        "node_name": node_name,
                        "section": section_name,
                        "source_file": md_file.name
                    }
                )
                grouped_docs[section_name].append(doc)

    for k, v in grouped_docs.items():
        print(f"{k}: {len(v)} node document blocks")
    return grouped_docs


# ============================================================
# 3. Vector Store Construction and Loading
# ============================================================

def build_vectorstore_for_section(section_name: str, docs: List[Document], embedding):
    """Build or load the corresponding vector index for a single section."""
    vector_path = f"{VECTOR_INDEX_BASE}_{section_name.replace(' ', '')}"

    # Skip if the section has no documents
    if not docs:
        print(f"Skipping {section_name}: No documents to index.")
        return None

    if not os.path.exists(vector_path):
        print(f"Building {section_name} vector index...")
        vector_store = FAISS.from_documents(docs, embedding)
        vector_store.save_local(vector_path)
        print(f"{section_name} vector index built, indexed {len(docs)} nodes.")
    else:
        print(f" Loading existing {section_name} index...")
        vector_store = FAISS.load_local(
            vector_path,
            embedding,
            allow_dangerous_deserialization=True
        )
        print(f"{section_name} index loaded.")

    return vector_store


def build_all_vectorstores() -> Dict[str, FAISS]:
    """Build separate vector stores for the three sections."""
    embedding = get_embedding()
    grouped_docs = load_pcg_docs_by_section()

    stores = {}
    for section_name, docs in grouped_docs.items():
        store = build_vectorstore_for_section(section_name, docs, embedding)
        if store:
            stores[section_name] = store
    return stores


# ============================================================
# 4. Query Interface (functions exposed to the user)
# ============================================================

def query_vectorstore(vector_store: FAISS, node_name: str, query: str, top_k: int = 5) -> List[Dict[str, Any]]:
    """
    General query function.
    Arguments:
        vector_store: FAISS vector store object
        node_name: Node name (for filtering results)
        query: Query text
        top_k: Number of top matching results to return
    Returns:
        List[dict] = [
            {
                "node_name": str,
                "section": str,
                "source_file": str,
                "content": str,
                "score": float (optional)
            },
            ...
        ]
    """
    if not vector_store:
        return []

    results = vector_store.similarity_search(query, k=top_k)

    filtered = []

    for doc in results:
        if doc.metadata.get("node_name") == node_name:
            filtered.append({
                "node_name": doc.metadata.get("node_name"),
                "section": doc.metadata.get("section"),
                "source_file": doc.metadata.get("source_file"),
                "content": doc.page_content.strip(),
            })

    return filtered


def ensure_vectorstore_exists(section_name: str) -> FAISS:
    """
    If the vector store for the current section doesn't exist, 
    it will automatically call build_all_vectorstores() to build it;
    otherwise, it will load and return the existing one.
    """
    embedding = get_embedding()
    vector_path = f"{VECTOR_INDEX_BASE}_{section_name.replace(' ', '')}"

    if not os.path.exists(vector_path):
        print(f"{section_name} vector store not found, automatically building all indexes...")
        stores = build_all_vectorstores()
        return stores.get(section_name)
    else:
        return FAISS.load_local(vector_path, embedding, allow_dangerous_deserialization=True)


# ============================================================
# 5. Specialized Query Wrappers
# ============================================================

def query_documentation(node_name: str, query: str, top_k: int = 5) -> List[Dict[str, Any]]:
    """Query the Documentation section"""
    store = ensure_vectorstore_exists("Documentation")
    return query_vectorstore(store, node_name, query, top_k)


def query_parameter_space(node_name: str, query: str, top_k: int = 5) -> List[Dict[str, Any]]:
    """Query the Parameter Space section"""
    store = ensure_vectorstore_exists("Parameter Space")
    return query_vectorstore(store, node_name, query, top_k)


def query_connection_patterns(node_name: str, query: str, top_k: int = 1) -> List[Dict[str, Any]]:
    """Query the Connection Patterns section"""
    store = ensure_vectorstore_exists("Connection Patterns")
    return query_vectorstore(store, node_name, query, top_k)


# ============================================================
# Debugging Example
# ============================================================
if __name__ == "__main__":
    print(query_parameter_space("PCGSelectRandomPointsSettings", "PCGSelectRandomPointsSettings"))