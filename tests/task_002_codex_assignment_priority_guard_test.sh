#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
scanner_source="$repo_root/src/utils/sessionscanner.cpp"

python3 - "$scanner_source" <<'PY'
import pathlib
import sys

text = pathlib.Path(sys.argv[1]).read_text()

artifact_marker = 'QList<CodexSessionArtifact> matchedArtifacts;'
log_marker = 'const QList<CodexLogSessionRecord> projectLogs ='

artifact_index = text.find(artifact_marker)
log_index = text.find(log_marker)

if artifact_index < 0:
    print("FAIL: Codex session assignment must have a project-artifact matching pass.", file=sys.stderr)
    raise SystemExit(1)
if log_index < 0:
    print("FAIL: Codex session assignment must have a log fallback pass.", file=sys.stderr)
    raise SystemExit(1)
if artifact_index > log_index:
    print("FAIL: Codex project artifacts must be assigned before log fallback so live rows don't bind to stale sessions.", file=sys.stderr)
    raise SystemExit(1)

print("PASS: task-002 Codex assignment priority guard rails are wired.")
PY
