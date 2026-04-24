---
runAt: 2026-04-23.10-38-25
title: 实时性的改造
author:
---
目前系统中监控的数据（实时监控板）采用了轮询机制，这种机制会导致实时性很差，特别是工作状态 Working / Ready 两种，为了保证实时性，是否可以采取另外的方式，比如 File Watcher 或在线的机制，让这些数据可以主动获取。计费数据是没有办法，因为牵涉到远程接口，只能依靠轮询，但状态呢？
另外，远程仓库选择时不用提示再输入项目名称。

## Changes
- 2026-04-23: [Team Leader] 完成监控板实时状态改造，新增基于 QFileSystemWatcher 的本地 AI 会话状态去抖刷新链路；保留远程计费/远程监控轮询；将本地 watcher 刷新改为本地行增量更新并保留远程行，同时降低监听重建频率以控制监控过程中的性能开销。
- 2026-04-23: [Team Leader] 继续完成状态稳定性修复，收紧 `Claude/Codex/OpenCode` 的本地/远程状态语义：显式 `Ready` 事件不再被 keep-alive 顶回 `Working`，`Claude` 的 `PostToolUse/hook_success/async_hook_response` 归并到完成态，`OpenCode` 的 `session.status` 改为解析明确状态值后再决定是否 `Working`；同步补强 guard，并通过状态守卫与构建验证。

- 2026-04-24: [Team Leader] 第三轮继续做启动期内存优化：将 Swimlane/Monitor/Preview/Remote/Special Monitor 的 timer/watcher 改为按需初始化，首页预览缓存改为默认仅保留 R2MO 摘要统计、完整项目结构延迟到 Tasks/Graph/AI Tools 首次访问时加载；补回远程连通性启动首刷，完成构建与 task-002 守卫验证。
- 2026-04-24: [Team Leader] 继续推进启动期内存优化：将默认单配置构建切到 Release，并在返回 Overview 时主动释放 Preview 中延迟加载的完整项目结构载荷；重新配置默认 build 后实测首屏 Physical footprint 约 64.5M。
- 2026-04-24: [Team Leader] 继续将首屏 Preview 本地统计异步化：新增 LocalPreviewStats 异步 watcher，将 Overview 中本地仓库的 Files/Folders/Hidden/Obsidian 统计从主线程移出，并清理未使用的监控本地刷新定时器字段；验证通过，实测 RSS 有所下降，但 footprint 未优于 Release 基线。
- 2026-04-24: [Team Leader] 重新扫描后转向资源层优化，在不改首页行为前提下去掉启动时对 Meslo 嵌入字体的强制加载，并将监控/AI 工具的大尺寸 PNG 资源压缩到更贴近实际显示尺寸；验证通过，默认 Release 构建实测首屏 Physical footprint 下降到约 60.5M。
