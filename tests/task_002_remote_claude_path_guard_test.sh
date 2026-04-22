#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
scanner_source="$repo_root/src/utils/remotesessionscanner.cpp"

require_pattern() {
    local pattern="$1"
    local file="$2"
    local message="$3"

    if ! grep -Eq "$pattern" "$file"; then
        echo "FAIL: $message" >&2
        exit 1
    fi
}

require_pattern 'def claude_project_dir_candidates\(project\):' "$scanner_source" \
    "Remote Claude scanning must build reusable project-dir candidates."
require_pattern 're\.sub\(r'\''-\+'\''[[:space:]]*,[[:space:]]*'\''-'\''[[:space:]]*,[[:space:]]*sanitized\)' "$scanner_source" \
    "Remote Claude scanning must collapse punctuation-heavy path fragments to a stable dashed project dir."
require_pattern 'for project_dir in claude_project_dir_candidates\(candidate\):' "$scanner_source" \
    "Remote Claude scanning must probe multiple encoded project-dir candidates for each path."

echo "PASS: remote Claude project-dir compatibility guard is present."
