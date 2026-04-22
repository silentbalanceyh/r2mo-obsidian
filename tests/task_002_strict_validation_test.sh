#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
scanner_source="$repo_root/src/utils/sessionscanner.cpp"
remote_scanner_source="$repo_root/src/utils/remotesessionscanner.cpp"
window_source="$repo_root/src/mainwindow.cpp"
window_header="$repo_root/src/mainwindow.h"
settings_source="$repo_root/src/utils/settingsmanager.cpp"
task_file="$repo_root/.r2mo/task/task-002.md"

pass_count=0

run_case() {
    local name="$1"
    shift
    printf '[%02d/22] %s\n' "$((pass_count + 1))" "$name"
    "$@"
    pass_count=$((pass_count + 1))
}

require_pattern() {
    local pattern="$1"
    local file="$2"
    local message="$3"
    if ! grep -Eq "$pattern" "$file"; then
        echo "FAIL: $message" >&2
        exit 1
    fi
}

run_case "Task-002 Codex status guards" \
    bash "$repo_root/tests/task_002_codex_status_guard_test.sh"

run_case "Task-002 local monitor path guards" \
    bash "$repo_root/tests/task_002_local_monitor_path_guard_test.sh"

run_case "Task-002 monitor status guards" \
    bash "$repo_root/tests/task_002_monitor_status_test.sh"

run_case "Task-002 OpenCode guards" \
    bash "$repo_root/tests/task_002_opencode_session_guard_test.sh"

run_case "Task-002 remote process-root guards" \
    bash "$repo_root/tests/task_002_remote_process_root_guard_test.sh"

run_case "Task-002 remote repository selector guards" \
    bash "$repo_root/tests/task_002_remote_repository_selector_guard_test.sh"

run_case "Task-002 terminal icon guards" \
    bash "$repo_root/tests/task_002_terminal_icon_guard_test.sh"

run_case "Remote monitor scanner guards" \
    bash "$repo_root/tests/task_001_remote_monitor_session_test.sh"

run_case "Special monitor refresh guards" \
    bash "$repo_root/tests/task_002_special_monitor_refresh_test.sh"

run_case "Minimum font size guards" \
    bash "$repo_root/tests/min_font_size_test.sh"

run_case "Build configuration" \
    cmake -B "$repo_root/build"

run_case "Build compilation" \
    cmake --build "$repo_root/build" -j4

run_case "Codex log indexing present" \
    grep -Eq 'collectCodexSessionLogRecords' "$scanner_source"

run_case "Codex freshness window present" \
    grep -Eq 'codexActiveFreshSeconds' "$scanner_source"

run_case "Claude artifact parsing present" \
    grep -Eq 'inferClaudeArtifactStatus' "$scanner_source"

run_case "OpenCode sqlite path present" \
    grep -Eq '\.local/share/opencode/opencode\.db' "$scanner_source"

run_case "OpenCode local multi-pair logic present" \
    grep -Eq 'assignCount[[:space:]]*=[[:space:]]*qMin\(sortedIndexes\.size\(\),[[:space:]]*candidateIds\.size\(\)\)' "$scanner_source"

run_case "Remote candidate ranking present" \
    grep -Eq 'isBetterRemoteSessionCandidate' "$remote_scanner_source"

run_case "Remote vendor preference present" \
    grep -Eq 'detailText\.contains\("/vendor/"\)' "$remote_scanner_source"

run_case "Monitor runtime formatting wired" \
    grep -Eq 'QString[[:space:]]+formatSessionRuntime[[:space:]]*\([[:space:]]*qint64[[:space:]]+runtimeSeconds[[:space:]]*\)[[:space:]]+const' "$window_header"

run_case "Monitor header version current" \
    grep -Eq 'monitorBoardHeaderState\.v4' "$settings_source"

run_case "Task-002 change log appended" \
    bash -lc "grep -Eq '^## Changes$' '$task_file' && grep -Eq '\\[Team Leader\\].*task-002' '$task_file'"

run_case "Local OpenCode session exists in sqlite" \
    python3 - "$repo_root" <<'PY'
import sqlite3
import sys

repo_root = sys.argv[1]
db = sqlite3.connect("/Users/lang/.local/share/opencode/opencode.db")
cur = db.cursor()
cur.execute(
    """
    select count(1)
    from session s
    join project p on p.id = s.project_id
    where p.worktree = ?
      and s.time_archived is null
    """,
    (repo_root,),
)
count = cur.fetchone()[0]
db.close()
raise SystemExit(0 if count >= 1 else 1)
PY

run_case "Remote SSH probe returns live monitor tools" \
    bash -lc "ssh -o BatchMode=yes -o StrictHostKeyChecking=no lang@mxt.webos.cn \"python3 - /media/psf/r2mo-apps/app-webos\" < /tmp/remote_opencode_probe.py | grep -Eq 'Codex|Claude|OpenCode'"

echo "PASS: 22/22 strict task-002 validation cases succeeded."
