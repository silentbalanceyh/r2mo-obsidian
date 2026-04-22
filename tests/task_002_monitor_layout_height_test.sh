#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
main_source="$repo_root/src/main.cpp"
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

require_pattern 'window\.resize\(1680,[[:space:]]*1050\);' "$main_source" \
    "Application bootstrap must use the 1680x1050 default window size."
require_pattern 'resize\(1680,[[:space:]]*1050\);' "$window_source" \
    "MainWindow must default to 1680x1050 before geometry restore."
require_pattern 'const int minPanelHeight = 160;' "$window_source" \
    "Special monitor billing panel minimum height must shrink so more space returns to the project list."
require_pattern 'const int preferredPanelHeight = qBound\(160, 300, totalHeight / 3\);' "$window_source" \
    "Special monitor billing panel preferred height must stay compact instead of holding the added monitor height."
require_pattern 'const int maxPanelHeight = qMax\(preferredPanelHeight, 360\);' "$window_source" \
    "Special monitor billing panel maximum height must be reduced to keep the lower area from growing too tall."
require_pattern 'const int topHeight = qMax\(260, totalHeight - targetPanelHeight\);' "$window_source" \
    "Extra monitor-board height must be allocated to the upper real-time panel."

echo "PASS: monitor layout height guards are wired."
