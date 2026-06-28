from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Any, Mapping

SUPPORTED_DRIVER = "adapter_call"
SUPPORTED_EXPECTED_TYPE = "static_trace_present"
SUPPORTED_STATIC_TRACE_KEYS = {
    "ability_module_export",
    "interactive_adapter_export",
    "engine_ports_mapped",
}
RUNTIME_DIR_NAME = "runtime"
SUMMARY_FILE_NAME = "runtime-summary.json"
LOG_FILE_NAME = "runtime-log.jsonl"


def runtime_validation_config(*, enabled: bool = False) -> dict[str, Any]:
    """Return the explicit Phase3 runtime harness contract used by dry-run output."""
    return {
        "enabled": bool(enabled),
        "runner": "tools/run_runtime_validation.py",
        "validator": "tools/validate_runtime_results.py",
        "summary_path": f"{RUNTIME_DIR_NAME}/{SUMMARY_FILE_NAME}",
        "log_path": f"{RUNTIME_DIR_NAME}/{LOG_FILE_NAME}",
        "supported_drivers": [SUPPORTED_DRIVER],
        "supported_expected_types": [SUPPORTED_EXPECTED_TYPE],
        "supported_static_trace_keys": sorted(SUPPORTED_STATIC_TRACE_KEYS),
        "launches_ue_editor": False,
        "launches_pie": False,
        "injects_player_input": False,
    }


def _normalise_rel(rel: Any) -> str:
    return str(rel).replace("\\", "/") if isinstance(rel, str) else ""


def _append_log(logs: list[dict[str, Any]], level: str, event: str, **fields: Any) -> None:
    record = {"level": level, "event": event}
    record.update({k: v for k, v in fields.items() if v is not None})
    logs.append(record)


def _add_error(summary: dict[str, Any], logs: list[dict[str, Any]], message: str, **fields: Any) -> None:
    summary["errors"].append(message)
    _append_log(logs, "error", "validation_error", message=message, **fields)


def _resolve_under_root(root: Path, rel: Any, label: str, summary: dict[str, Any], logs: list[dict[str, Any]], *, step_id: Any = None) -> Path | None:
    rel_s = _normalise_rel(rel)
    if not rel_s:
        _add_error(summary, logs, f"{label} missing relative path", step_id=step_id)
        return None
    rel_path = Path(rel_s)
    if rel_path.is_absolute() or ".." in rel_path.parts:
        _add_error(summary, logs, f"{label} unsafe path: {rel_s}", step_id=step_id)
        return None
    target = (root / rel_path).resolve()
    try:
        target.relative_to(root)
    except ValueError:
        _add_error(summary, logs, f"{label} path outside output root: {rel_s}", step_id=step_id)
        return None
    return target


def _require_file(root: Path, rel: Any, label: str, summary: dict[str, Any], logs: list[dict[str, Any]], *, step_id: Any = None) -> Path | None:
    target = _resolve_under_root(root, rel, label, summary, logs, step_id=step_id)
    rel_s = _normalise_rel(rel)
    if target is None:
        return None
    if not target.is_file():
        _add_error(summary, logs, f"{label} missing file: {rel_s}", step_id=step_id)
        return None
    _append_log(logs, "info", "file_present", label=label, path=rel_s, step_id=step_id)
    return target


def _load_json(path: Path, label: str, summary: dict[str, Any], logs: list[dict[str, Any]], *, step_id: Any = None) -> Any | None:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:  # noqa: BLE001 - fail-loud summary needs exact parse error
        _add_error(summary, logs, f"{label} is not valid JSON: {path}: {exc}", step_id=step_id)
        return None


def _read_text(path: Path, label: str, summary: dict[str, Any], logs: list[dict[str, Any]], *, step_id: Any = None) -> str:
    try:
        return path.read_text(encoding="utf-8", errors="replace")
    except Exception as exc:  # noqa: BLE001
        _add_error(summary, logs, f"{label} cannot be read: {path}: {exc}", step_id=step_id)
        return ""


