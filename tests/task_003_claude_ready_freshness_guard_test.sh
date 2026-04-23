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

require_pattern 'const[[:space:]]+double[[:space:]]+claudeReadyFreshSeconds[[:space:]]*=' "$scanner_source" \
    "Claude artifact status inference must define a freshness window for Ready-like terminal events."
require_pattern 'return[[:space:]]+readyStateIsFresh[[:space:]]*\?[[:space:]]*SessionStatus::Ready[[:space:]]*:[[:space:]]*SessionStatus::Unknown;' "$scanner_source" \
    "Claude stale Ready-like artifact events must decay to Unknown so live CPU detection can still surface Working."

echo "PASS: task-003 Claude ready freshness guard is present."
