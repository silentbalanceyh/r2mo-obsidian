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
require_pattern 'formatSessionRuntime[[:space:]]*\([[:space:]]*runtimeSeconds[[:space:]]*\)' "$window_source" \
    "Monitor status text must append the cumulative runtime after the status label."
require_pattern 'statusTextLabel->setStyleSheet\("QLabel \{ color: #34c759;' "$window_source" \
    "Working status text must render in green."
require_pattern 'statusTextLabel->setStyleSheet\("QLabel \{ color: #007aff;' "$window_source" \
    "Ready status text must render in blue."
require_pattern 'updateMonitorStatusLabel\(QWidget \*label, SessionStatus status\) const' "$repo_root/src/mainwindow.h" \
    "Monitor status updater must operate on a generic QWidget container, not a QLabel."
require_pattern 'statusContainer[[:space:]]*=[[:space:]]*new[[:space:]]+QWidget\(\)' "$window_source" \
    "Status cell widget must use a plain QWidget container so text does not paint over the progress bar."
require_pattern 'const[[:space:]]+qint64[[:space:]]+runningSeconds[[:space:]]*=' "$scanner_source" \
    "Session scanning must calculate cumulative running seconds."
require_pattern 'runtimeSeconds[[:space:]]*=' "$scanner_source" \
    "Detected sessions must persist computed runtime seconds."
require_pattern 'const[[:space:]]+double[[:space:]]+workingKeepAliveSeconds[[:space:]]*=' "$scanner_source" \
    "Status detection must use an explicit keep-alive window to reduce drift."
require_pattern 'secsSinceWork[[:space:]]*<[[:space:]]*workingKeepAliveSeconds' "$scanner_source" \
    "Working status should remain stable during the keep-alive window instead of dropping immediately."

echo "PASS: task-002 monitor status requirements are wired."
