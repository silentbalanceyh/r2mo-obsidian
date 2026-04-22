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

require_pattern 'void[[:space:]]+startMonitorRefresh[[:space:]]*\([[:space:]]*bool[[:space:]]+initialOpen[[:space:]]*=[[:space:]]*false[[:space:]]*\)' "$header" \
    "Monitor board should expose a shared refresh entry point for initial open and manual refresh."
require_pattern 'void[[:space:]]+applyMonitorRefreshBatch[[:space:]]*\([[:space:]]*int[[:space:]]+generation[[:space:]]*,[[:space:]]*const[[:space:]]+QList<ProjectMonitorData>&[[:space:]]+data[[:space:]]*\)' "$header" \
    "Monitor board should accept partial refresh batches as projects finish."
require_pattern 'void[[:space:]]+finishMonitorRefresh[[:space:]]*\([[:space:]]*int[[:space:]]+generation[[:space:]]*\)' "$header" \
    "Monitor board should finalize refresh state after all project batches are delivered."
require_pattern 'QMap<QString,[[:space:]]*ProjectMonitorData>[[:space:]]+m_monitorProjectCache;' "$header" \
    "Monitor board should retain the latest project snapshots so partial batches can merge into the view."
require_pattern 'QSet<QString>[[:space:]]+m_monitorLoadingProjects;' "$header" \
    "Monitor board should track which projects are still refreshing so row-level loading can clear project by project."
require_pattern 'QMetaObject::invokeMethod' "$source" \
    "Background monitor workers should queue partial UI updates back onto the main thread."
require_pattern 'applyMonitorRefreshBatch\(generation,[[:space:]]*\{projectData\}\)' "$source" \
    "Each finished monitor worker should merge its project batch immediately."
require_pattern 'finishMonitorRefresh\(m_monitorRefreshGeneration\)' "$source" \
    "Monitor refresh should only clear loading state after the generation has fully completed."

echo "PASS: incremental monitor backfill guard rails are wired."
