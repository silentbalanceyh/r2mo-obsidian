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
    "MainWindow must expose an internal helper to toggle monitor refresh loading."
require_pattern 'void[[:space:]]+setMonitorRowsLoading[[:space:]]*\([[:space:]]*bool[[:space:]]+loading[[:space:]]*\)' "$header" \
    "MainWindow must expose a row-level monitor loading helper."
require_pattern 'void[[:space:]]+setMonitorRefreshEnabled[[:space:]]*\([[:space:]]*bool[[:space:]]+enabled[[:space:]]*\)' "$header" \
    "MainWindow must expose an internal helper to enable or disable the monitor refresh trigger."
forbid_pattern 'monitorRefreshOverlay' "$source" \
    "Main monitor loading must not use a full-table overlay."
forbid_pattern 'monitorRefreshTableHost' "$source" \
    "Main monitor loading must not rebuild the table into an overlay host."
require_pattern 'setMonitorRowsLoading[[:space:]]*\([[:space:]]*true[[:space:]]*\)' "$source" \
    "Manual monitor refresh must enable row-level loading before async scanning starts."
require_pattern 'setMonitorRowsLoading[[:space:]]*\([[:space:]]*false[[:space:]]*\)' "$source" \
    "Monitor scan completion must clear row-level loading after async scanning ends."
require_pattern 'setMonitorRefreshEnabled[[:space:]]*\([[:space:]]*false[[:space:]]*\)' "$source" \
    "Manual monitor refresh must disable its refresh button during loading."
require_pattern 'setMonitorRefreshEnabled[[:space:]]*\([[:space:]]*true[[:space:]]*\)' "$source" \
    "Monitor refresh completion must re-enable its refresh button."
require_pattern 'connect[[:space:]]*\([[:space:]]*refreshBtn[[:space:]]*,[[:space:]]*&QPushButton::clicked[[:space:]]*,[[:space:]]*this[[:space:]]*,[[:space:]]*&MainWindow::refreshMonitorAsync[[:space:]]*\)' "$source" \
    "Refresh button must be connected directly, not through an indirect child lookup."
require_pattern 'setObjectName[[:space:]]*\([[:space:]]*"monitorRefreshButton"[[:space:]]*\)' "$source" \
    "Monitor refresh button must have a stable object name so its enabled state can be managed."
require_pattern 'QStringLiteral\("◌ %1"\)' "$source" \
    "Row-level loading must prefix the session column with the loading marker."
require_pattern 'row->setText\(3,[[:space:]]*loading[[:space:]]*\?[[:space:]]*QStringLiteral\("◌ %1"\)' "$source" \
    "Main monitor loading must only change the existing row session text."
forbid_pattern 'struct[[:space:]]+MonitorRowSnapshot' "$header" \
    "Main monitor loading must not introduce a shadow row model that can drift from real session data."
forbid_pattern 'collectMonitorRowSnapshots' "$header" \
    "Main monitor loading must not synthesize placeholder monitor rows."
forbid_pattern 'collectMonitorRowSnapshotMap' "$header" \
    "Main monitor loading must not synthesize placeholder monitor rows."
forbid_pattern 'updateMonitorTreeRows' "$header" \
    "Main monitor loading must not rebuild tree rows through a synthetic snapshot layer."
require_pattern 'm_specialMonitorStatusLabel->setText[[:space:]]*\([[:space:]]*tr\("Refreshing\.\.\."\)[[:space:]]*\)' "$source" \
    "Special billing refresh must keep its own status-label loading state."
require_pattern 'void[[:space:]]+setSpecialMonitorRefreshLoading[[:space:]]*\([[:space:]]*bool[[:space:]]+loading[[:space:]]*\)' "$header" \
    "MainWindow must expose an internal helper to toggle the special billing refresh loading overlay."
require_pattern 'void[[:space:]]+setSpecialMonitorActionsEnabled[[:space:]]*\([[:space:]]*bool[[:space:]]+enabled[[:space:]]*\)' "$header" \
    "MainWindow must expose an internal helper to enable or disable the special billing action buttons."
require_pattern 'setObjectName[[:space:]]*\([[:space:]]*"specialMonitorRefreshOverlay"[[:space:]]*\)' "$source" \
    "Special billing panel must create its own named loading overlay."
require_pattern 'setObjectName[[:space:]]*\([[:space:]]*"specialMonitorTableHost"[[:space:]]*\)' "$source" \
    "Special billing loading overlay must be attached to a dedicated table host."
require_pattern 'setSpecialMonitorRefreshLoading[[:space:]]*\([[:space:]]*true[[:space:]]*\)' "$source" \
    "Special billing refresh must show its loading overlay before async fetching starts."
require_pattern 'setSpecialMonitorRefreshLoading[[:space:]]*\([[:space:]]*false[[:space:]]*\)' "$source" \
    "Special billing refresh completion must hide its loading overlay."
require_pattern 'setSpecialMonitorActionsEnabled[[:space:]]*\([[:space:]]*false[[:space:]]*\)' "$source" \
    "Special billing refresh must disable its action buttons during loading."
require_pattern 'setSpecialMonitorActionsEnabled[[:space:]]*\([[:space:]]*true[[:space:]]*\)' "$source" \
    "Special billing refresh completion must re-enable its action buttons."
require_pattern 'addBtn->setCursor[[:space:]]*\([[:space:]]*Qt::PointingHandCursor[[:space:]]*\)' "$source" \
    "Special billing Add button must use the pointing-hand cursor."
require_pattern 'editBtn->setCursor[[:space:]]*\([[:space:]]*Qt::PointingHandCursor[[:space:]]*\)' "$source" \
    "Special billing Edit button must use the pointing-hand cursor."
require_pattern 'removeBtn->setCursor[[:space:]]*\([[:space:]]*Qt::PointingHandCursor[[:space:]]*\)' "$source" \
    "Special billing Remove button must use the pointing-hand cursor."
require_pattern 'refreshBtn->setCursor[[:space:]]*\([[:space:]]*Qt::PointingHandCursor[[:space:]]*\)' "$source" \
    "Special billing Refresh button must use the pointing-hand cursor."
require_pattern 'setObjectName[[:space:]]*\([[:space:]]*"specialMonitorAddButton"[[:space:]]*\)' "$source" \
    "Special billing Add button must have a stable object name."
require_pattern 'setObjectName[[:space:]]*\([[:space:]]*"specialMonitorEditButton"[[:space:]]*\)' "$source" \
    "Special billing Edit button must have a stable object name."
require_pattern 'setObjectName[[:space:]]*\([[:space:]]*"specialMonitorRemoveButton"[[:space:]]*\)' "$source" \
    "Special billing Remove button must have a stable object name."
require_pattern 'setObjectName[[:space:]]*\([[:space:]]*"specialMonitorRefreshButton"[[:space:]]*\)' "$source" \
    "Special billing Refresh button must have a stable object name."

echo "PASS: monitor refresh row-loading guard rails are present."
