#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

python3 - "$ROOT_DIR" <<'PY'
import pathlib
import re
import sys

root = pathlib.Path(sys.argv[1])
pattern = re.compile(r"font-size\s*:\s*(\d+)px")
violations = []
allowed_snippets = [
    'padding: 2px 10px; min-width: 80px; min-height: 24px;',
    'padding: 2px 8px; } QPushButton:hover',
    'Running Tasks',
]

for path in root.joinpath("src").rglob("*"):
    if not path.is_file():
        continue
    if path.suffix not in {".cpp", ".h"}:
        continue
    for lineno, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        for match in pattern.finditer(line):
            if any(snippet in line for snippet in allowed_snippets):
                continue
            if int(match.group(1)) < 14:
                violations.append(f"{path.relative_to(root)}:{lineno}:{match.group(0)}")

if violations:
    print("FAIL: found font sizes below 14px:")
    for item in violations:
        print(item)
    sys.exit(1)

print("PASS: all explicit font sizes are 14px or above.")
PY
