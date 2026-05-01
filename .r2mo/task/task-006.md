---
runAt: 2026-03-30.08-48-55
title: BUT-特殊计费板加载慢
---

## Bug

特殊计费板配置多个 token 时，部分 Provider 响应慢会导致整板数据等待很久后才整体显示。

## Root Cause

`SpecialMonitorFetcher::fetchSnapshots()` 按 token 串行请求，每个慢请求最多等待 5 秒。多个慢源叠加后，总刷新耗时线性增长。

## Fix

将特殊计费源抓取拆分为单源分派函数，并在 `fetchSnapshots()` 中为每个源创建独立 `std::future` 并发请求，最终按 future 收集顺序保持 UI 行顺序不变。

## Files Changed

- `src/utils/specialmonitorfetcher.h`
- `src/utils/specialmonitorfetcher.cpp`
- `tests/task_003_special_monitor_parallel_guard_test.sh`

## Verification

- `bash tests/task_003_special_monitor_parallel_guard_test.sh`
- `bash tests/task_003_special_monitor_cache_guard_test.sh`
- `bash tests/task_002_special_monitor_refresh_test.sh`
- `cmake -B build -DCMAKE_PREFIX_PATH=/Users/lang/Qt/6.10.0/macos`
- `cmake --build build`

## Changes

- 2026-05-01 11:00: [Codex] Fixed special billing slow-load behavior by fetching Provider/token sources concurrently while preserving table order, added a guard test for concurrent source fetches, and verified special billing guards plus CMake configure/build.
