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
require_pattern 'type == "turn-start"' "$scanner_source" \
    "Local OpenCode status inference must still inspect active-start notification types."
require_pattern 'type == "session.status"' "$scanner_source" \
    "Local OpenCode status inference must still inspect session.status notification types."
require_pattern 'firstStatusScalar\(entry\.value\("status"\)\)' "$scanner_source" \
    "Local OpenCode session.status inference must inspect the explicit status payload instead of trusting the event name alone."
require_pattern 'isActiveStatusWord\(statusValue\)' "$scanner_source" \
    "Local OpenCode session.status inference must only report Working for active status words."
require_pattern 'notificationTime\.msecsTo\(QDateTime::currentDateTimeUtc\(\)\)[[:space:]]*<=' "$scanner_source" \
    "Local OpenCode Working inference must compare notification time against now."
require_pattern 'return notificationIsFresh \? SessionStatus::Working : SessionStatus::Ready;' "$scanner_source" \
    "Local OpenCode stale active notifications must decay back to Ready."
require_pattern 'notification_time_is_fresh' "$remote_scanner_source" \
    "Remote OpenCode probe must define freshness logic for active notifications."
require_pattern 'status_value = first_status_scalar\(notification\.get\('"'"'status'"'"'\)\)' "$remote_scanner_source" \
    "Remote OpenCode probe must inspect the notification status payload."
require_pattern "if is_active_status_word\\(status_value\\):" "$remote_scanner_source" \
    "Remote OpenCode probe must only report working for explicit active status words."
require_pattern "return 'working' if notification_time_is_fresh\\(notification\\) else 'ready'" "$remote_scanner_source" \
    "Remote OpenCode stale active notifications must decay back to ready."

echo "PASS: OpenCode freshness guard rails are wired."
