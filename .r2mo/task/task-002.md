---
runAt: 2026-04-20.08-22-08
title: 应用整体调整
author:
---
- HOME 左侧的项目列表添加之后可以通过拖拽调整列表中项目的顺序
- 实时监控板中的顺序应该和项目列表中顺序一致
- 最终验证抓取的 Session 是否合理
- 在实时监控板中双击时可以直接和项目列表中一样，可打开 obsidian 或激活
- 右侧的Goto 在 MacOS 的虚拟屏幕上并没有移动到合理的位置打开其桌面
- Terminal还需要追加 Ghostty 的类型（目前打开了但没有识别出来）

## Changes
- 2026-04-20: [Team Leader] 协调完成 session 扫描器子任务，补充 Ghostty 识别与图标映射回退，稳定 live session 项目归类与会话/状态归并，并完成构建验证
- 2026-04-20: [Team Leader] 完成 HOME 项目列表拖拽排序持久化、监控板顺序与列表联动、监控板双击与 Goto 统一激活/打开逻辑，并再次通过 CMake 构建验证
- 2026-04-20: [Team Leader] 重写 dev-start.sh / dev-stop.sh，补齐停止旧实例、强制重新配置构建、可靠拉起与启动日志校验，修复重启后无明显生效的问题
- 2026-04-20: [Team Leader] 全局收敛树控件展开图标样式，统一覆盖 QTreeWidget/QTreeView 的浅深色与选中态分支图标，修复项目选择器在浅色主题下展开图标不可见的问题
- 2026-04-20: [Team Leader] 进一步修复项目选择器：改为显式绘制目录展开 chevron，单击仅在左侧图标热区展开/收起，双击目录才触发添加，避免普通单击误添加项目
- 2026-04-20: [Team Leader] 补齐项目选择器 viewport 事件过滤接线，修复点击展开图标无响应的问题，并重新构建重启应用
- 2026-04-20: [Team Leader] 修复实时监控 Session 映射：Claude 改为同项目多 artifact 按 shell 会话分配，避免多个 Claude Code 共用同一 Session ID；OpenCode 增加从全局最近会话状态补齐项目 Session ID
- 2026-04-20: [Team Leader] 二次修复实时监控 Session 映射：Claude 改为按 shell/process 稳定顺序一一绑定项目 artifact，消除重复 Session ID；OpenCode 改为优先读取 SQLite 运行态会话并按项目目录归属匹配，补齐 `unknown` Session ID
