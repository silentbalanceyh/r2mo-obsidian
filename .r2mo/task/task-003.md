---
runAt: 2026-04-03.15-53-11
title: 特殊计费版追加
---
追加新的 Provider：
CM Key
URL：https://cmkey.cn/api/query?key=添加时
响应：
```json
{
    "success": true,
    "data": {
        "username": "admin",
        "token_name": "glm5.1-plus-1-6TPFrZ",
        "weekly_limit": 3000,
        "weekly_used": 25,
        "weekly_remain": 2975,
        "week_pct": 0,
        "expired_at": 1779727164,
        "expired_str": "2026-05-26",
        "expired": false,
        "week_reset_str": "\u672a\u8bbe\u7f6e",
        "week_reset_at": null,
        "period_limit_hours": 5,
        "period_limit": 3000,
        "period_used": 25,
        "period_remain": 2975,
        "period_pct": 0,
        "primary_type": "period",
        "primary_label": "5\u5c0f\u65f6",
        "total_calls": 25
    }
}
```

计费维度字段
- Provider：换一个 emoji，CM Key
- Token：同样可复制
- 账号：token_name
- 类型：每5小时
- 新增：period_limit
- 更新时间每次调用时提取
- 数据按 period_ 进行统计

## Changes
- 2026-04-26: [Team Leader] Completed task - implemented CM Key special billing fetch logic without adding provider UI entries; CM Key now uses the key query parameter, maps token_name as account, period_limit/period_used/period_remain for period statistics, shows type as every N hours, and records update time on each fetch.
- 2026-04-26: [Team Leader] Follow-up - added the CM Key provider option to the special billing Add/Edit provider dropdown so it can be selected from the UI.
- 2026-04-29: [Codex] Fixed transient special billing refresh zeroing: failed/timeout snapshots are now merged with the previous cache by provider/token key, preserving last known quota and usage values while surfacing the refresh error in row tooltips. Added `tests/task_003_special_monitor_cache_guard_test.sh` and verified the special monitor guards plus CMake build.
