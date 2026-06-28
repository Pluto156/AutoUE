from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from core.runtime_validation import validate_runtime_summary


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate a saved Phase3 runtime-summary.json.")
    parser.add_argument("--root", required=True, help="Demo output root containing runtime/runtime-summary.json")
    args = parser.parse_args()

    result = validate_runtime_summary(args.root)
    print(json.dumps(result, ensure_ascii=False, indent=2))
    return 1 if result.get("result") != "pass" else 0


if __name__ == "__main__":
    raise SystemExit(main())
