---
runAt: 2026-03-30.08-48-55
title: 状态计算依旧有问题
---
网络搜索下，Claude 的窗口中我只是打开了窗口，没有做任何其他事，但目前状态却显示工作中，而且很多 Session 都显示工作中，有时候又是就绪，这个问题是否可以通过某种方式解决？最少让状态计算准确点？

## Changes

### 2026-04-28 — 状态计算准确性修复

**根因分析：**

1. **Claude artifact 状态误判**：`inferClaudeArtifactStatus` 将 `PreToolUse`、`tool_use`、`assistant(tool_use)` 等事件直接映射为 Working，但后台 hook（oh-my-claudecode、auto-compact、session-restore 等）即使空闲时也会持续产生这些事件，导致窗口仅打开就显示"工作中"。
2. **CPU 回退阈值过低**：`strongCpuBurst=300K`、`sustainedCpuActivity=90K` 阈值对 Claude/Electron 进程偏低，空闲时后台线程的 CPU 活动即可触发误判；30 秒 keep-alive 进一步延长了 Working 状态。

**修复内容：**

1. **用户轮次感知（User Turn Awareness）**：在 `inferClaudeArtifactStatus` 中增加 Phase 1 预扫描，检查最近是否有用户发起的 turn（`type: "user"` 或 `"last-prompt"` 事件）且未收到 `end_turn` / `turn_duration` / `away_summary` 终止信号。只有存在活跃用户轮次时，PreToolUse/tool_use/assistant(tool_use) 事件才映射为 Working，否则映射为 Ready。用户轮次窗口 300 秒。

2. **CPU 阈值上调**：
   - `strongCpuBurst`: 300K → 800K
   - `sustainedCpuActivity`: 90K → 250K
   - `highSamples` 触发门槛: 2 → 3
   - `strongCpuBurst` 初始值: 2 → 3

3. **Keep-alive 缩短**：`workingKeepAliveSeconds` 从 30 秒降至 15 秒，减少误判 Working 状态的滞留时间。

**影响文件：** `src/utils/sessionscanner.cpp`

### 2026-04-29 — Claude 空闲 transcript 不再回落 CPU 误判

- [Codex] 修复 `determineStatus()` 中 Claude 已匹配 transcript artifact 后仍继续走 CPU 兜底的问题：当 artifact 没有新鲜活跃用户轮次语义时直接返回 Ready，避免只打开 Claude Code/空闲 CLI 进程因 CPU 抖动显示 Working。
- 新增 `tests/task_005_claude_idle_artifact_guard_test.sh`，并接入 task-002 状态验证套件，防止该回归再次出现。
- 已通过状态守卫、CMake 配置和完整构建验证。
