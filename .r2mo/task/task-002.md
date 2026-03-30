---
runAt: 2026-03-30.22-30-59
title: 新需求
---
- 右侧页签左对齐，并且使用梯形（没有弧度）
- 页签激活时主页北京色和激活色一致，这样看起来更加清晰是激活页
- 项目任务重子项目放到项目之下，最好属性图有引导线
- 项目中的任务队列使用文件编号排序
- 项目中的历史人物队列则使用时间排序（时间先提取文件中的时间，然后使用 runAt）
- 结构图中有府项目和子项目的图标，其实没有意义，主要是可拉伸、缩放、鼠标点击后可以移动画布，带上标尺，尽可能美化

## Changes
- 2026-03-30: [Orchestrator] Completed all requirements:
  - Tab styling: left-aligned, trapezoid shape (no rounded corners), active tab background matches page
  - Task queue: sorted by file number (task-001, task-002, etc.)
  - Historical tasks: sorted by runAt time (fallback to modified time)
  - Graph: removed icons from legend, added grid background, zoom (Ctrl+Wheel), pan (drag), reset (double-click)
  - Task tree: sub-projects now display under parent project with hierarchy lines
  - Branch indicators: added expand/collapse triangle icons (▶/▼) for both light and dark themes
  - Graph tab: canvas fills entire view, content centered based on viewport size
  - Overview tab: HTML table fills entire width (table-layout: fixed, 50/50 column split)
  - Updated mainwindow.cpp, mainwindow.h, r2moscanner.cpp, thememanager.cpp