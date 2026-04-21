---
runAt: 2026-04-21.14-42-43
title: 状态运算问题
author:
---
状态运算中的工作中和就绪依旧不准确，主要覆盖在 Codex 类型的 AI 工具，当我使用它打开了Terminal终端之后，有时候刷新它就会自然演变成 Working，但实际上并没有在工作，所以此处的状态计算是有问题的。我希望在线检索更加理性的 Working 方案，实际是它真正在运行的时候才可以断定。

## Changes
- 2026-04-21: [Team Leader] Split monitor refresh cadence by panel so the upper AI session board now auto-refreshes every 10 seconds while the lower special billing board refreshes independently every 5 minutes with manual refresh still available, reducing unnecessary background polling without changing the current UI.
- 2026-04-21: [Team Leader] Refined Codex Working/Ready inference after checking real Codex session tails and platform process signals. `task_started`, tool-call, and reasoning events now count as Working only when their artifact timestamps are still fresh; stale activity falls back to Ready, and end-side events such as `task_complete`, `final_answer`, `function_call_output`, and tool-end events explicitly resolve to Ready to prevent idle terminal sessions from drifting into false Working.
- 2026-04-21: [Team Leader] Performed a second deep memory optimization pass without changing the current UI. Replaced swimlane queue cell widget explosions with a lightweight painted queue widget, replaced monitor status/action `setItemWidget()` cells with delegate painting, added short-lived Git and AI scan caches, rebuilt and restarted the app, and re-measured memory at RSS `191776 KB` with physical footprint `91.3 MB` on PID `61856`.
- 2026-04-21: [Team Leader] Continued macOS-only optimization without changing the visible UI. The preview detail tabs now hydrate lazily so task tree, structure graph, and AI tool trees are built only when their tab is opened. After rebuilding and restarting the full environment, memory measured at RSS `162384 KB` with physical footprint `70.0 MB` on PID `69773`.
- 2026-04-21: [Team Leader] Fixed the repeated Codex `Session ID = unknown` regression by indexing active `thread_id` records from `~/.codex/log/codex-tui.log`, mapping them back to each live process `workdir`, and only then falling back to rollout artifacts plus process/artifact start-time matching. Rebuilt and restarted the app, and verified against live local Codex processes that projects such as `app-aisz`, `app-harmony-os`, `app-shoot-r2`, `r2mo-rapid`, `zero-ecotope`, and `r2mo-obsidian` now resolve concrete session IDs instead of `unknown`.
