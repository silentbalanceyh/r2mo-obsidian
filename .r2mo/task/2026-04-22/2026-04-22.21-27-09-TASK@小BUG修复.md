---
runAt: 2026-04-22.19-23-56
title: 小BUG修复
author:
---
- 项目监控频率改成1分钟，更改后产生 commits
- 核心问题来了：
	- 每一行在会话列前边加上 Loading 小圈圈，进行并行 Loading，如果是计费则在添加的记录最前边
	- 不使用整页 Loading
- 并行加载后：
	- 项目数量不能更改，之前有并行加载后左侧项目有无效项目出现
	- 目前终端、工具、会话、状态这些列的数据要正常，之前更改后此处数据没了

## Changes
- 2026-04-22: [Team Leader] 完成 task-001：将监控板轮询频率调整为 1 分钟；移除主监控表整表 Loading 遮罩，改为按稳定项目行逐行打 `◌` loading 标识并就地刷新，避免并行刷新导致项目数量漂移或无效项目出现；特殊计费板同步改为在记录前插入逐行 loading 占位；并通过 `bash tests/monitor_refresh_loading_test.sh`、`bash tests/task_002_monitor_status_test.sh`、`bash tests/task_002_special_monitor_refresh_test.sh` 与 `cmake --build build -j4` 验证。