def _has_export(text: str, export_name: str) -> bool:
    escaped = re.escape(export_name)
    patterns = [
        rf"\bexport\s+(?:default\s+)?(?:async\s+)?(?:function|class|const|let|var)\s+{escaped}\b",
        rf"\bexport\s*\{{[^}}]*\b{escaped}\b[^}}]*\}}",
    ]
    return any(re.search(pattern, text, flags=re.MULTILINE) for pattern in patterns)


def _find_mapping(runtime_mapping: Mapping[str, Any], trace: Mapping[str, Any]) -> Mapping[str, Any] | None:
    mappings = runtime_mapping.get("mappings", [])
    behavior_id = trace.get("behavior_id")
    flow_id = trace.get("flow_id")
    exact = [m for m in mappings if isinstance(m, Mapping) and m.get("behavior_id") == behavior_id and m.get("flow_id") == flow_id]
    if exact:
        return exact[0]
    by_behavior = [m for m in mappings if isinstance(m, Mapping) and m.get("behavior_id") == behavior_id]
    return by_behavior[0] if by_behavior else None


def _rel_list(value: Any) -> list[str]:
    return [_normalise_rel(item) for item in value] if isinstance(value, list) else []


def _validate_export(
    root: Path,
    summary: dict[str, Any],
    logs: list[dict[str, Any]],
    *,
    step_id: Any,
    export_name: Any,
    file_rel: Any,
    label: str,
) -> bool:
    if not isinstance(export_name, str) or not export_name.strip():
        _add_error(summary, logs, f"{label} expected_value must be a non-empty export name", step_id=step_id)
        return False
    path = _require_file(root, file_rel, label, summary, logs, step_id=step_id)
    if path is None:
        return False
    text = _read_text(path, label, summary, logs, step_id=step_id)
    if not _has_export(text, export_name):
        _add_error(summary, logs, f"{label} export not found: {export_name} in {_normalise_rel(file_rel)}", step_id=step_id)
        return False
    _append_log(logs, "info", "export_present", step_id=step_id, label=label, path=_normalise_rel(file_rel), export_name=export_name)
    return True


def _validate_engine_ports(
    summary: dict[str, Any],
    logs: list[dict[str, Any]],
    *,
    step_id: Any,
    expected_ports: Any,
    trace_ports: set[str],
    mapping_ports: dict[str, Mapping[str, Any]],
    trace_adjudications: set[str],
) -> bool:
    if not isinstance(expected_ports, list) or not all(isinstance(port, str) and port for port in expected_ports):
        _add_error(summary, logs, "engine_ports_mapped expected_value must be a non-empty string list", step_id=step_id)
        return False
    ok = True
    for port in expected_ports:
        if port not in trace_ports:
            _add_error(summary, logs, f"engine_ports_mapped missing from eval trace: {port}", step_id=step_id)
            ok = False
        mapping = mapping_ports.get(port)
        if mapping is None:
            _add_error(summary, logs, f"engine_ports_mapped missing from runtime mapping: {port}", step_id=step_id)
            ok = False
            continue
        if mapping.get("verdict") != "hit":
            _add_error(summary, logs, f"engine_ports_mapped runtime mapping is not hit: {port}", step_id=step_id)
            ok = False
        adjudication_path = _normalise_rel(mapping.get("adjudication_path"))
        if adjudication_path and adjudication_path not in trace_adjudications:
            _add_error(summary, logs, f"engine_ports_mapped adjudication not present in eval trace for {port}: {adjudication_path}", step_id=step_id)
            ok = False
    if ok:
        _append_log(logs, "info", "engine_ports_mapped", step_id=step_id, engine_port_ids=list(expected_ports))
    return ok


