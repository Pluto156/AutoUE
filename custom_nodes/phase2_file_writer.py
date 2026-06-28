from __future__ import annotations

import json
import os
import re
from pathlib import Path
from typing import Any

from core.BaseLLMNode import GraphState
from core.phase2_validation import parse_phase2_json, validate_phase2_output

REPO_ROOT = Path(__file__).resolve().parents[1]
TEMPLATE_ROOT = REPO_ROOT / "templates" / "typescript"
PLACEHOLDER_RE = re.compile(r"{{\s*([A-Za-z0-9_]+)\s*}}")


def output_root(state: GraphState) -> Path:
    save_dir = getattr(state, "save_dir", "") or os.getenv("AUTOUE_TS_OUTPUT_DIR", "")
    if not save_dir:
        raise RuntimeError("state.save_dir/AUTOUE_TS_OUTPUT_DIR is required for Phase2 file emission")
    root = Path(save_dir).resolve()
    root.mkdir(parents=True, exist_ok=True)
    return root


def safe_target(root: Path, rel_text: str) -> Path:
    target = (root / Path(rel_text.replace("\\", "/"))).resolve()
    try:
        target.relative_to(root)
    except ValueError as exc:
        raise RuntimeError(f"generated path escapes output dir: {rel_text}") from exc
    return target


def _template_path(template_name: str) -> Path:
    path = (TEMPLATE_ROOT / f"{template_name}.ts.tmpl").resolve()
    try:
        path.relative_to(TEMPLATE_ROOT.resolve())
    except ValueError as exc:
        raise RuntimeError(f"template path escapes template root: {template_name}") from exc
    if not path.exists():
        raise RuntimeError(f"missing TypeScript template: {path}")
    return path


def _render(template_text: str, values: dict[str, Any]) -> str:
    flat: dict[str, str] = {}
    for key, value in values.items():
        if isinstance(value, str):
            flat[key] = value
            flat[f"{key}_json"] = json.dumps(value, ensure_ascii=False)
        elif value is None:
            flat[key] = ""
            flat[f"{key}_json"] = "null"
        else:
            flat[key] = str(value)
            flat[f"{key}_json"] = json.dumps(value, ensure_ascii=False)

    def replace(match: re.Match[str]) -> str:
        name = match.group(1)
        if name not in flat:
            raise RuntimeError(f"template placeholder has no value: {name}")
        return flat[name]

    rendered = PLACEHOLDER_RE.sub(replace, template_text)
    if PLACEHOLDER_RE.search(rendered):
        raise RuntimeError("unresolved TypeScript template placeholders remain")
    return rendered


def render_template_input(item: dict[str, Any]) -> str:
    template_name = str(item.get("template", ""))
    template_text = _template_path(template_name).read_text(encoding="utf-8")
    return _render(template_text, item)


def write_files_from_output(state: GraphState, node_name: str, output: str) -> list[Path]:
    data: dict[str, Any] = parse_phase2_json(node_name, validate_phase2_output(node_name, output))
    root = output_root(state)
    written: list[Path] = []
    for item in data.get("template_inputs", []):
        target = safe_target(root, item["path"])
        content = render_template_input(item)
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(content, encoding="utf-8")
        written.append(target)
        print(f"[DEBUG] {node_name} rendered TypeScript template {item['template']} -> {target}")
    if not written:
        raise RuntimeError(f"{node_name} produced no template_inputs to write")
    return written
