#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
local_scanner_source="$repo_root/src/utils/sessionscanner.cpp"
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

require_pattern 'sortedIndexes' "$local_scanner_source" \
    "OpenCode local session assignment should sort live rows before pairing them with candidate session IDs."
require_pattern 'assignCount[[:space:]]*=[[:space:]]*qMin\(sortedIndexes\.size\(\),[[:space:]]*candidateIds\.size\(\)\)' "$local_scanner_source" \
    "OpenCode local session assignment should pair as many rows as possible instead of requiring a single session."
forbid_pattern 'indexes\.size\(\)[[:space:]]*!=[[:space:]]*1[[:space:]]*\|\|[[:space:]]*candidateIds\.size\(\)[[:space:]]*!=[[:space:]]*1' "$local_scanner_source" \
    "OpenCode local session assignment must no longer bail out when a project has multiple candidate sessions."
require_pattern 'isBetterRemoteSessionCandidate' "$remote_scanner_source" \
    "Remote session scanner should use explicit candidate ranking when duplicate rows share one session."
require_pattern 'detailText\.contains\("/vendor/"\)' "$remote_scanner_source" \
    "Remote Codex dedupe should prefer the real vendor executable over the outer node wrapper."
require_pattern 'groupedSessions' "$remote_scanner_source" \
    "Remote session scanning should group duplicate rows before materializing monitor entries."

echo "PASS: task-002 OpenCode session guard rails are wired."