def collect_runtime_validation(root: str | Path) -> tuple[dict[str, Any], list[dict[str, Any]]]:
    root_path = Path(root).resolve()
    logs: list[dict[str, Any]] = []
    summary: dict[str, Any] = {
        "result": "fail",
        "root": str(root_path),
        "contract": runtime_validation_config(enabled=True),
        "instruction_count": 0,
        "steps": [],
        "errors": [],
        "evidence": {
            "runtime_mapping_paths": [],
            "adjudication_paths": [],
            "ts_files": [],
            "engine_port_ids": [],
        },
        "notes": [
            "Phase3 runtime validation is a Python static adapter_call harness only.",
            "It does not launch UE Editor, does not launch PIE, and does not inject real player input.",
        ],
    }
    _append_log(logs, "info", "runtime_validation_started", root=str(root_path))

    if not root_path.exists():
        _add_error(summary, logs, f"root does not exist: {root_path}")
        return _finalize(summary, logs)

    instructions_path = root_path / "MyPCG" / "eval" / "instructions.json"
    if not instructions_path.is_file():
        _add_error(summary, logs, f"missing instructions.json: {instructions_path}")
        return _finalize(summary, logs)

    instructions_obj = _load_json(instructions_path, "instructions.json", summary, logs)
    if not isinstance(instructions_obj, Mapping):
        _add_error(summary, logs, "instructions.json root must be an object")
        return _finalize(summary, logs)
    instructions = instructions_obj.get("evaluation_instructions")
    if not isinstance(instructions, list) or not instructions:
        _add_error(summary, logs, "instructions.json has no evaluation_instructions")
        return _finalize(summary, logs)

    summary["instruction_count"] = len(instructions)
    mapping_cache: dict[str, Any] = {}

    for index, instruction in enumerate(instructions):
        if not isinstance(instruction, Mapping):
            _add_error(summary, logs, f"evaluation_instructions[{index}] must be object")
            continue
        _validate_instruction(root_path, instruction, summary, logs, mapping_cache, fallback_step_id=index + 1)

    return _finalize(summary, logs)


