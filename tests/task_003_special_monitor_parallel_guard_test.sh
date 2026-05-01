#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source_file="$repo_root/src/utils/specialmonitorfetcher.cpp"

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

require_pattern '#include <future>' "$source_file" \
    "Special billing fetcher should use per-source futures so slow providers cannot block all other rows."
require_pattern 'std::async\(std::launch::async' "$source_file" \
    "Special billing sources should be fetched concurrently."
require_pattern 'snapshotFutures' "$source_file" \
    "Special billing fetcher should collect per-source futures and preserve source order."
forbid_pattern 'for[[:space:]]*\\([^)]*SpecialMonitorSource[^)]*: sources\\)[[:space:]]*\\{[[:space:]]*const QString normalizedUrl' "$source_file" \
    "Special billing fetcher must not process network sources serially inside a single loop."

echo "PASS: special monitor fetches sources concurrently."
