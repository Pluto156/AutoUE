from __future__ import annotations

import json
import os
from copy import deepcopy
from pathlib import Path
from typing import Any, Dict, Iterable, Mapping, Optional

PROJECT_ROOT = Path(__file__).resolve().parents[1]


def _read_json(path: Path) -> Dict[str, Any]:
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)


def _deep_merge(base: Dict[str, Any], override: Mapping[str, Any]) -> Dict[str, Any]:
    result = deepcopy(base)
    for key, value in override.items():
        if isinstance(value, Mapping) and isinstance(result.get(key), Mapping):
            result[key] = _deep_merge(dict(result[key]), value)
        else:
            result[key] = deepcopy(value)
    return result


def repo_path(value: str | os.PathLike[str], *, base: Path = PROJECT_ROOT) -> Path:
    p = Path(value)
    return p if p.is_absolute() else (base / p).resolve()


def load_runtime_config(config_path: Optional[str] = None) -> Dict[str, Any]:
    config = _read_json(PROJECT_ROOT / "config" / "local.example.json")
    local_path = PROJECT_ROOT / "config" / "local.json"
    if local_path.exists():
        config = _deep_merge(config, _read_json(local_path))
    env_config = os.getenv("AUTOUE_CONFIG")
    if env_config:
        env_path = repo_path(env_config)
        if env_path.exists():
            config = _deep_merge(config, _read_json(env_path))
    if config_path:
        config = _deep_merge(config, _read_json(repo_path(config_path)))
    return config


def load_llm_profiles() -> Dict[str, Any]:
    config = _read_json(PROJECT_ROOT / "config" / "llm-profiles.example.json")
    local_path = PROJECT_ROOT / "config" / "llm-profiles.local.json"
    if local_path.exists():
        config = _deep_merge(config, _read_json(local_path))
    return config


def load_workflow_config(workflow_path: Optional[str], runtime_config: Mapping[str, Any]) -> Dict[str, Any]:
    selected = workflow_path or os.getenv("AUTOUE_WORKFLOW") or runtime_config.get("workflow") or "config/workflows/default.json"
    return _read_json(repo_path(str(selected)))


def resolve_configured_path(runtime_config: Mapping[str, Any], key: str) -> Path:
    paths = runtime_config.get("paths", {})
    if key not in paths:
        raise KeyError(f"missing paths.{key} in runtime config")
    return repo_path(paths[key])


def resolve_copy_dirs(runtime_config: Mapping[str, Any]) -> list[Path]:
    raw = runtime_config.get("paths", {}).get("copy_dirs", [])
    if isinstance(raw, str):
        raw = [x for x in raw.split(";") if x.strip()]
    return [repo_path(x) for x in raw]


def iter_enabled_nodes(workflow_config: Mapping[str, Any]) -> Iterable[Dict[str, Any]]:
    for node in workflow_config.get("nodes", []):
        if node.get("enabled", True):
            yield dict(node)
