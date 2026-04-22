#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
remote_scanner_source="$repo_root/src/utils/remotesessionscanner.cpp"

require_pattern() {
    local pattern="$1"
    local file="$2"
    local message="$3"

    if ! grep -Eq "$pattern" "$file"; then
        echo "FAIL: $message" >&2
        exit 1
    fi
}

require_pattern 'if tool == '\''Codex'\'':' "$remote_scanner_source" \
    "Remote scanner must keep a dedicated Codex artifact-status branch."
require_pattern 'if typ == '\''response_item'\'':' "$remote_scanner_source" \
    "Remote scanner must parse Codex response_item events."
require_pattern 'payload_type in \('\''function_call'\'', '\''custom_tool_call'\'', '\''reasoning'\''\)' "$remote_scanner_source" \
    "Remote scanner must treat live Codex response_item reasoning/tool calls as active work."
require_pattern 'payload_type == '\''message'\''' "$remote_scanner_source" \
    "Remote scanner must inspect Codex response_item message phases."
require_pattern 'phase == '\''commentary'\''' "$remote_scanner_source" \
    "Remote scanner must treat Codex commentary phases as working when fresh."
require_pattern 'return '\''working'\'' if event_is_fresh else '\''ready'\''' "$remote_scanner_source" \
    "Remote scanner must downgrade stale live events to ready instead of forcing ready immediately."
require_pattern 'mtime_is_fresh = \(now - os\.path\.getmtime\(path\)\) <= 8' "$remote_scanner_source" \
    "Remote scanner must keep a fresh-mtime fallback for active Codex sessions."
require_pattern 'if typ == '\''task_complete'\'':' "$remote_scanner_source" \
    "Remote scanner must still inspect task_complete events explicitly."
require_pattern 'return '\''working'\'' if mtime_is_fresh else '\''ready'\''' "$remote_scanner_source" \
    "Remote scanner must keep a short working grace period when Codex just wrote task_complete."
require_pattern 'def process_cpu_ticks\(pid\):' "$remote_scanner_source" \
    "Remote scanner must sample remote process CPU ticks for live-session fallback."
require_pattern 'cpu_samples_before' "$remote_scanner_source" \
    "Remote scanner must capture a pre-sleep CPU sample set."
require_pattern 'time\.sleep\(2\.0\)' "$remote_scanner_source" \
    "Remote scanner must use a long enough live-sampling window to observe remote CPU tick changes."
require_pattern 'cpu_delta > 0' "$remote_scanner_source" \
    "Remote scanner must mark sessions working when remote process CPU ticks advance."
require_pattern 'obj\.get\('\''cwd'\''\)' "$remote_scanner_source" \
    "Remote Claude artifact lookup must inspect recorded cwd values inside Claude session logs."
require_pattern 'path_belongs_to_project\(cwd, project\) or path_belongs_to_project\(project, cwd\)' "$remote_scanner_source" \
    "Remote Claude artifact lookup must match nested project paths in either direction."
require_pattern "glob\\.glob\\(os\\.path\\.expanduser\\('~/.claude/projects/\\*\\*/\\*\\.jsonl'\\), recursive=True\\)" "$remote_scanner_source" \
    "Remote Claude artifact lookup must search Claude session files recursively instead of relying on one encoded folder guess."
require_pattern "if '/subagents/' in path and session_id:" "$remote_scanner_source" \
    "Remote Claude artifact lookup must recognize subagent session files explicitly."
require_pattern "primary_path = os\\.path\\.join\\(os\\.path\\.dirname\\(os\\.path\\.dirname\\(path\\)\\), session_id \\+ '\\.jsonl'\\)" "$remote_scanner_source" \
    "Remote Claude artifact lookup must resolve the owning top-level Claude session file from a subagent path."
if grep -Eq 'if not found_opencode_row:' "$remote_scanner_source"; then
    echo "FAIL: Remote scanner must not fabricate OpenCode rows from stale artifacts when no OpenCode process is running." >&2
    exit 1
fi

echo "PASS: remote status guard rails are wired."