def _validate_instruction(
    root: Path,
    instruction: Mapping[str, Any],
    summary: dict[str, Any],
    logs: list[dict[str, Any]],
    mapping_cache: dict[str, Any],
    *,
    fallback_step_id: int,
) -> None:
    step_id = instruction.get("step_id", fallback_step_id)
    step_record: dict[str, Any] = {
        "step_id": step_id,
        "action": instruction.get("action"),
        "target": instruction.get("target"),
        "driver": instruction.get("driver"),
        "status": "fail",
        "checks": [],
    }
    summary["steps"].append(step_record)
    before_errors = len(summary["errors"])

    driver = instruction.get("driver")
    if driver != SUPPORTED_DRIVER:
        _add_error(summary, logs, f"unknown runtime validation driver: {driver}", step_id=step_id)

    trace = instruction.get("trace")
    if not isinstance(trace, Mapping):
        _add_error(summary, logs, f"instruction {step_id} missing trace object", step_id=step_id)
        return

    runtime_mapping_rel = _normalise_rel(trace.get("runtime_mapping_path"))
    runtime_mapping_path = _require_file(root, runtime_mapping_rel, "runtime_mapping_path", summary, logs, step_id=step_id)
    runtime_mapping: Mapping[str, Any] = {}
    if runtime_mapping_path is not None:
        if runtime_mapping_rel not in mapping_cache:
            mapping_cache[runtime_mapping_rel] = _load_json(runtime_mapping_path, "runtime_mapping_path", summary, logs, step_id=step_id)
        loaded = mapping_cache.get(runtime_mapping_rel)
        if isinstance(loaded, Mapping):
            runtime_mapping = loaded
            if loaded.get("runtime_mapping_path") != runtime_mapping_rel:
                _add_error(summary, logs, f"runtime mapping self path mismatch: expected {runtime_mapping_rel}, got {loaded.get('runtime_mapping_path')}", step_id=step_id)
            _record_evidence(summary, "runtime_mapping_paths", runtime_mapping_rel)
        else:
            _add_error(summary, logs, f"runtime_mapping_path must contain a JSON object: {runtime_mapping_rel}", step_id=step_id)

    trace_ports = set(_rel_list(trace.get("engine_port_ids")))
    if not trace_ports:
        _add_error(summary, logs, f"instruction {step_id} trace.engine_port_ids must be non-empty", step_id=step_id)
    for port in sorted(trace_ports):
        _record_evidence(summary, "engine_port_ids", port)

    trace_adjudications = set(_rel_list(trace.get("adjudication_paths")))
    if not trace_adjudications:
        _add_error(summary, logs, f"instruction {step_id} trace.adjudication_paths must be non-empty", step_id=step_id)
    for rel in sorted(trace_adjudications):
        if _require_file(root, rel, "adjudication_paths", summary, logs, step_id=step_id):
            _record_evidence(summary, "adjudication_paths", rel)

    trace_ts_files = _rel_list(trace.get("ts_files"))
    if not trace_ts_files:
        _add_error(summary, logs, f"instruction {step_id} trace.ts_files must be non-empty", step_id=step_id)
    ts_file_set = set(trace_ts_files)
    for rel in trace_ts_files:
        if _require_file(root, rel, "ts_files", summary, logs, step_id=step_id):
            _record_evidence(summary, "ts_files", rel)

    mapping_entry = _find_mapping(runtime_mapping, trace) if runtime_mapping else None
    if mapping_entry is None:
        _add_error(summary, logs, f"runtime mapping missing behavior trace: behavior_id={trace.get('behavior_id')} flow_id={trace.get('flow_id')}", step_id=step_id)
        mapping_ports: dict[str, Mapping[str, Any]] = {}
    else:
        mapping_ports = {
            port.get("engine_port_id", ""): port
            for port in mapping_entry.get("engine_port_mappings", [])
            if isinstance(port, Mapping) and port.get("engine_port_id")
        }
        runtime_owner = _normalise_rel(mapping_entry.get("runtime_owner"))
        if runtime_owner and runtime_owner not in ts_file_set:
            _add_error(summary, logs, f"runtime_owner is not listed in eval trace ts_files: {runtime_owner}", step_id=step_id)

    for port, port_mapping in mapping_ports.items():
        adjudication_path = _normalise_rel(port_mapping.get("adjudication_path"))
        if adjudication_path:
            if _require_file(root, adjudication_path, "runtime mapping adjudication_path", summary, logs, step_id=step_id):
                _record_evidence(summary, "adjudication_paths", adjudication_path)

    expected = instruction.get("expected")
    if not isinstance(expected, list) or not expected:
        _add_error(summary, logs, f"instruction {step_id} expected must be non-empty list", step_id=step_id)
        expected = []

    for expected_index, item in enumerate(expected):
        if not isinstance(item, Mapping):
            _add_error(summary, logs, f"instruction {step_id} expected[{expected_index}] must be object", step_id=step_id)
            continue
        expected_type = item.get("type")
        if expected_type != SUPPORTED_EXPECTED_TYPE:
            _add_error(summary, logs, f"unknown runtime expected type: {expected_type}", step_id=step_id)
            continue
        key = item.get("key")
        if key not in SUPPORTED_STATIC_TRACE_KEYS:
            _add_error(summary, logs, f"unknown static_trace_present key: {key}", step_id=step_id)
            continue
        if key == "ability_module_export":
            ability_rel = _normalise_rel(mapping_entry.get("runtime_owner")) if mapping_entry else ""
            if not ability_rel:
                ability_rel = next((rel for rel in trace_ts_files if "/interactive/" not in rel.lower()), "")
            ok = _validate_export(root, summary, logs, step_id=step_id, export_name=item.get("expected_value"), file_rel=ability_rel, label="ability_module_export")
        elif key == "interactive_adapter_export":
            interactive_rels = [rel for rel in trace_ts_files if "/interactive/" in rel.lower()]
            if not interactive_rels:
                _add_error(summary, logs, "interactive_adapter_export has no interactive ts file in trace.ts_files", step_id=step_id)
                ok = False
            else:
                ok = any(
                    _validate_export(root, summary, logs, step_id=step_id, export_name=item.get("expected_value"), file_rel=rel, label="interactive_adapter_export")
                    for rel in interactive_rels
                )
        elif key == "engine_ports_mapped":
            ok = _validate_engine_ports(
                summary,
                logs,
                step_id=step_id,
                expected_ports=item.get("expected_value"),
                trace_ports=trace_ports,
                mapping_ports=mapping_ports,
                trace_adjudications=trace_adjudications,
            )
        else:
            ok = False
        step_record["checks"].append({"type": expected_type, "key": key, "passed": bool(ok)})

    step_record["status"] = "pass" if len(summary["errors"]) == before_errors else "fail"
    _append_log(logs, "info", "instruction_validated", step_id=step_id, status=step_record["status"])


