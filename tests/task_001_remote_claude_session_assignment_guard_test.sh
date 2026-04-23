#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
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

forbid_pattern() {
    local pattern="$1"
    local file="$2"
    local message="$3"

    if grep -Eq "$pattern" "$file"; then
        echo "FAIL: $message" >&2
        exit 1
    fi
}

require_pattern 'def collect_claude_artifacts\(project\):' "$remote_scanner_source" \
    "Remote Claude probe must collect all candidate Claude artifacts for a project."
require_pattern 'rows_by_project_tool' "$remote_scanner_source" \
    "Remote probe must group rows before assigning Claude session artifacts."
require_pattern 'if tool == '\''Claude'\'' and artifacts:' "$remote_scanner_source" \
    "Remote probe must assign Claude artifacts per live row instead of reusing one latest artifact."
forbid_pattern 'latest_claude_artifact\(cwd\)' "$remote_scanner_source" \
    "Remote Claude rows must not all bind to a single latest artifact anymore."

echo "PASS: remote Claude session assignment guard rails are wired."
