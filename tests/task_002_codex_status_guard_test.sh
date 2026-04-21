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

require_pattern 'QDateTime[[:space:]]+parseArtifactEventTime' "$scanner_source" \
    "SessionScanner must parse artifact event timestamps before inferring Codex activity."
require_pattern 'const[[:space:]]+double[[:space:]]+codexActiveFreshSeconds[[:space:]]*=' "$scanner_source" \
    "Codex activity inference must use a freshness window instead of trusting stale events forever."
require_pattern 'type[[:space:]]*==[[:space:]]*"task_started"' "$scanner_source" \
    "Codex event inference should still inspect task_started records."
require_pattern 'payloadType[[:space:]]*==[[:space:]]*"user_message"' "$scanner_source" \
    "Codex user input events must be treated explicitly so typing/waiting does not count as Working."
require_pattern 'payloadType[[:space:]]*==[[:space:]]*"message"' "$scanner_source" \
    "Codex generic message items must be distinguished from real active execution events."
require_pattern 'eventTime\.secsTo\(QDateTime::currentDateTimeUtc\(\)\)' "$scanner_source" \
    "Codex Working inference must compare artifact event time against the current time."
require_pattern 'return[[:space:]]+SessionStatus::Ready;' "$scanner_source" \
    "Codex stale activity events must decay back to Ready."
require_pattern 'parseRecentJsonObjects' "$scanner_source" \
    "Codex and Claude status inference must scan multiple recent artifact events instead of trusting only the last line."
require_pattern 'usedPaths\.insert\(remainingArtifacts\.at\(bestArtifactIndex\)\.sessionPath\)' "$scanner_source" \
    "Codex artifact assignment must reserve each matched rollout so different monitor rows do not reuse the same session ID."
require_pattern 'remainingIndexes\.removeAll\(bestSessionIndex\)' "$scanner_source" \
    "Codex artifact matching must consume each live process only once during one-to-one session assignment."
require_pattern 'candidate\.detailText\.contains\("/vendor/"\)' "$scanner_source" \
    "Codex process dedupe should prefer the real vendor executable over the outer node wrapper."
require_pattern 'collectCodexSessionLogRecords' "$scanner_source" \
    "Codex session assignment must parse codex-tui.log as an active-session index."
require_pattern '\"workdir\"' "$scanner_source" \
    "Codex log parsing must read workdir fields to map live threads back to project roots."
require_pattern 'usedSessionIds' "$scanner_source" \
    "Codex log/session assignment must prevent one thread ID from being reused across multiple rows."

echo "PASS: Codex status guard rails are wired."
