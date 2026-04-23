#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
scanner_source="$repo_root/src/utils/sessionscanner.cpp"
remote_scanner_source="$repo_root/src/utils/remotesessionscanner.cpp"

require_pattern() {
    local pattern="$1"
    local file="$2"
    local message="$3"

    if ! grep -Eq "$pattern" "$file"; then
        echo "FAIL: $message" >&2
        exit 1
    fi
}

require_pattern 'stopReason[[:space:]]*==[[:space:]]*"tool_use"' "$scanner_source" \
    "Claude local status inference must treat stop_reason=tool_use as Working."
require_pattern 'stopReason[[:space:]]*==[[:space:]]*"end_turn"' "$scanner_source" \
    "Claude local status inference must treat stop_reason=end_turn as Ready."
require_pattern 'payloadType[[:space:]]*==[[:space:]]*"task_started"|type[[:space:]]*==[[:space:]]*"task_started"' "$scanner_source" \
    "Codex local status inference must treat task_started as Working."
require_pattern 'payloadType[[:space:]]*==[[:space:]]*"task_complete"|type[[:space:]]*==[[:space:]]*"task_complete"' "$scanner_source" \
    "Codex local status inference must treat task_complete as Ready."
require_pattern 'turn-complete' "$scanner_source" \
    "OpenCode local status inference must inspect turn-complete notifications."
require_pattern 'turn-complete' "$remote_scanner_source" \
    "Remote status inference must inspect turn-complete notifications too."

echo "PASS: status semantics guard rails are wired."
