#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source="$repo_root/src/mainwindow.cpp"
scan_body="$(mktemp)"
display_body="$(mktemp)"
refresh_body="$(mktemp)"
batch_body="$(mktemp)"
trap 'rm -f "$scan_body" "$display_body" "$refresh_body" "$batch_body"' EXIT

awk '
    /QList<QPair<QString, QString>> MainWindow::collectAllProjectPaths\(\) const/ { capture = 1 }
    capture { print }
    capture && /^void MainWindow::onMonitorBoard\(\)/ { exit }
' "$source" > "$scan_body"

awk '
    /QList<QPair<QString, QString>> MainWindow::collectMonitorDisplayProjectPaths\(\) const/ { capture = 1 }
    capture { print }
    capture && /^QList<QPair<QString, QString>> MainWindow::collectAllProjectPaths\(\) const/ { exit }
' "$source" > "$display_body"

awk '
    /void MainWindow::startMonitorRefresh\(bool initialOpen\)/ { capture = 1 }
    capture { print }
    capture && /^void MainWindow::applyMonitorRefreshBatch/ { exit }
' "$source" > "$refresh_body"

awk '
    /void MainWindow::applyMonitorRefreshBatch/ { capture = 1 }
    capture { print }
    capture && /^void MainWindow::finishMonitorRefresh/ { exit }
' "$source" > "$batch_body"

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

require_pattern 'QList<QPair<QString,[[:space:]]*QString>>[[:space:]]+MainWindow::collectMonitorDisplayProjectPaths\(\)[[:space:]]+const' "$display_body" \
    "Monitor board must keep a display-only project source list."
require_pattern 'QList<Vault>[[:space:]]+vaults[[:space:]]*=[[:space:]]*m_vaultModel->vaults\(\);' "$display_body" \
    "Monitor board should derive visible projects from the registered repository list."
require_pattern 'result\.append\(qMakePair\(vault\.name,[[:space:]]*normalizedVaultPath\)\);' "$display_body" \
    "Registered repository rows must still flow into the monitor board."
forbid_pattern 'R2moScanner[[:space:]]+scanner;' "$display_body" \
    "Visible monitor rows must not expand extra subprojects beyond the registered repository list."
require_pattern 'R2moScanner[[:space:]]+scanner;' "$scan_body" \
    "Monitor scanning should still include R2MO subproject paths for accurate session and artifact detection."
require_pattern 'scanner\.scanVault\(vault\.path\)' "$scan_body" \
    "Monitor scanning should keep child project context separate from visible row selection."
require_pattern 'const[[:space:]]+QList<QPair<QString,[[:space:]]*QString>>[[:space:]]+displayProjects[[:space:]]*=[[:space:]]*collectMonitorDisplayProjectPaths\(\);' "$refresh_body" \
    "Monitor refresh should initialize rows from the display-only project list."
require_pattern 'const[[:space:]]+QList<QPair<QString,[[:space:]]*QString>>[[:space:]]+scanProjects[[:space:]]*=[[:space:]]*collectAllProjectPaths\(\);' "$refresh_body" \
    "Monitor refresh should scan with the full project context."
require_pattern 'scanner\.scanLiveSessions\(scanProjects\)' "$refresh_body" \
    "Local session scanning must use the full scan context, not only visible rows."
require_pattern 'QMap<QString,[[:space:]]*ProjectMonitorData>[[:space:]]+localDisplayData;' "$refresh_body" \
    "Local scan results should be grouped by visible project before clearing row loading."
require_pattern 'appendToDisplayBatch\(localDisplayData,[[:space:]]*projectData\)' "$refresh_body" \
    "Local child-project rows must be folded into their visible parent batch."
require_pattern 'QMap<QString,[[:space:]]*ProjectMonitorData>[[:space:]]+remoteDisplayData;' "$refresh_body" \
    "Remote scan results should be grouped by visible project before clearing row loading."
require_pattern 'collectMonitorDisplayProjectPaths\(\)' "$batch_body" \
    "Partial monitor batches must be folded back into visible project rows."
require_pattern 'monitorDisplayProjectForPath' "$batch_body" \
    "Subproject monitor data must be mapped back to its registered parent row."

echo "PASS: monitor board is restricted to registered repository rows."
