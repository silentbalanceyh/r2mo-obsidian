#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source_file="$repo_root/src/utils/specialmonitorfetcher.cpp"
main_window="$repo_root/src/mainwindow.cpp"

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

require_pattern 'resolveTokenKey' "$source_file" \
    "Special billing fetcher must resolve environment variable token keys before sending requests."
require_pattern 'qgetenv' "$source_file" \
    "DeepSeek token entries like OPEN_DS_KEY_MY must be expanded from the process environment."
require_pattern 'deepSeekAuthHeader\(tokenKey\)' "$source_file" \
    "DeepSeek auth headers must be normalized through a helper."
require_pattern 'startsWith\(QStringLiteral\("Bearer "\), Qt::CaseInsensitive\)' "$source_file" \
    "DeepSeek platform tokens pasted with a Bearer prefix must not be double-prefixed."
require_pattern 'return trimmed\.toUtf8\(\);' "$source_file" \
    "DeepSeek Bearer tokens should be passed through as-is after trimming."
require_pattern 'root\.value\(QStringLiteral\("code"\)\)\.toInt\(-1\)[[:space:]]*!=[[:space:]]*0' "$source_file" \
    "DeepSeek platform summary must inspect the JSON business code, not only HTTP status."
require_pattern 'SpecialMonitorSnapshot balanceSnapshot[[:space:]]*=[[:space:]]*fetchDeepSeekBalanceSnapshot\(' "$source_file" \
    "DeepSeek platform failures must fall back to the official balance endpoint."
require_pattern 'usageCostAmount,' "$source_file" \
    "DeepSeek balance fallback must receive the usage/cost amount."
require_pattern 'extractDeepSeekModelUsageCount\(usageAmountData\),' "$source_file" \
    "DeepSeek balance fallback must preserve usage/cost data so the Used column is not reset to zero."
forbid_pattern 'QStringLiteral\("需 Bearer"\)' "$source_file" \
    "DeepSeek balance fallback should display zero instead of an inline Bearer hint when usage data is unavailable."
require_pattern 'https://platform\.deepseek\.com/auth-api/v0/users/current' "$source_file" \
    "DeepSeek account names must come from the platform current-user endpoint."
require_pattern 'id_profile' "$source_file" \
    "DeepSeek account mapping must inspect id_profile.name."
require_pattern 'monthly_costs' "$source_file" \
    "DeepSeek consumption must be read from platform monthly_costs."
require_pattern 'const bool hasMonthlyCostAmount[[:space:]]*=[[:space:]]*usedAmount[[:space:]]*>[[:space:]]*0\.0' "$source_file" \
    "DeepSeek summary monthly_costs.amount should be the primary Used source."
require_pattern 'https://platform\.deepseek\.com/api/v0/usage/amount' "$source_file" \
    "DeepSeek usage-count extraction must request the platform usage amount endpoint."
require_pattern 'https://platform\.deepseek\.com/api/v0/usage/cost' "$source_file" \
    "DeepSeek cost extraction must request the platform usage cost endpoint."
require_pattern 'extractDeepSeekUsageCostAmount\(usageCostData\)' "$source_file" \
    "DeepSeek used amount must be read from platform usage/cost data so values like 5.13 are displayed."
require_pattern 'if[[:space:]]*\(!hasMonthlyCostAmount[[:space:]]*&&[[:space:]]*usageCostAmount[[:space:]]*>[[:space:]]*0\.0\)' "$source_file" \
    "DeepSeek usage/cost should only fill Used when summary monthly_costs.amount is unavailable."
require_pattern 'currentDate\.month\(\)' "$source_file" \
    "DeepSeek usage endpoints must use the current month."
require_pattern 'currentDate\.year\(\)' "$source_file" \
    "DeepSeek usage endpoints must use the current year."
require_pattern 'snapshot\.todayAddedQuota[[:space:]]*=[[:space:]]*formatMoneyAmount\(addedAmount,[[:space:]]*true\)' "$source_file" \
    "DeepSeek Today Added must be computed from remaining balance plus consumed amount."
