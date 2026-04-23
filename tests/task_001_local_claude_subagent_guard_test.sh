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

require_pattern 'QDirIterator[[:space:]]+it\(' "$scanner_source" \
    "Local Claude artifact collection must walk nested artifact directories."
require_pattern 'QDirIterator::Subdirectories' "$scanner_source" \
    "Local Claude artifact collection must include subagents/ artifacts recursively."
require_pattern 'subagents' "$scanner_source" \
    "Local Claude session assignment must account for nested subagent transcripts."

echo "PASS: local Claude subagent artifact guard is present."
