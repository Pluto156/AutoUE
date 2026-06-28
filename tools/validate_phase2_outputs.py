from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from core.phase2_validation import BANNED_OUTPUT_MARKERS, PHASE2_NODE_ORDER, Phase2ValidationError, is_safe_relative_path, validate_phase2_output_set

CXX_SUFFIXES = {'.h', '.cpp'}
CXX_FILE_RE = re.compile(r'(?i)\.(?:h|cpp)\b')


def banned(label: str, text: str, errors: list[str]) -> None:
    if CXX_FILE_RE.search(text):
        errors.append(f'{label} contains forbidden native filename marker')
    for marker in BANNED_OUTPUT_MARKERS:
        if marker in text:
            errors.append(f'{label} contains banned marker: {marker}')


def validate_ts(path: Path, errors: list[str]) -> None:
    text = path.read_text(encoding='utf-8', errors='replace')
    banned(f'generated TypeScript {path}', text, errors)
    if '```' in text:
        errors.append(f'generated TypeScript contains Markdown fences: {path}')
    if text.count('{') != text.count('}'):
        errors.append(f'generated TypeScript has unbalanced braces: {path}')
    if text.count('(') != text.count(')'):
        errors.append(f'generated TypeScript has unbalanced parentheses: {path}')
    if 'export ' not in text and 'class ' not in text and 'function ' not in text:
        errors.append(f'generated TypeScript lacks an obvious exported/function/class entry point: {path}')


def ensure_rel_file(root: Path, rel: str, label: str, errors: list[str]) -> Path | None:
    if not isinstance(rel, str) or not is_safe_relative_path(rel):
        errors.append(f'{label} unsafe path: {rel}')
        return None
    target = (root / rel.replace('\\', '/')).resolve()
    try:
        target.relative_to(root)
    except ValueError:
        errors.append(f'{label} path outside output root: {rel}')
        return None
    if not target.exists():
        errors.append(f'{label} missing file: {rel}')
        return None
    return target


def validate_declared_files(root: Path, data: dict, errors: list[str]) -> list[str]:
    emitted: list[str] = []
    for node in ['TypeScriptInteractiveObjectGenerator', 'TypeScriptCodeGenerator']:
        node_data = data.get(node, {}) if isinstance(data, dict) else {}
        for item in node_data.get('template_inputs', []) if isinstance(node_data, dict) else []:
            rel = item.get('path', '') if isinstance(item, dict) else ''
            target = ensure_rel_file(root, rel, f'{node} template output', errors)
            if target:
                emitted.append(rel)
    return emitted


def validate_mcp_artifacts(root: Path, data: dict, errors: list[str]) -> list[str]:
    artifacts: list[str] = []
    mcp = data.get('UEApiMCPFeasibilitySearcher', {}) if isinstance(data, dict) else {}
    for query in mcp.get('queries', []) if isinstance(mcp, dict) else []:
        for key in ['raw_path', 'adjudication_path']:
            rel = query.get(key, '') if isinstance(query, dict) else ''
            target = ensure_rel_file(root, rel, f'UEApiMCPFeasibilitySearcher {key}', errors)
            if target:
                artifacts.append(rel)
                try:
                    obj = json.loads(target.read_text(encoding='utf-8'))
                    if key == 'adjudication_path' and obj.get('verdict') != 'hit':
                        errors.append(f'adjudication verdict must be hit for Phase2 done: {rel}')
                except Exception as exc:
                    errors.append(f'{key} is not valid JSON: {rel}: {exc}')
    for rel in ['flow/03-thin-gameplay-flow.json', 'flow/04-ue-api-mcp/summary.json', 'flow/05-puerts-runtime-mapping.json']:
        target = ensure_rel_file(root, rel, 'Phase2 flow artifact', errors)
        if target:
            artifacts.append(rel)
    return artifacts


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument('--root', required=True)
    parser.add_argument('--write-report', action='store_true')
    args = parser.parse_args()
    root = Path(args.root).resolve()
    errors: list[str] = []
    evidence = {'root': str(root)}
    outputs: dict[str, str] = {}

    if not root.exists():
        errors.append(f'root does not exist: {root}')
    llm_dir = root / 'llm_outputs'
    if not llm_dir.exists():
        errors.append(f'missing llm_outputs directory: {llm_dir}')
    else:
        for node in PHASE2_NODE_ORDER:
            path = llm_dir / f'{node}.txt'
            if not path.exists():
                errors.append(f'missing LLM output: {path}')
                continue
            text = path.read_text(encoding='utf-8', errors='replace')
            if not text.strip():
                errors.append(f'empty LLM output: {path}')
                continue
            outputs[node] = text
            banned(f'LLM output {node}', text, errors)
        evidence['validated_llm_nodes'] = list(outputs)

    data: dict = {}
    if len(outputs) == len(PHASE2_NODE_ORDER):
        try:
            result = validate_phase2_output_set(outputs)
            data = result['data_by_node']
            evidence.update(result['evidence'])
        except Phase2ValidationError as exc:
            errors.append(str(exc))

    ts_files = sorted(path for path in root.rglob('*.ts') if path.is_file()) if root.exists() else []
    if not ts_files:
        errors.append('no generated .ts files found under output root')
    for path in ts_files:
        validate_ts(path, errors)
    evidence['generated_ts_files'] = [str(path.relative_to(root)) for path in ts_files]
    if data:
        evidence['declared_ts_files'] = validate_declared_files(root, data, errors)
        evidence['mcp_artifacts'] = validate_mcp_artifacts(root, data, errors)

    native_files = sorted(path for path in root.rglob('*') if path.is_file() and path.suffix.lower() in CXX_SUFFIXES) if root.exists() else []
    if native_files:
        errors.append('Phase2 output must not contain native code files: ' + ', '.join(str(path.relative_to(root)) for path in native_files))

    instructions = root / 'MyPCG' / 'eval' / 'instructions.json'
    if not instructions.exists():
        errors.append(f'missing instructions.json: {instructions}')
    else:
        try:
            text = instructions.read_text(encoding='utf-8')
            banned('instructions.json', text, errors)
            obj = json.loads(text)
            if not obj.get('evaluation_instructions'):
                errors.append('instructions.json has no evaluation_instructions')
            else:
                evidence['instruction_count'] = len(obj['evaluation_instructions'])
        except Exception as exc:
            errors.append(f'instructions.json is not valid JSON: {exc}')

    report = {'result': 'fail' if errors else 'pass', 'errors': errors, 'evidence': evidence}
    if args.write_report and root.exists():
        (root / 'phase2_validation_report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False, indent=2))
    return 1 if errors else 0

if __name__ == '__main__':
    raise SystemExit(main())
