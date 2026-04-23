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
    "Remote OpenCode rows must not be synthesized from stale artifacts once no real opencode process is present."
require_pattern 'arg\(vault\.path,[[:space:]]*session\.toolName,[[:space:]]*QString::number\(session\.processPid\),[[:space:]]*dedupeSessionId\)' "$remote_scanner_source" \
    "Remote dedupe key must include processPid so multiple live Claude processes do not collapse into one row."

echo "PASS: remote dedupe guard rails are wired."
