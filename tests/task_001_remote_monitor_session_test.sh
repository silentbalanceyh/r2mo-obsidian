#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
window_source="$repo_root/src/mainwindow.cpp"
window_header="$repo_root/src/mainwindow.h"
scanner_header="$repo_root/src/utils/remotesessionscanner.h"
scanner_source="$repo_root/src/utils/remotesessionscanner.cpp"
cmake_file="$repo_root/CMakeLists.txt"

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

require_pattern 'class[[:space:]]+RemoteSessionScanner' "$scanner_header" \
    "Remote Codex/Claude monitoring must be implemented in a dedicated remote session scanner."
require_pattern 'QList<ProjectMonitorData>[[:space:]]+scanRemoteVault' "$scanner_header" \
    "Remote session scanner must expose a structured monitor-data entry point."
require_pattern 'python3[[:space:]]*-[[:space:]]*%1[[:space:]]*<<' "$scanner_source" \
    "Remote session scanner must query the remote host with a structured Python probe."
require_pattern 'Codex' "$scanner_source" \
    "Remote session scanner must recognize remote Codex sessions."
require_pattern 'Claude' "$scanner_source" \
    "Remote session scanner must recognize remote Claude sessions."
require_pattern 'OpenCode' "$scanner_source" \
    "Remote session scanner must recognize remote OpenCode sessions."
require_pattern "\\.omc.*sessions|os\\.path\\.join\\(project, '\\.omc', 'sessions'" "$scanner_source" \
    "Remote OpenCode monitoring must inspect project-local .omc session artifacts."
require_pattern 'session_id' "$scanner_source" \
    "Remote OpenCode monitoring must extract the recorded session_id field."
require_pattern 'collectRemoteMonitorData' "$window_header" \
    "MainWindow must keep a dedicated remote-monitor data collection path."
require_pattern 'RemoteSessionScanner' "$window_source" \
    "MainWindow must delegate remote session discovery to the remote scanner."
forbid_pattern 'session\.toolName[[:space:]]*=[[:space:]]*tr\("SSH"\)' "$window_source" \
    "Remote monitor rows must no longer collapse all remote activity into a generic SSH placeholder."
require_pattern 'src/utils/remotesessionscanner.cpp' "$cmake_file" \
    "CMake must compile the remote session scanner."

echo "PASS: task-001 remote monitor session scanning is wired."
