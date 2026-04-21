#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
window_source="$repo_root/src/mainwindow.cpp"
scanner_source="$repo_root/src/utils/sessionscanner.cpp"

require_pattern() {
    local pattern="$1"
    local file="$2"
    local message="$3"

    if ! grep -Eq "$pattern" "$file"; then
        echo "FAIL: $message" >&2
        exit 1
    fi
}

require_pattern 'm_swimlaneRefreshTimer->stop\(' "$window_source" \
    "Swimlane refresh timer should stop when the tab is not actively in use."
require_pattern 'm_monitorRefreshTimer->stop\(' "$window_source" \
    "Monitor refresh timer should stop when the monitor tab is not actively in use."
require_pattern 'm_cachedSwimlaneWidget->deleteLater\(' "$window_source" \
    "Replacing swimlane cache must release the old widget tree."
require_pattern 'm_cachedMonitorWidget->deleteLater\(' "$window_source" \
    "Replacing monitor cache must release the old widget tree."
require_pattern 'swimlaneCacheTtlSeconds' "$window_source" \
    "Swimlane data collection should use a short-lived cache instead of rescanning on every visible tick."
require_pattern 'm_swimlaneDataCache[[:space:]]*=' "$window_source" \
    "Explicit swimlane refresh should invalidate cached data."
require_pattern 'class[[:space:]]+SwimlaneQueueWidget' "$window_source" \
    "Swimlane queues should render through a lightweight custom widget instead of a frame per cell."
require_pattern 'class[[:space:]]+MonitorTreeDelegate' "$window_source" \
    "Monitor rows should use delegate painting for status and actions instead of setItemWidget-heavy cells."
require_pattern 'ensurePreviewTabContent\(m_tabWidget->currentIndex\(\)\)' "$window_source" \
    "Preview refresh should only hydrate the currently visible detail tab."
require_pattern 'connect\(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::ensurePreviewTabContent\)' "$window_source" \
    "Preview detail tabs should lazily build content when the user switches to them."
require_pattern 'm_gitStatusCache' "$repo_root/src/mainwindow.h" \
    "MainWindow should keep a short-lived Git status cache to reduce repeated scans."
require_pattern 'm_aiToolCache' "$repo_root/src/mainwindow.h" \
    "MainWindow should keep a short-lived AI tool cache to reduce repeated scans."
require_pattern 'readArtifactTail\(const QString& sessionPath, qint64 maxBytes = 131072\)' "$repo_root/src/utils/sessionscanner.h" \
    "Session artifact tail reads should use a smaller default budget to reduce scan memory."

echo "PASS: memory footprint guard rails are wired."
