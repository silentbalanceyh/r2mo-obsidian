#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
header="$repo_root/src/mainwindow.h"
source="$repo_root/src/mainwindow.cpp"
settings_source="$repo_root/src/utils/settingsmanager.cpp"

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
    "MainWindow must expose an internal helper to toggle monitor table row-level refresh loading."
require_pattern 'void[[:space:]]+setMonitorRefreshEnabled[[:space:]]*\([[:space:]]*bool[[:space:]]+enabled[[:space:]]*\)' "$header" \
    "MainWindow must expose an internal helper to enable or disable the monitor refresh trigger."
require_pattern 'setObjectName[[:space:]]*\([[:space:]]*"monitorRefreshOverlay"[[:space:]]*\)' "$source" \
    "Monitor table view must create a named loading overlay for refresh feedback."
require_pattern 'setObjectName[[:space:]]*\([[:space:]]*"monitorRefreshTableHost"[[:space:]]*\)' "$source" \
    "Monitor table refresh overlay must be attached to the top table host, not the whole board."
require_pattern 'm_monitorVerticalSplitter->addWidget[[:space:]]*\([[:space:]]*tableStack[[:space:]]*\)' "$source" \
    "Top monitor table host must be the splitter child so the special billing panel keeps its own loading behavior."
require_pattern 'mainLayout->addWidget[[:space:]]*\([[:space:]]*m_monitorVerticalSplitter[[:space:]]*,[[:space:]]*1[[:space:]]*\)' "$source" \
    "The splitter must remain the board container after isolating the top-table overlay."
require_pattern 'setMonitorRefreshLoading[[:space:]]*\([[:space:]]*true[[:space:]]*\)' "$source" \
    "Manual monitor refresh must mark rows as refreshing before async scanning starts."
require_pattern 'setMonitorRefreshLoading[[:space:]]*\([[:space:]]*false[[:space:]]*\)' "$source" \
    "Monitor scan completion must clear row-level refreshing state."
require_pattern 'setMonitorRefreshEnabled[[:space:]]*\([[:space:]]*false[[:space:]]*\)' "$source" \
    "Manual monitor refresh must disable its refresh button during loading."
require_pattern 'setMonitorRefreshEnabled[[:space:]]*\([[:space:]]*true[[:space:]]*\)' "$source" \
    "Monitor refresh completion must re-enable its refresh button."
require_pattern 'connect[[:space:]]*\([[:space:]]*refreshBtn[[:space:]]*,[[:space:]]*&QPushButton::clicked[[:space:]]*,[[:space:]]*this[[:space:]]*,[[:space:]]*&MainWindow::refreshMonitorAsync[[:space:]]*\)' "$source" \
    "Refresh button must be connected directly, not through an indirect child lookup."
require_pattern 'setObjectName[[:space:]]*\([[:space:]]*"monitorRefreshButton"[[:space:]]*\)' "$source" \
    "Monitor refresh button must have a stable object name so its enabled state can be managed."
require_pattern 'class[[:space:]]+MonitorTreeDelegate' "$source" \
    "Monitor row status must be rendered through the lightweight delegate."
require_pattern 'kMonitorLoadingRole' "$source" \
    "Monitor rows must carry an inline refreshing state."
require_pattern 'kMonitorPlaceholderRole' "$source" \
    "Placeholder rows must be distinguishable from real session rows."
require_pattern 'buildMonitorView\(placeholders\)' "$source" \
    "Initial monitor opening must render project rows immediately instead of an empty table."
require_pattern 'createMonitorLoadingIcon' "$source" \
    "Monitor row refresh feedback should be a compact project-column indicator."
require_pattern 'row->setIcon\(0,[[:space:]]*projectLoading[[:space:]]*\?[[:space:]]*createMonitorLoadingIcon\(\)[[:space:]]*:[[:space:]]*QIcon\(\)\)' "$source" \
    "Refreshing state should appear beside the project name, not as blocking row text."
