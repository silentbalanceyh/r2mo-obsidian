#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
header="$repo_root/src/utils/sessionscanner.h"
scanner_source="$repo_root/src/utils/sessionscanner.cpp"
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

require_pattern 'qint64[[:space:]]+runtimeSeconds;' "$header" \
    "SessionInfo must store cumulative runtime seconds for monitor status display."
require_pattern 'QString[[:space:]]+formatSessionRuntime[[:space:]]*\([[:space:]]*qint64[[:space:]]+runtimeSeconds[[:space:]]*\)[[:space:]]+const' "$repo_root/src/mainwindow.h" \
    "MainWindow must expose a formatter for cumulative session runtime."
require_pattern 'class[[:space:]]+MonitorTreeDelegate' "$window_source" \
    "Monitor rows should render status through a delegate to avoid per-row widget overhead."
require_pattern 'const[[:space:]]+QString[[:space:]]+statusText[[:space:]]*=' "$window_source" \
    "Monitor status paint path must build a combined status label with runtime."
require_pattern 'QCoreApplication::translate\("MainWindow",[[:space:]]*"Working"\)' "$window_source" \
    "Working status text must use the MainWindow translation context."
require_pattern 'QCoreApplication::translate\("MainWindow",[[:space:]]*"Ready"\)' "$window_source" \
    "Ready status text must use the MainWindow translation context."
require_pattern 'QCoreApplication::translate\("MainWindow",[[:space:]]*"Unknown"\)' "$window_source" \
    "Unknown status text must use the MainWindow translation context."
require_pattern 'QColor\(\"#34c759\"' "$window_source" \
    "Working status paint path must render in green."
require_pattern 'QColor\(\"#007aff\"' "$window_source" \
    "Ready status paint path must render in blue."
require_pattern 'QColor\(\"#8e8e93\"' "$window_source" \
    "Unknown status paint path must render in neutral gray."
require_pattern 'm_animationOffset[[:space:]]*=' "$window_source" \
    "Working status paint path should keep a lightweight animation offset for the running bar."
require_pattern 'painter->fillRect\(centeredBarRect, fillColor\);' "$window_source" \
    "Status bars should use full-width fills so Ready can render as a zebra waiting stripe."
require_pattern 'Memory %1M' "$window_source" \
    "Toolbar should expose a live RSS memory label."
require_pattern 'updateMonitorStatusLabel\(QWidget \*label, SessionStatus status\) const' "$repo_root/src/mainwindow.h" \
    "Monitor status updater must operate on a generic QWidget container, not a QLabel."
require_pattern 'setItemDelegate\(new[[:space:]]+MonitorTreeDelegate\(tree\)\)' "$window_source" \
    "Monitor tree should install the lightweight delegate for status/action painting."
require_pattern 'const[[:space:]]+qint64[[:space:]]+runningSeconds[[:space:]]*=' "$scanner_source" \
    "Session scanning must calculate cumulative running seconds."
require_pattern 'runtimeSeconds[[:space:]]*=' "$scanner_source" \
    "Detected sessions must persist computed runtime seconds."
require_pattern 'const[[:space:]]+double[[:space:]]+workingKeepAliveSeconds[[:space:]]*=' "$scanner_source" \
    "Status detection must use an explicit keep-alive window to reduce drift."
require_pattern 'secsSinceWork[[:space:]]*<[[:space:]]*workingKeepAliveSeconds' "$scanner_source" \
    "Working status should remain stable during the keep-alive window instead of dropping immediately."

echo "PASS: task-002 monitor status requirements are wired."
