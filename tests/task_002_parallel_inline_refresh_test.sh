#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
window_header="$repo_root/src/mainwindow.h"
window_source="$repo_root/src/mainwindow.cpp"
fetcher_header="$repo_root/src/utils/specialmonitorfetcher.h"
fetcher_source="$repo_root/src/utils/specialmonitorfetcher.cpp"

require_pattern() {
    local pattern="$1"
    local file="$2"
    local message="$3"

    if ! grep -Eq "$pattern" "$file"; then
        echo "FAIL: $message" >&2
        exit 1
    fi
}

require_pattern 'QList<ProjectMonitorData>[[:space:]]+collectMonitorData[[:space:]]*\([[:space:]]*\)[[:space:]]+const' "$window_header" \
    "Monitor board should expose a single data collector that can coordinate local and remote scans."
require_pattern 'localFuture[[:space:]]*=[[:space:]]*QtConcurrent::run' "$window_source" \
    "Monitor board should scan local project sessions in a background future."
require_pattern 'remoteFuture[[:space:]]*=[[:space:]]*QtConcurrent::run' "$window_source" \
    "Monitor board should scan SSH remote project sessions in a parallel background future."
require_pattern 'QList<QFuture<QList<ProjectMonitorData>>>[[:space:]]+futures' "$window_source" \
    "Remote monitor scans should fan out per connected remote project instead of scanning SSH hosts serially."
require_pattern 'RemoteSessionScanner[[:space:]]+scanner;' "$window_source" \
    "Each remote scan task should own its scanner instance inside the worker."
require_pattern 'QList<QFuture<SpecialMonitorSnapshot>>[[:space:]]+futures' "$fetcher_source" \
    "Special billing refresh should fan out one future per token source."
require_pattern 'SpecialMonitorSnapshot[[:space:]]+fetchSnapshot[[:space:]]*\([[:space:]]*const[[:space:]]+SpecialMonitorSource&[[:space:]]+source[[:space:]]*\)[[:space:]]+const' "$fetcher_header" \
    "Special billing fetcher should expose a per-source fetch unit for parallel execution."
require_pattern 'kMonitorLoadingRole' "$window_source" \
    "Monitor table rows should carry an inline loading state."
require_pattern 'createMonitorLoadingIcon' "$window_source" \
    "Monitor table should use a compact leading loading icon instead of status text."
require_pattern 'const[[:space:]]+bool[[:space:]]+rowLoading[[:space:]]*=[[:space:]]*loading[[:space:]]*&&[[:space:]]*m_monitorLoadingProjects\.contains\(projectPath\)' "$window_source" \
    "Monitor refresh loading should mark only still-refreshing project rows instead of showing a full-table overlay."
require_pattern 'overlay->hide\(\)' "$window_source" \
    "Refresh overlays should be kept hidden when inline loading is active."
require_pattern 'updatedItem->setText\(tr\("Refreshing\.\.\."\)\)' "$window_source" \
    "Special billing refresh should show row-level refreshing text in the table."

echo "PASS: parallel inline refresh guard rails are wired."
