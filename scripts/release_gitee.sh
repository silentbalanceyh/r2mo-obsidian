#!/usr/bin/env bash
set -euo pipefail

# Usage:
#   scripts/release_gitee.sh v1.0.0
# Optional env:
#   REPO=silentbalanceyh/r2mo-obsidian
#   BRANCH=master
#   WORKFLOW=gitee-release-ci.yml
#
# Prerequisites:
# - gh CLI installed and authenticated
# - GitHub repo secret: GITEE_TOKEN
# - GitHub repo variables: GITEE_OWNER, GITEE_REPO (optional, have defaults in workflow)

if [ "$#" -lt 1 ]; then
  echo "Usage: $0 <version-tag>"
  exit 1
fi

VERSION="$1"
REPO="${REPO:-silentbalanceyh/r2mo-obsidian}"
BRANCH="${BRANCH:-master}"
WORKFLOW="${WORKFLOW:-gitee-release-ci.yml}"

echo "Dispatch workflow: ${WORKFLOW}"
echo "Repo: ${REPO}"
echo "Branch: ${BRANCH}"
echo "Version: ${VERSION}"

gh workflow run "${WORKFLOW}" -R "${REPO}" -r "${BRANCH}" -f "version=${VERSION}"

sleep 3

RUN_ID="$(gh run list \
  --workflow "${WORKFLOW}" \
  -R "${REPO}" \
  --branch "${BRANCH}" \
  --event workflow_dispatch \
  --limit 1 \
  --json databaseId \
  --jq '.[0].databaseId')"

if [ -z "${RUN_ID}" ] || [ "${RUN_ID}" = "null" ]; then
  echo "Failed to get workflow run id"
  exit 1
fi

echo "Watching run: ${RUN_ID}"
gh run watch "${RUN_ID}" -R "${REPO}" --interval 20

echo "Run finished. Summary:"
gh run view "${RUN_ID}" -R "${REPO}" --json status,conclusion,url
