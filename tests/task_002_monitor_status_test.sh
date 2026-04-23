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

forbid_pattern() {
    local pattern="$1"
    local file="$2"
    local message="$3"

    if grep -Eq "$pattern" "$file"; then
        echo "FAIL: $message" >&2
        exit 1
    fi
}

require_pattern 'qint64[[:space:]]+runtimeSeconds;' "$header" \
    "SessionInfo must store cumulative runtime seconds for monitor status display."
require_pattern 'QString[[:space:]]+formatSessionRuntime[[:space:]]*\([[:space:]]*qint64[[:space:]]+runtimeSeconds[[:space:]]*\)[[:space:]]+const' "$repo_root/src/mainwindow.h" \
    "MainWindow must expose a formatter for cumulative session runtime."
require_pattern 'class[[:space:]]+MonitorToolGridDelegate' "$window_source" \
    "Monitor rows should render status through a delegate to avoid per-row widget overhead."
require_pattern 'const[[:space:]]+QString[[:space:]]+statusText[[:space:]]*=[[:space:]]*cell.status[[:space:]]*==[[:space:]]*SessionStatus::Working' "$window_source" \
    "Monitor grid status paint path must render a Working/Ready label from session status."
require_pattern 'QCoreApplication::translate\("MainWindow",[[:space:]]*"Working"\)' "$window_source" \
    "Working status text must use the MainWindow translation context."
require_pattern 'QCoreApplication::translate\("MainWindow",[[:space:]]*"Ready"\)' "$window_source" \
    "Ready status text must use the MainWindow translation context."
forbid_pattern 'QStringLiteral\(" "\)[[:space:]]*\+[[:space:]]*runtimeText' "$window_source" \
    "Monitor status paint path must no longer append runtime text to the Ready/Working label."
require_pattern 'QColor\(\"#34c759\"' "$window_source" \
    "Working status paint path must render in green."
require_pattern 'QColor\(\"#4f9cff\"' "$window_source" \
    "Ready status paint path must render in blue."
require_pattern 'drawText\(statusTextRect,[[:space:]]*Qt::AlignRight[[:space:]]*\|[[:space:]]*Qt::AlignVCenter,[[:space:]]*statusText\)' "$window_source" \
    "Monitor grid status text must render on the right side of each cell."
require_pattern 'm_animationOffset[[:space:]]*=' "$window_source" \
    "Working status paint path should keep a lightweight animation offset for the running bar."
require_pattern 'painter->fillRect\(barRect, fillColor\);' "$window_source" \
    "Status bars should use full-width fills so Ready can render as a zebra waiting stripe."
require_pattern 'Memory %1M' "$window_source" \
    "Toolbar should expose a live RSS memory label."
require_pattern 'updateMonitorStatusLabel\(QWidget \*label, SessionStatus status\) const' "$repo_root/src/mainwindow.h" \
    "Monitor status updater must operate on a generic QWidget container, not a QLabel."
require_pattern 'setItemDelegate\(new[[:space:]]+MonitorToolGridDelegate\(tree\)\)' "$window_source" \
    "Monitor tree should install the lightweight delegate for status/action painting."
require_pattern 'const[[:space:]]+qint64[[:space:]]+runningSeconds[[:space:]]*=' "$scanner_source" \
    "Session scanning must calculate cumulative running seconds."
require_pattern 'runtimeSeconds[[:space:]]*=' "$scanner_source" \
    "Detected sessions must persist computed runtime seconds."
require_pattern 'const[[:space:]]+double[[:space:]]+workingKeepAliveSeconds[[:space:]]*=' "$scanner_source" \
    "Status detection must use an explicit keep-alive window to reduce drift."
require_pattern 'secsSinceWork[[:space:]]*<[[:space:]]*workingKeepAliveSeconds' "$scanner_source" \
    "Working status should remain stable during the keep-alive window instead of dropping immediately."
require_pattern 'const[[:space:]]+bool[[:space:]]+hasToolAncestor[[:space:]]*=' "$scanner_source" \
    "Live session detection must identify descendant helper processes that are not session roots."
require_pattern 'if[[:space:]]*\([[:space:]]*hasToolAncestor[[:space:]]*\)[[:space:]]*\{' "$scanner_source" \
    "Live session detection must skip descendant helper processes once a root tool process exists in the same terminal chain."
require_pattern 'const[[:space:]]+double[[:space:]]+claudeActiveFreshSeconds[[:space:]]*=' "$scanner_source" \
    "Claude artifact status detection must use an explicit freshness window."
require_pattern 'eventTime\.secsTo\(QDateTime::currentDateTimeUtc\(\)\)[[:space:]]*<=[[:space:]]*claudeActiveFreshSeconds' "$scanner_source" \
    "Claude status detection must verify event freshness before reporting Working."
require_pattern 'type[[:space:]]*==[[:space:]]*"tool_use"' "$scanner_source" \
    "Claude status detection must handle tool_use events explicitly."
require_pattern 'return[[:space:]]+eventIsFresh[[:space:]]*\?[[:space:]]*SessionStatus::Working[[:space:]]*:[[:space:]]*SessionStatus::Ready' "$scanner_source" \
    "Claude status detection must degrade stale activity events back to Ready instead of leaving phantom Working states."
forbid_pattern 'isRunning[[:space:]]*&&[[:space:]]*deltaPerSecond[[:space:]]*>=[[:space:]]*25000' "$scanner_source" \
    "CPU fallback must not flip to Working from a single low-threshold running sample during refresh."

echo "PASS: task-002 monitor status requirements are wired."
