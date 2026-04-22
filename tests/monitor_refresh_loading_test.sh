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
    "MainWindow must expose an internal helper to toggle the monitor table refresh loading overlay."
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
    "Manual monitor refresh must show the loading overlay before async scanning starts."
require_pattern 'setMonitorRefreshLoading[[:space:]]*\([[:space:]]*false[[:space:]]*\)' "$source" \
    "Monitor scan completion must hide the loading overlay."
require_pattern 'setMonitorRefreshEnabled[[:space:]]*\([[:space:]]*false[[:space:]]*\)' "$source" \
    "Manual monitor refresh must disable its refresh button during loading."
require_pattern 'setMonitorRefreshEnabled[[:space:]]*\([[:space:]]*true[[:space:]]*\)' "$source" \
    "Monitor refresh completion must re-enable its refresh button."
require_pattern 'connect[[:space:]]*\([[:space:]]*refreshBtn[[:space:]]*,[[:space:]]*&QPushButton::clicked[[:space:]]*,[[:space:]]*this[[:space:]]*,[[:space:]]*&MainWindow::refreshMonitorAsync[[:space:]]*\)' "$source" \
    "Refresh button must be connected directly, not through an indirect child lookup."
require_pattern 'setObjectName[[:space:]]*\([[:space:]]*"monitorRefreshButton"[[:space:]]*\)' "$source" \
    "Monitor refresh button must have a stable object name so its enabled state can be managed."
require_pattern 'QProgressBar[^\\n]*statusBar' "$source" \
    "Monitor row status must be rendered with progress bars instead of simple dots."
require_pattern 'setObjectName[[:space:]]*\([[:space:]]*"monitorStatusBar"[[:space:]]*\)' "$source" \
    "Monitor status progress bars must use a stable object name."
require_pattern 'setFixedWidth[[:space:]]*\([[:space:]]*256[[:space:]]*\)' "$source" \
    "Monitor status progress bars must be widened to double the previous width."
require_pattern 'statusWidth[[:space:]]*=.*304.*336' "$source" \
    "Monitor status column width must be expanded to fit the wider bar and visible status text."
require_pattern 'minStatusWidth[[:space:]]*=[[:space:]]*280' "$source" \
    "Monitor status column minimum width must prevent the column from collapsing below the control width."
require_pattern 'monitorBoardHeaderState\\.v3' "$settings_source" \
    "Monitor board header persistence version must be bumped so stale saved widths do not override the new status column layout."
require_pattern 'QLabel[^\\n]*statusTextLabel' "$source" \
    "Monitor status presentation must preserve visible status text beside the progress bar."
require_pattern 'setObjectName[[:space:]]*\([[:space:]]*"monitorStatusText"[[:space:]]*\)' "$source" \
    "Monitor status text labels must use a stable object name."
require_pattern 'setRange[[:space:]]*\([[:space:]]*0[[:space:]]*,[[:space:]]*0[[:space:]]*\)' "$source" \
    "Working state must use an indeterminate progress bar."
require_pattern 'setRange[[:space:]]*\([[:space:]]*0[[:space:]]*,[[:space:]]*100[[:space:]]*\)' "$source" \
    "Ready state must use a bounded progress bar."
require_pattern 'setValue[[:space:]]*\([[:space:]]*0[[:space:]]*\)' "$source" \
    "Ready state must show a blocked or empty progress bar."
require_pattern '#d9ecff' "$source" \
    "Ready state must use a blue waiting-style progress bar instead of a grey disabled look."
require_pattern 'setProperty[[:space:]]*\([[:space:]]*"slowMode"[[:space:]]*,[[:space:]]*true[[:space:]]*\)' "$source" \
    "Working monitor bars must be marked for slower animation handling."
forbid_pattern 'auto createLegend' "$source" \
    "Monitor board should no longer render the top legend/status indicator strip."
forbid_pattern 'legendLayout->addWidget\\(createLegend' "$source" \
    "Monitor board should no longer add the removed top legend widgets."
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

echo "PASS: monitor refresh loading overlay wiring is present."
