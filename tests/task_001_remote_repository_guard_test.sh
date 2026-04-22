#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
bash "$repo_root/tests/task_001_ui_bug_guard_test.sh"
