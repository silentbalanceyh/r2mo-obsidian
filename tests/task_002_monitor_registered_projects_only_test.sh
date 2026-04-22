#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source="$repo_root/src/mainwindow.cpp"
collect_body="$(mktemp)"
trap 'rm -f "$collect_body"' EXIT

awk '
    /QList<QPair<QString, QString>> MainWindow::collectAllProjectPaths\(\) const/ { capture = 1 }
    capture { print }
    capture && /^void MainWindow::onMonitorBoard\(\)/ { exit }
' "$source" > "$collect_body"

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

require_pattern 'QList<QPair<QString,[[:space:]]*QString>>[[:space:]]+MainWindow::collectAllProjectPaths\(\)[[:space:]]+const' "$collect_body" \
    "Monitor board must centralize its visible project source list in collectAllProjectPaths()."
require_pattern 'QList<Vault>[[:space:]]+vaults[[:space:]]*=[[:space:]]*m_vaultModel->vaults\(\);' "$collect_body" \
    "Monitor board should derive visible projects from the registered repository list."
require_pattern 'result\.append\(qMakePair\(vault\.name,[[:space:]]*normalizedVaultPath\)\);' "$collect_body" \
    "Registered repository rows must still flow into the monitor board."
forbid_pattern 'R2moScanner[[:space:]]+scanner;' "$collect_body" \
    "Monitor board must not expand extra subprojects beyond the registered repository list."
forbid_pattern 'scanner\.scanVault\(vault\.path\)' "$collect_body" \
    "Monitor board must not scan vault children when building the visible project list."

echo "PASS: monitor board is restricted to registered repository rows."
