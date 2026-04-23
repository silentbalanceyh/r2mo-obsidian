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

forbid_pattern 'if not found_opencode_row:' "$remote_scanner_source" \
    "Remote OpenCode probe must not synthesize artifact-only rows once live-process gating is enforced."
forbid_pattern 'if artifact and found_any_row:' "$remote_scanner_source" \
    "Remote OpenCode artifact fallback must be removed instead of conditionally creating extra rows."
forbid_pattern "detailText': 'OpenCode live session artifact'" "$remote_scanner_source" \
    "Remote monitor must not synthesize standalone OpenCode artifact rows."

echo "PASS: task-002 remote OpenCode artifact fallback guard is present."
