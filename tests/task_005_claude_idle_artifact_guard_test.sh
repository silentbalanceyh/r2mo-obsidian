#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
scanner_source="$repo_root/src/utils/sessionscanner.cpp"

require_pattern() {
    local pattern="$1"
    local file="$2"
    local message="$3"

    if ! grep -Eq "$pattern" "$file"; then
        echo "FAIL: $message" >&2
        exit 1
    fi
}

require_pattern 'if[[:space:]]*\([[:space:]]*toolName[[:space:]]*==[[:space:]]*"Claude"[[:space:]]*&&[[:space:]]*!sessionPath\.isEmpty\(\)[[:space:]]*\)' "$scanner_source" \
    "Claude sessions with a matched transcript must not fall back to CPU heuristics after artifact inference is non-working."
require_pattern 'return[[:space:]]+SessionStatus::Ready;[[:space:]]*//[[:space:]]*Claude transcript matched but no fresh active turn' "$scanner_source" \
    "Claude matched transcripts with no fresh active turn must resolve to Ready instead of phantom Working."

echo "PASS: task-005 Claude idle artifact guard is wired."
