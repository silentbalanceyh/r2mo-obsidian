---
runAt: 2026-04-22.19-23-59
title: 关于状态的计算
author:
---
目前看起来状态计算有两个问题，我觉得需要从系统架构这一层去解决：
- 有些 Terminal 中没有打开的 Session 也呈现出来了，出现的 Session 必须是有 Terminal 工具进程的
- Ready 和 Working 在刷新过程中会出现幻觉，我觉得此处算法应该要更改，看是否可以从更加底层的方式去解决
- 远程仓库选择时不用提示再输入项目名称

## Changes
- 2026-04-22: [Team Leader] 完成 task-002：在 `SessionScanner` 中增加 AI 进程祖先过滤，避免同一终端链上的辅助/子进程被误报为独立 Session；为 Claude artifact 状态判定补充事件 freshness 窗口，避免旧 `tool_use/progress` 事件把状态误判为 Working；收紧 CPU fallback 的即时 Working 判定，减少刷新期间 `Ready/Working` 幻觉；并通过 `bash tests/task_002_monitor_status_test.sh`、`bash tests/monitor_refresh_loading_test.sh`、`bash tests/task_002_special_monitor_refresh_test.sh` 与 `cmake --build build -j4` 验证。
- 2026-04-22: [Team Leader] 补充监控漏捕捉修正：`collectAllProjectPaths()` 现在会将本地 `.r2mo` 仓库条目归一到项目根目录，避免像 `app-gitbackend` 这类配置路径与运行 cwd 不一致时漏掉会话；远程 `RemoteSessionScanner` probe 新增 AI 进程祖先过滤，避免 `webos-mxt` 这类远程仓库把 Codex vendor/helper 或 Claude helper 误识别成独立会话；并通过新增守护测试与 `bash tests/task_002_validation_guard_suite.sh`、`bash tests/task_002_strict_validation_test.sh`、`cmake --build build -j4` 验证。
