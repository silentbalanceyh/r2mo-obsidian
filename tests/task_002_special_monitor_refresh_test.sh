#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
header="$repo_root/src/mainwindow.h"
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

require_pattern 'QTimer.*m_specialMonitorRefreshTimer;' "$header" \
    "Special billing panel should own a dedicated refresh timer instead of piggybacking on monitor polling."
require_pattern 'm_specialMonitorRefreshTimer[[:space:]]*=[[:space:]]*new[[:space:]]+QTimer\(this\);' "$source_file" \
    "Special billing panel should initialize its dedicated refresh timer."
require_pattern 'm_specialMonitorRefreshTimer->setInterval\(5 .* 60 .* 1000\)' "$source_file" \
    "Special billing panel auto refresh should default to a 5 minute cadence."
require_pattern 'refreshSpecialMonitorAsync\(false\)' "$source_file" \
    "Special billing panel should support non-forced auto refreshes."
require_pattern 'refreshSpecialMonitorAsync\(true\)' "$source_file" \
    "Manual special billing refresh actions should force a fresh fetch."
require_pattern 'm_specialMonitorDataCache' "$header" \
    "Special billing panel should cache fetched snapshots so monitor table rebuilds do not re-fetch immediately."

echo "PASS: special monitor refresh cadence guard rails are wired."
