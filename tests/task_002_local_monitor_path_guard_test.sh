#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
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

require_pattern 'QFileInfo[[:space:]]+vaultInfo\(vault\.path\);' "$window_source" \
    "Monitor project collection must inspect the configured vault path before grouping sessions."
require_pattern 'vaultInfo\.fileName\(\)[[:space:]]*==[[:space:]]*QStringLiteral\("\.r2mo"\)' "$window_source" \
    "Monitor project collection must detect vault entries that point at a .r2mo directory."
require_pattern 'normalizedVaultPath[[:space:]]*=[[:space:]]*QDir::cleanPath\(vaultInfo\.path\(\)\);' "$window_source" \
    "Monitor project collection must normalize .r2mo-based vault entries back to the project root."

echo "PASS: local monitor path normalization guard is present."
