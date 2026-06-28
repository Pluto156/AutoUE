from __future__ import annotations

import importlib
from pathlib import Path
from typing import Any, Mapping

from core.config import repo_path
from core.BaseLLMNode import BaseLLMNode


def read_prompt_text(txt_path: str | Path) -> str:
    path = repo_path(txt_path)
    raw = path.read_bytes()
    if raw.startswith(b"\xff\xfe"):
        text = raw.decode("utf-16-le")
    elif raw.startswith(b"\xfe\xff"):
        text = raw.decode("utf-16-be")
    else:
        text = raw.decode("utf-8", errors="ignore")
    return text.replace("\ufeff", "").strip()


def create_node_from_spec(spec: Mapping[str, Any]) -> BaseLLMNode:
    factory_path = spec.get("factory")
    if not factory_path or ":" not in factory_path:
        raise ValueError(f"Invalid node factory for {spec.get('name')}: {factory_path}")
    module_name, func_name = factory_path.split(":", 1)
    module = importlib.import_module(module_name)
    factory = getattr(module, func_name)
    node = factory()
    if not isinstance(node, BaseLLMNode):
        raise TypeError(f"Factory {factory_path} did not return BaseLLMNode")
    if "prompt_file" in spec and spec["prompt_file"]:
        node.change_prompt(read_prompt_text(spec["prompt_file"]))
    if "enable_feedback" in spec:
        node.enable_feedback = bool(spec["enable_feedback"])
    if "debug" in spec:
        node.isDebug = bool(spec["debug"])
    if spec.get("name") and node.name != spec["name"]:
        node.name = spec["name"]
    return node
