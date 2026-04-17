---
runAt: 2026-04-07.12-54-17
title: 泳道图目前存在的 BUG
---
- [x] 当泳道图被打开的时候，HOME 页签左侧多出了一个 x 的关闭按钮，这个按钮点击过后没有任何效果，而且是多余的，HOME 页签保持永远不会被关闭的基本语义逻辑。
- [x] 当泳道图页签被打开的时候，泳道图页签左侧很大区域的空白，中间隐藏了一个白色的 x 按钮，点击过后是关闭，此处正确的排序应该是：图标 泳道图 关闭（应该是在页签的右侧）

上述两个问题在经历了很久的更新之后一直没有改对，所以我觉得你是思路出了问题，一直在打补丁而且一直在碰撞，我觉得你是不是应该调整思路之后来更改这个问题保证一次性改对，最好的方式是直接捕捉。

## Changes
- 2026-04-07: [Agent] 彻底修复泳道图标签页 BUG
  - **根本原因**：`setTabsClosable(true)` 是全局设置，每次操作 tab 后会重新创建所有 tab 的关闭按钮
  - **解决方案**：使用 `setTabsClosable(false)`，手动为泳道图 tab 添加关闭按钮
  - **HOME 标签**：永不显示关闭按钮（无 setTabButton 调用）
  - **泳道图标签**：手动创建 `QPushButton("×")` 并设置到 `RightSide`，悬停时红色背景
  - **关闭按钮大小**：在样式表中添加 `QTabBar::close-button { width: 14px; height: 14px; }` 限制为正方形
  - **修改位置**：
    - `mainwindow.cpp:620` - `setTabsClosable(false)`
    - `mainwindow.cpp:628` - 添加 `close-button` 样式限制宽高
    - `mainwindow.cpp:1783-1798` - 添加泳道图 loading 标签的关闭按钮
    - `mainwindow.cpp:1810-1825` - 添加泳道图实际内容的关闭按钮
    - `mainwindow.cpp:2530-2545` - 添加刷新后的关闭按钮
- 2026-04-07: [Agent] 修复关闭按钮宽度过大问题
  - **问题**：QPushButton 的 setFixedSize(14, 14) 不生效，按钮宽度超过5个中文字
  - **解决方案**：改用 QToolButton，设置 `setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed)`
  - **修改内容**：
    - 添加 `#include <QToolButton>` 头文件
    - 三处创建关闭按钮的代码都改为使用 QToolButton
    - 设置 `setFixedSize(QSize(14, 14))` 和 `setSizePolicy(Fixed, Fixed)` 确保严格的正方形
- 2026-04-07: [Agent] 统一三处关闭按钮样式，添加 margin-right: 4px
  - **问题**：第三处（刷新阶段 2541 行）缺少 margin-right，可能导致视觉不一致
  - **修复**：三处关闭按钮样式完全一致，包含 `margin-right: 4px` 防止按钮紧贴右边界