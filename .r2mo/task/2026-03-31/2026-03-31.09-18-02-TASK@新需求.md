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
  - Graph: ruler background with tick marks and grid, zoom (Ctrl+Wheel), pan (drag), reset (double-click)
  - Task tree: sub-projects display under parent project with expand/collapse icons (⊕/⊖ style)
  - Overview tab: 
    - Red path highlight with larger font (15px, bold)
    - Tables fill entire width with 50/50 column split
    - 8px spacing between path bar and tables
    - Increased padding (16px 24px) for better readability
  - LSP configuration: added .clangd and compile_commands.json symlink
  - Updated mainwindow.cpp, mainwindow.h, r2moscanner.cpp, thememanager.cpp, CLAUDE.md