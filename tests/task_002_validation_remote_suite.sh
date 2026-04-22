#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
remote_scanner_source="$repo_root/src/utils/remotesessionscanner.cpp"

grep -Eq 'isBetterRemoteSessionCandidate' "$remote_scanner_source"
grep -Eq 'detailText\.contains\("/vendor/"\)' "$remote_scanner_source"
ssh -o BatchMode=yes -o StrictHostKeyChecking=no lang@mxt.webos.cn \
  "python3 - /media/psf/r2mo-apps/app-webos" < /tmp/remote_opencode_probe.py | grep -Eq 'Codex|Claude|OpenCode'

echo "PASS: task-002 remote validation suite succeeded."