def _record_evidence(summary: dict[str, Any], key: str, value: str) -> None:
    bucket = summary["evidence"].setdefault(key, [])
    if value and value not in bucket:
        bucket.append(value)


def _finalize(summary: dict[str, Any], logs: list[dict[str, Any]]) -> tuple[dict[str, Any], list[dict[str, Any]]]:
    for values in summary.get("evidence", {}).values():
        if isinstance(values, list):
            values.sort()
    summary["result"] = "fail" if summary["errors"] else "pass"
    _append_log(logs, "info", "runtime_validation_finished", result=summary["result"], error_count=len(summary["errors"]))
    return summary, logs


def write_runtime_validation_outputs(root: str | Path, summary: Mapping[str, Any], logs: list[Mapping[str, Any]]) -> tuple[Path, Path]:
    root_path = Path(root).resolve()
    runtime_dir = root_path / RUNTIME_DIR_NAME
    runtime_dir.mkdir(parents=True, exist_ok=True)
    summary_path = runtime_dir / SUMMARY_FILE_NAME
    log_path = runtime_dir / LOG_FILE_NAME
    summary_path.write_text(json.dumps(summary, ensure_ascii=False, indent=2), encoding="utf-8")
    log_path.write_text("".join(json.dumps(record, ensure_ascii=False) + "\n" for record in logs), encoding="utf-8")
    return summary_path, log_path


def run_runtime_validation(root: str | Path, *, write_outputs: bool = False) -> dict[str, Any]:
    summary, logs = collect_runtime_validation(root)
    if write_outputs:
        summary_path, log_path = write_runtime_validation_outputs(root, summary, logs)
        summary["summary_path"] = str(summary_path)
        summary["log_path"] = str(log_path)
        summary_path.write_text(json.dumps(summary, ensure_ascii=False, indent=2), encoding="utf-8")
    return summary


def validate_runtime_summary(root: str | Path) -> dict[str, Any]:
    root_path = Path(root).resolve()
    summary_path = root_path / RUNTIME_DIR_NAME / SUMMARY_FILE_NAME
    errors: list[str] = []
    evidence: dict[str, Any] = {"summary_path": str(summary_path)}
    if not summary_path.is_file():
        errors.append(f"missing runtime summary: {summary_path}")
    else:
        try:
            summary = json.loads(summary_path.read_text(encoding="utf-8"))
            evidence["runtime_result"] = summary.get("result")
            evidence["instruction_count"] = summary.get("instruction_count")
            if summary.get("result") != "pass":
                errors.append("runtime validation result is not pass")
                for err in summary.get("errors", []):
                    errors.append(str(err))
        except Exception as exc:  # noqa: BLE001
            errors.append(f"runtime summary is not valid JSON: {exc}")
    return {"result": "fail" if errors else "pass", "errors": errors, "evidence": evidence}
