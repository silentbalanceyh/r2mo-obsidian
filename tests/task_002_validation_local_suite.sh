#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
scanner_source="$repo_root/src/utils/sessionscanner.cpp"
window_header="$repo_root/src/mainwindow.h"
settings_source="$repo_root/src/utils/settingsmanager.cpp"
task_file="$repo_root/.r2mo/task/task-002.md"

grep -Eq 'collectCodexSessionLogRecords' "$scanner_source"
grep -Eq 'codexActiveFreshSeconds' "$scanner_source"
grep -Eq 'inferClaudeArtifactStatus' "$scanner_source"
grep -Eq '\.local/share/opencode/opencode\.db' "$scanner_source"
grep -Eq 'assignCount[[:space:]]*=[[:space:]]*qMin\(sortedIndexes\.size\(\),[[:space:]]*candidateIds\.size\(\)\)' "$scanner_source"
grep -Eq 'QString[[:space:]]+formatSessionRuntime[[:space:]]*\([[:space:]]*qint64[[:space:]]+runtimeSeconds[[:space:]]*\)[[:space:]]+const' "$window_header"
grep -Eq 'monitorBoardHeaderState\.v4' "$settings_source"
grep -Eq '^## Changes$' "$task_file"
grep -Eq '\[Team Leader\].*task-002' "$task_file"

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

echo "PASS: task-002 local validation suite succeeded."
