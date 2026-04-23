#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
window_source="$repo_root/src/mainwindow.cpp"

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

require_pattern 'const QString statusText = cell.status == SessionStatus::Working' "$window_source" \
    "Monitor grid status should render a Working/Ready text label again."
forbid_pattern 'QStringLiteral\(" "\) \+ runtimeText' "$window_source" \
    "Monitor status rendering must not append runtime text to Ready or Working labels anymore."
forbid_pattern 'const QString runtimeText = formatRuntime\(runtimeSeconds\);' "$window_source" \
    "Monitor status paint path should not compute runtime text for the status cell anymore."

echo "PASS: task-002 monitor runtime display guard is present."
