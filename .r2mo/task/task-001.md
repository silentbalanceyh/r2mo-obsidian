---
runAt: 2026-04-07.09-53-09
title: 追加项目标记
---
通道图的项目卡片项目内容上方追加（记得是内容，同是项目区域）
- 如果扫描使用了哪些AI工具，则使用AI工具对应的图标，后边跟上 sessions 的数量
- 然后：项目名称上检查 Git 的状态，和 Terminal 一样，如果完全同步项目名称使用绿色，如果有commits没有push用蓝色，如果还没有commits则使用红色。
- 参考目前 zsh 中的配置呈现 Git：分支名（下边统计使用不同颜色）
	- 向上箭头 N 表示未提交的 commits
	- 感叹号N：Changes not staged for commits
	- 问号N：Untracked fileds