#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
scanner_source="$repo_root/src/utils/sessionscanner.cpp"

python3 - "$scanner_source" <<'PY'
import pathlib
import re
import sys

text = pathlib.Path(sys.argv[1]).read_text()

patterns = [
    (
        r'payloadType == "exec_command_end"[\s\S]{0,240}?'
        r'return eventIsFresh \? SessionStatus::Working : SessionStatus::Ready;',
        "Codex local status inference must treat fresh exec_command_end as mid-turn Working."
    ),
    (
        r'payloadType == "function_call_output"[\s\S]{0,240}?'
        r'return eventIsFresh \? SessionStatus::Working : SessionStatus::Ready;',
        "Codex local status inference must treat fresh function_call_output as mid-turn Working."
    ),
    (
        r'payloadType == "patch_apply_end"[\s\S]{0,240}?'
        r'return eventIsFresh \? SessionStatus::Working : SessionStatus::Ready;',
        "Codex local status inference must treat fresh patch_apply_end as mid-turn Working."
    ),
]

for pattern, message in patterns:
    if not re.search(pattern, text):
        print(f"FAIL: {message}", file=sys.stderr)
        raise SystemExit(1)

print("PASS: task-002 Codex mid-turn guard rails are wired.")
PY
