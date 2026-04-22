#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
header="$repo_root/src/mainwindow.h"
source="$repo_root/src/mainwindow.cpp"

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

require_pattern 'void[[:space:]]+setMonitorRefreshLoading[[:space:]]*\([[:space:]]*bool[[:space:]]+loading[[:space:]]*\)' "$header" \
    "MainWindow must expose a helper that toggles monitor loading visuals."
require_pattern 'void[[:space:]]+setMonitorRefreshEnabled[[:space:]]*\([[:space:]]*bool[[:space:]]+enabled[[:space:]]*\)' "$header" \
    "MainWindow must expose a helper that toggles the monitor refresh trigger."
require_pattern 'kMonitorLoadingRole' "$source" \
    "Monitor rows must persist a loading state."
require_pattern 'paintProjectCell' "$source" \
    "Monitor loading should be rendered in the project cell."
require_pattern 'm_loadingAngle[[:space:]]*=' "$source" \
    "Monitor loading indicator must animate by advancing an angle."
require_pattern 'painter->drawArc' "$source" \
    "Monitor loading indicator should be a compact animated circle."
require_pattern 'QList<ProjectMonitorData>[[:space:]]+placeholderData' "$source" \
    "Opening the monitor board should prepare placeholder rows immediately."
require_pattern 'buildMonitorView\(placeholderData\)' "$source" \
    "Monitor board must render project placeholders on first open."
require_pattern 'pmd\.sessions\.isEmpty\(\)' "$source" \
    "Monitor board must keep project placeholder rows when sessions are still loading."
require_pattern 'row->setData\(0,[[:space:]]*kMonitorLoadingRole' "$source" \
    "Monitor rows must store loading state for later refresh toggles."
require_pattern 'rowLoading[[:space:]]*=[[:space:]]*loading[[:space:]]*&&[[:space:]]*!row->text\(0\)\.isEmpty\(\)' "$source" \
    "Refresh toggling should update only project-name rows with an inline loading circle."
forbid_pattern 'Loading Monitor Board' "$source" \
    "Initial monitor opening must no longer replace the table with a full-page loading screen."
monitor_loading_body="$(awk '/void MainWindow::setMonitorRefreshLoading\(bool loading\)/,/^}/' "$source")"
if grep -Eq 'overlay->setVisible\(loading\)' <<< "$monitor_loading_body"; then
    echo "FAIL: Main monitor refresh should no longer show a blocking table overlay." >&2
    exit 1
fi
require_pattern 'setSpecialMonitorRefreshLoading[[:space:]]*\([[:space:]]*true[[:space:]]*\)' "$source" \
    "Special billing refresh should keep its own loading behavior."

echo "PASS: monitor refresh loading wiring is present."
