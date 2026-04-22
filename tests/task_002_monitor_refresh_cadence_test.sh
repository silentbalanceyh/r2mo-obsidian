#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source_file="$repo_root/src/mainwindow.cpp"

require_pattern() {
    local pattern="$1"
    local file="$2"
    local message="$3"

    if ! grep -Eq "$pattern" "$file"; then
        echo "FAIL: $message" >&2
        exit 1
    fi
}

require_pattern 'm_monitorRefreshTimer[[:space:]]*=[[:space:]]*new[[:space:]]+QTimer\(this\);' "$source_file" \
    "Upper monitor board should keep its dedicated refresh timer."
require_pattern 'm_monitorRefreshTimer->setInterval\(60 .* 1000\)' "$source_file" \
    "Upper monitor board auto refresh should slow down to a 1 minute cadence after adding SSH remote scans."
require_pattern 'm_specialMonitorRefreshTimer->setInterval\(5 .* 60 .* 1000\)' "$source_file" \
    "Special billing panel should keep its independent 5 minute cadence."

echo "PASS: monitor refresh cadence guards are wired."
