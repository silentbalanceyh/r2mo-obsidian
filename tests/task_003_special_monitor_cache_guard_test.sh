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

require_pattern 'mergeSpecialMonitorSnapshots' "$header" \
    "Special billing refresh must expose a merge helper so failed refresh rows can preserve cached values."
require_pattern 'freshSnapshot\.errorMessage\.isEmpty\(\)' "$source" \
    "Special billing cache merge must distinguish successful snapshots from transient fetch errors."
require_pattern 'cachedByKey' "$source" \
    "Special billing cache merge must index previous rows by stable provider/token key."
require_pattern 'm_specialMonitorDataCache\.data[[:space:]]*=[[:space:]]*mergedSnapshots' "$source" \
    "Special billing refresh completion must cache merged snapshots, not raw fetch results."
forbid_pattern 'm_specialMonitorDataCache\.data[[:space:]]*=[[:space:]]*m_specialMonitorScanWatcher->result\(\)' "$source" \
    "Special billing refresh completion must not overwrite cached values with raw failed snapshots."

echo "PASS: special monitor cache guard rails are wired."
