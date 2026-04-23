#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
scanner_source="$repo_root/src/utils/sessionscanner.cpp"

python3 - "$scanner_source" <<'PY'
import pathlib
import re
import sys

text = pathlib.Path(sys.argv[1]).read_text()

open_code_index = text.find('if (toolName == "OpenCode") {')
session_path_guard_index = text.find('if (sessionPath.isEmpty()) {')
if open_code_index < 0:
    print("FAIL: OpenCode artifact inference branch must exist.", file=sys.stderr)
    raise SystemExit(1)
if session_path_guard_index < 0:
    print("FAIL: Generic sessionPath guard must exist.", file=sys.stderr)
    raise SystemExit(1)
if open_code_index > session_path_guard_index:
    print("FAIL: OpenCode global status must be checked before sessionPath emptiness short-circuits inference.", file=sys.stderr)
    raise SystemExit(1)

if not re.search(
    r'SessionStatus SessionScanner::determineStatus\([\s\S]*?'
    r'const SessionStatus artifactStatus = inferArtifactStatus\(toolName, sessionId, sessionPath\);[\s\S]*?'
    r'if \(artifactStatus != SessionStatus::Unknown\) \{[\s\S]*?return artifactStatus;[\s\S]*?'
    r'if \(toolName == "OpenCode"\) \{[\s\S]*?return SessionStatus::Ready;',
    text,
):
    print("FAIL: OpenCode local status must fall back to Ready instead of CPU heuristics when no fresh active event exists.", file=sys.stderr)
    raise SystemExit(1)

print("PASS: task-002 local OpenCode status guard rails are wired.")
PY
