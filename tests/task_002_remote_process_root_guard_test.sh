#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
scanner_source="$repo_root/src/utils/remotesessionscanner.cpp"

require_pattern() {
    local pattern="$1"
    local file="$2"
    local message="$3"

    if ! grep -Eq "$pattern" "$file"; then
        echo "FAIL: $message" >&2
        exit 1
    fi
}

require_pattern 'def process_parent_pid\(pid\):' "$scanner_source" \
    "Remote probe must read parent PIDs so it can distinguish root sessions from helper processes."
require_pattern 'def has_tool_ancestor\(pid\):' "$scanner_source" \
    "Remote probe must detect AI helper processes that sit under another AI tool process."
require_pattern 'if has_tool_ancestor\(int\(pid\)\):' "$scanner_source" \
    "Remote probe must skip descendant helper processes once a root AI process exists."

echo "PASS: remote process-root guard is present."
