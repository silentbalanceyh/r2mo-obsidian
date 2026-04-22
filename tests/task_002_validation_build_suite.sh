#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

cmake -B "$repo_root/build"
cmake --build "$repo_root/build" -j4

echo "PASS: task-002 build validation suite succeeded."