require_pattern 'row->setIcon\(0,[[:space:]]*rowLoading[[:space:]]*\?[[:space:]]*createMonitorLoadingIcon\(\)[[:space:]]*:[[:space:]]*QIcon\(\)\)' "$source" \
    "Refresh toggling must update the project-column loading indicator in place."
require_pattern 'baseIndex\.data\(kMonitorPlaceholderRole\)\.toBool\(\)' "$source" \
    "Status delegate must bypass progress-bar painting for placeholder rows."
forbid_pattern 'QCoreApplication::translate\("MainWindow",[[:space:]]*"Refreshing\.\.\."\)' "$source" \
    "Monitor table loading should no longer be rendered as status-column text."
forbid_pattern 'row->setText\(1,[[:space:]]*projectLoading[[:space:]]*\\?[[:space:]]*tr\("Refreshing\.\.\."\)' "$source" \
    "Refreshing state must not overwrite the Terminal column."
forbid_pattern 'row->setText\(4,[[:space:]]*projectLoading[[:space:]]*\\?[[:space:]]*tr\("Refreshing\.\.\."\)' "$source" \
    "Refreshing state must not overwrite the Status column."
require_pattern 'statusWidth[[:space:]]*=.*344.*372' "$source" \
    "Monitor status column width must be expanded to fit the wider bar and visible status text."
require_pattern 'minStatusWidth[[:space:]]*=[[:space:]]*336' "$source" \
    "Monitor status column minimum width must prevent the column from collapsing below the control width."
require_pattern 'monitorBoardHeaderState\.v[0-9]+' "$settings_source" \
    "Monitor board header persistence must be versioned so stale saved widths do not override the status column layout."
require_pattern '#d9ecff' "$source" \
    "Ready state must use a blue waiting-style status bar instead of a grey disabled look."
forbid_pattern 'auto createLegend' "$source" \
    "Monitor board should no longer render the top legend/status indicator strip."
forbid_pattern 'legendLayout->addWidget\(createLegend' "$source" \
    "Monitor board should no longer add the removed top legend widgets."
require_pattern 'm_specialMonitorStatusLabel->setText[[:space:]]*\([[:space:]]*tr\("Refreshing\.\.\."\)[[:space:]]*\)' "$source" \
    "Special billing refresh must keep its own status-label loading state."
require_pattern 'void[[:space:]]+setSpecialMonitorRefreshLoading[[:space:]]*\([[:space:]]*bool[[:space:]]+loading[[:space:]]*\)' "$header" \
    "MainWindow must expose an internal helper to toggle special billing row-level refresh loading."
require_pattern 'void[[:space:]]+setSpecialMonitorActionsEnabled[[:space:]]*\([[:space:]]*bool[[:space:]]+enabled[[:space:]]*\)' "$header" \
    "MainWindow must expose an internal helper to enable or disable the special billing action buttons."
require_pattern 'setObjectName[[:space:]]*\([[:space:]]*"specialMonitorRefreshOverlay"[[:space:]]*\)' "$source" \
    "Special billing panel must create its own named loading overlay."
require_pattern 'setObjectName[[:space:]]*\([[:space:]]*"specialMonitorTableHost"[[:space:]]*\)' "$source" \
    "Special billing loading overlay must be attached to a dedicated table host."
require_pattern 'setSpecialMonitorRefreshLoading[[:space:]]*\([[:space:]]*true[[:space:]]*\)' "$source" \
    "Special billing refresh must mark rows as refreshing before async fetching starts."
require_pattern 'setSpecialMonitorRefreshLoading[[:space:]]*\([[:space:]]*false[[:space:]]*\)' "$source" \
    "Special billing refresh completion must clear row-level refreshing state."
require_pattern 'updatedItem->setText\(tr\("Refreshing\.\.\."\)\)' "$source" \
    "Special billing refresh must put loading feedback into the row instead of blocking the table."
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

echo "PASS: monitor refresh loading overlay wiring is present."