require_pattern 'snapshot\.todayUsedQuota[[:space:]]*=[[:space:]]*formatMoneyAmount\(usedAmount,[[:space:]]*true\)' "$source_file" \
    "DeepSeek Today Used must display consumed amount, not token estimation."
require_pattern 'snapshot\.remainQuota[[:space:]]*=[[:space:]]*formatMoneyAmount\(remainAmount,[[:space:]]*true\)' "$source_file" \
    "DeepSeek Remain must display wallet balance amount, not token estimation."
require_pattern 'deepseek-v4-pro' "$source_file" \
    "DeepSeek usage-count extraction must explicitly look for the deepseek-v4-pro model when model usage data is available."
require_pattern 'typeValue[[:space:]]*==[[:space:]]*QStringLiteral\("REQUEST"\)' "$source_file" \
    "DeepSeek usage count must read usage entries whose type is REQUEST."
require_pattern 'jsonInt\(usageItem\.value\(QStringLiteral\("amount"\)\)\)' "$source_file" \
    "DeepSeek REQUEST usage count must come from the usage amount field."
require_pattern 'snapshot\.accountName[[:space:]]*=[[:space:]]*accountName\.trimmed\(\)\.isEmpty\(\)[[:space:]]*\?[[:space:]]*QStringLiteral\("DeepSeek"\)' "$source_file" \
    "DeepSeek fallback account must not use balance money as the account name."
forbid_pattern 'snapshot\.accountName[[:space:]]*=[^;]*QStringLiteral\("¥%1"\)' "$source_file" \
    "DeepSeek account mapping must not put balance money into the account column."
forbid_pattern 'snapshot\.accountName[[:space:]]*=[^;]*QStringLiteral\("￥%1"\)' "$source_file" \
    "DeepSeek account mapping must not put balance money into the account column."
forbid_pattern 'total_available_token_estimation' "$source_file" \
    "DeepSeek special monitor should not treat token estimation as the displayed amount fields."
forbid_pattern 'current_token' "$source_file" \
    "DeepSeek special monitor should not use current_token as the displayed amount fields."
require_pattern 'extractDeepSeekModelUsageCount\(usageAmountData\)' "$source_file" \
    "DeepSeek usage-count extraction should be wired into the snapshot mapping."
require_pattern 'formatMoneyAmount\(addedAmount,[[:space:]]*true\)' "$source_file" \
    "DeepSeek added amount should preserve decimal formatting."
require_pattern 'tr\("Σ Total"\)' "$main_window" \
    "Special billing table should rename Today Added to Total."
require_pattern 'tr\("🔥 Used"\)' "$main_window" \
    "Special billing table should rename Today Used to Used."
require_pattern 'tr\("💰 Balance"\)' "$main_window" \
    "Special billing table should rename Remain to Balance."
require_pattern 'int providerWidth[[:space:]]*=[[:space:]]*qBound\(96,[[:space:]]*viewportWidth / 14,[[:space:]]*132\)' "$main_window" \
    "Special billing Provider column should be narrower."
require_pattern 'int typeWidth[[:space:]]*=[[:space:]]*qBound\(150,[[:space:]]*viewportWidth / 8,[[:space:]]*240\)' "$main_window" \
    "Special billing Type column should be wider to avoid compressed unit text."
require_pattern 'QStringLiteral\("DeepSeek / 累积"\)' "$source_file" \
    "DeepSeek package type should show the cumulative billing unit."
require_pattern 'https://api\.deepseek\.com/user/balance' "$main_window" \
    "DeepSeek special monitor provider should default to the stable official balance endpoint."
require_pattern 'platform Bearer token' "$source_file" \
    "DeepSeek platform failures should explain that account and usage fields require a platform Bearer token."

echo "PASS: DeepSeek special billing guard rails are wired."
