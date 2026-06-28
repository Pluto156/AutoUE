from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from core.phase2_validation import PHASE2_NODE_ORDER, Phase2ValidationError, validate_phase2_output_set


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument('--root', required=True)
    args = parser.parse_args()
    root = Path(args.root).resolve()
    errors: list[str] = []
    required = [
        root / 'MyPCG' / 'eval' / 'instructions.json',
        root / 'MyPCG' / 'eval' / 'Prompt.txt',
        root / 'TypeScript' / 'content' / 'generated' / 'interactive' / 'SmokeInteractable.ts',
        root / 'TypeScript' / 'content' / 'generated' / 'SmokeGame.ts',
        root / 'flow' / '03-thin-gameplay-flow.json',
        root / 'flow' / '04-ue-api-mcp' / 'raw' / 'input.action_binding.raw.json',
        root / 'flow' / '04-ue-api-mcp' / 'raw' / 'damage.apply.raw.json',
        root / 'flow' / '04-ue-api-mcp' / 'adjudication' / 'input.action_binding.json',
        root / 'flow' / '04-ue-api-mcp' / 'adjudication' / 'damage.apply.json',
        root / 'flow' / '04-ue-api-mcp' / 'summary.json',
        root / 'flow' / '05-puerts-runtime-mapping.json',
    ] + [root / 'llm_outputs' / f'{node}.txt' for node in PHASE2_NODE_ORDER]
    for path in required:
        if not path.exists():
            errors.append(f'missing required smoke artifact: {path}')
    outputs = {}
    llm_dir = root / 'llm_outputs'
    if llm_dir.exists():
        for node in PHASE2_NODE_ORDER:
            path = llm_dir / f'{node}.txt'
            if path.exists():
                outputs[node] = path.read_text(encoding='utf-8')
    if len(outputs) == len(PHASE2_NODE_ORDER):
        try:
            validate_phase2_output_set(outputs)
        except Phase2ValidationError as exc:
            errors.append(str(exc))
    native_files = sorted(path for path in root.rglob('*') if path.is_file() and path.suffix.lower() in {'.h', '.cpp'}) if root.exists() else []
    if native_files:
        errors.append('Phase2 output must not contain native code files: ' + ', '.join(str(path.relative_to(root)) for path in native_files))
    if errors:
        print(json.dumps({'result': 'fail', 'errors': errors}, indent=2, ensure_ascii=False))
        return 1
    ts_files = sorted(path for path in root.rglob('*.ts') if path.is_file())
    print(json.dumps({'result': 'pass', 'root': str(root), 'generated_ts_files': [str(path.relative_to(root)) for path in ts_files]}, indent=2, ensure_ascii=False))
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
