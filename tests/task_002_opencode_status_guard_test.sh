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

require_pattern 'if[[:space:]]*\([[:space:]]*toolName[[:space:]]*==[[:space:]]*"OpenCode"[[:space:]]*\)' "$scanner_source" \
    "OpenCode status detection should have a dedicated fallback branch."
require_pattern 'artifactStatus[[:space:]]*==[[:space:]]*SessionStatus::Unknown' "$scanner_source" \
    "OpenCode fallback branch should explicitly recognize missing artifact/global activity."
require_pattern 'return[[:space:]]+SessionStatus::Ready;' "$scanner_source" \
    "OpenCode without fresh artifact/global activity should fall back to Ready instead of CPU-noise Working."

echo "PASS: OpenCode status guard rails are wired."
