from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from core.runtime_validation import run_runtime_validation


def main() -> int:
    parser = argparse.ArgumentParser(description="Run Phase3 Python runtime validation harness for one AutoUE demo output root.")
    parser.add_argument("--root", required=True, help="Demo output root, e.g. data/output-real-008/demo_1")
    parser.add_argument("--write-summary", action="store_true", help="Write runtime/runtime-summary.json and runtime/runtime-log.jsonl")
    args = parser.parse_args()

    summary = run_runtime_validation(args.root, write_outputs=args.write_summary)
    print(json.dumps(summary, ensure_ascii=False, indent=2))
    return 1 if summary.get("result") != "pass" else 0


if __name__ == "__main__":
    raise SystemExit(main())
