#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

bash "$repo_root/tests/task_002_codex_status_guard_test.sh"
bash "$repo_root/tests/task_002_monitor_refresh_cadence_test.sh"
bash "$repo_root/tests/task_002_monitor_incremental_fill_test.sh"
bash "$repo_root/tests/task_002_opencode_status_guard_test.sh"
bash "$repo_root/tests/task_002_parallel_inline_refresh_test.sh"
bash "$repo_root/tests/task_002_monitor_layout_height_test.sh"
bash "$repo_root/tests/task_002_monitor_status_test.sh"
bash "$repo_root/tests/task_002_opencode_session_guard_test.sh"
bash "$repo_root/tests/task_002_remote_repository_selector_guard_test.sh"
bash "$repo_root/tests/task_002_terminal_icon_guard_test.sh"
bash "$repo_root/tests/task_002_remote_status_guard_test.sh"
bash "$repo_root/tests/task_001_remote_monitor_session_test.sh"
bash "$repo_root/tests/task_002_special_monitor_refresh_test.sh"
bash "$repo_root/tests/min_font_size_test.sh"

echo "PASS: task-002 guard validation suite succeeded."
