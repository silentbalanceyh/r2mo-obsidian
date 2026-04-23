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

require_pattern 'const[[:space:]]+double[[:space:]]+openCodeActiveFreshSeconds[[:space:]]*=' "$scanner_source" \
    "Local OpenCode status inference must define an explicit freshness window for active notifications."
require_pattern 'type == "turn-start" \|\| type == "session.status"' "$scanner_source" \
    "Local OpenCode status inference must still inspect active-start notification types."
require_pattern 'notificationTime\.msecsTo\(QDateTime::currentDateTimeUtc\(\)\)[[:space:]]*<=' "$scanner_source" \
    "Local OpenCode Working inference must compare notification time against now."
require_pattern 'return notificationIsFresh \? SessionStatus::Working : SessionStatus::Ready;' "$scanner_source" \
    "Local OpenCode stale active notifications must decay back to Ready."
require_pattern 'notification_time_is_fresh' "$remote_scanner_source" \
    "Remote OpenCode probe must define freshness logic for active notifications."
require_pattern "return 'working' if notification_time_is_fresh\\(notification\\) else 'ready'" "$remote_scanner_source" \
    "Remote OpenCode stale active notifications must decay back to ready."

echo "PASS: OpenCode freshness guard rails are wired."
