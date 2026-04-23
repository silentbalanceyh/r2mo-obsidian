---
runAt: 2026-04-23.10-38-25
title: 界面调整
author:
---
终端、AI 工具、会话、状态四列改成 Grid 模式

标题：
- Claude Code（带图标）
- Codex（带图标）
- Open Code（带图标）
单元格（一行内）：
- 图标显示终端（Hover显示终端类型）
- 图标按钮（复制），点击后复制恢复命令，不同工具恢复命令不同
- 目前状态进度条，只显示进度条，不显示文字）

最终效果：
项目和操作中间只包含 Grid 布局的内容，从上往下排列
- Claude一列
- Codex一列
- OpenCode一列
如果有多个 Session，如 Claude x 2，则切换到 2行，以最多那列的行数为主。

## Changes
- 2026-04-23: [Team Leader] 完成 task-001：将 Monitor Board 从 Terminal/AI Tool/Session/Status 四列重构为 Project | AI Sessions Grid | Action 三列；中间区域改为固定 Claude / Codex / Open Code 三列的 Grid，并按项目内各工具会话数自动扩展为多行；每个单元格显示终端图标、复制恢复命令按钮与纯进度条状态，终端图标支持 Hover 展示终端类型，复制按钮会按工具生成不同恢复命令写入剪贴板；同步补充并更新 `tests/task_001_monitor_grid_guard_test.sh`、`tests/monitor_refresh_loading_test.sh`、`tests/task_002_monitor_status_test.sh`、`tests/task_002_monitor_runtime_display_guard_test.sh` 守护验证，且已通过 `cmake -B build` 与 `cmake --build build`。
