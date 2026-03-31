---
runAt: 2026-03-31.09-18-00
title: 追加新需求
---

- [x] 小BUG：结构图中的图信息没有居中
- [x] 追加一个新的页签：AI工具
	- [x] /Users/lang/zero-cloud/app-zero/r2mo-apps/app-mom 中有资源图标
	- [x] 检查打开的目录，如果目录是 .r2mo 则证明符合 momo 的基础原则，检查这个包含了 .r2mo 目录的项目中是否有
		- Claude / .claude 配置
		- Codex / .codex 配置
		- OpenCode / .opencode 配置
		- Gemini / .gemini 配置
		- Cursor / .cursor 配置
		- Trae / .trae 配置
	- [x] 如果存在则直接显示对应的配置信息，这个呈现可以参考项目任务重的配置一样，显示出当前项目的 AI工具详情，主要枚举
		- 会话数量（可展开）
		- SKILLS数量（可展开）
		- 规则数量（可展开）
- [x] 可以直接根据现有的项目配置来测试这个功能

## Changes
- 2026-03-31: [AI Agent] Completed all tasks
  - **BUG修复**: 修复 `drawProjectGraph()` 函数中文本居中问题，使用 `(nodeHeight - textHeight)/2` 计算垂直居中位置
  - **新增AI工具页签**: 在 `mainwindow.cpp` 中添加第4个Tab "AI工具"
  - **创建AIToolScanner类**: 新增 `src/utils/aitoolscanner.h` 和 `src/utils/aitoolscanner.cpp`
    - 扫描6种AI工具配置: .claude, .codex, .opencode, .gemini, .cursor, .trae
    - 统计会话数量、技能数量、规则数量
    - 支持获取具体文件列表
  - **图标集成**: 使用 app-mom 的 AI 工具图标
    - Claude.png, Cursor.png, Trae.png, opcode.png, Antigravity.png (用于 Gemini)
  - **UI实现**: 
    - 空状态提示
    - 卡片式展示各AI工具配置
    - 显示统计数据（sessions/skills/rules）
  - **更新CMakeLists.txt**: 添加新的源文件引用

- 2026-03-31: [AI Agent] Bug fixes (Round 2)
  - **修复概览红色文字字体**: 从 17px 改为 18px，增加 font-weight: 500
  - **修复结构图节点居中**: 在 `drawProjectGraph()` 末尾添加 `m_graphView->centerOn(centerPoint)` 让视图居中显示父节点
  - **修复AI工具扫描路径**: 当打开的是 `.r2mo` 目录时，自动使用父目录进行 AI 工具配置扫描

- 2026-03-31: [AI Agent] Bug fixes (Round 3)
  - **修复红色文字字体**: 改为 16px
  - **修复图标映射**: 
    - Codex -> Codex.png
    - OpenCode -> OpenCode.png
  - **优化 sessions 统计**: 重写 `countSessions()` 函数，精确统计历史会话（sessions/conversations/history/projects 目录）
  - **优化 UI 版面**: 去掉多层边框，简化卡片样式，使用单行布局（图标+名称+统计信息）

- 2026-03-31: [AI Agent] Major refactor - Tree structure (Round 4)
  - **重构数据结构**: 新增 `AIToolItem` 和 `AIToolCategory` 结构体
  - **树形结构展示**: 使用 `QTreeWidget` 替代 `QGridLayout`
  - **层级结构**:
    - 第一层：AI 工具类型（Claude/Codex/OpenCode/Gemini/Cursor/Trae）
    - 第二层：分类（Sessions/Skills/Rules/MCP）
    - 第三层：具体文件/配置项
  - **新增 MCP 扫描**: 支持扫描 mcp.json 和 settings.json 中的 MCP 配置
  - **统计显示**: 每个分类显示数量，可展开查看具体内容
  - **颜色区分**:
    - Sessions: 蓝色 (#007aff)
    - Skills: 橙色 (#ff9500)
    - Rules: 绿色 (#34c759)
    - MCP: 紫色 (#af52de)

- 2026-03-31: [AI Agent] Fix vault opening issue
  - **修复打开仓库问题**: 使用 VaultID 而非路径打开 Obsidian 仓库
  - **新增方法**: `ObsidianConfigReader::getVaultByPath()` 通过路径查找 VaultID
  - **URL Scheme**: 使用 `obsidian://open?vault=VaultID` 格式
  - **错误处理**: 找不到 VaultID 时提示用户先在 Obsidian 中添加仓库

- 2026-03-31: [AI Agent] AI Tools - Global config support
  - **Sessions 从全局查找**: 从 `~/.claude/projects/` 查找当前项目的会话
  - **Skills/Rules/MCP 标记全局**: 项目级和全局级配置分别标记
    - 项目级：直接显示文件名
    - 全局级：显示 "(global)" 后缀
  - **新增 `isGlobal` 字段**: `AIToolItem` 结构体新增标记
  - **图标显示**: 树形节点显示实际 PNG 图标
  - **数量统计**: 顶层节点显示 sessions/skills/rules/mcp 数量

- 2026-03-31: [AI Agent] Simplify Obsidian open
  - **移除注册功能**: 不需要注册 vault，因为枚举的仓库一定已注册
  - **核心逻辑**: 通过路径在 Obsidian 配置中查找正确的 VaultID
  - **错误提示**: 找不到 VaultID 时提示用户先在 Obsidian 中添加仓库

- 2026-03-31: [AI Agent] AI Tools - Project-based grouping (Final)
  - **按项目分组**: 复用 `R2moSubProject` 结构，与项目任务保持一致的层级
  - **层级结构**:
    - 📁 主项目（包含所有子项目）
      - 📂 子项目-1
        - 🤖 Claude (sessions, skills, rules, mcp)
          - 💬 Sessions: N
          - ⚡ Skills: N
          - 📋 Rules: N
          - 🔌 MCP: N
        - 🤖 Codex (...)
      - 📂 子项目-2
        - ...
  - **默认不展开**: 与项目任务一致，双击才展开
  - **图标显示**: AI工具节点使用实际的PNG图标（Claude/Codex/OpenCode/Gemini/Cursor/Trae）
  - **修改方法签名**:
    - `buildAIToolsTree(const QList<R2moSubProject>& projects)`
    - `addAIToolsToItem(QTreeWidgetItem* parentItem, const QList<AIToolInfo>& tools)`

- 2026-03-31: [AI Agent] AI Tools - Fix display logic
  - **子项目在上**: 展开后子项目显示在AI工具之前（与项目任务一致）
  - **只显示存在的配置**: 只有项目目录下存在 `.claude/.codex/.opencode/.gemini/.cursor/.trae` 才显示对应工具
  - **Sessions 精确匹配**: 只统计当前项目对应的会话（精确匹配项目路径）
  - **移除全局配置显示**: 不再显示全局的 skills/rules/mcp，只显示项目级配置

- 2026-03-31: [AI Agent] AI Tools - Fix logic (correct)
  - **显示条件修正**: 项目目录下有 `.claude` 配置 **或者** 全局 `~/.claude/projects/` 中有项目信息 → 显示对应工具
  - **核心逻辑**:
    1. 检查项目目录下是否存在配置目录
    2. 或检查全局 projects 目录中是否有该项目
    3. 两者满足其一即显示该 AI 工具
  - **Sessions 从全局读取**: 即使项目没有 `.claude` 目录，只要全局有项目会话记录也会显示

- 2026-03-31: [AI Agent] AI Tools - Final fixes
  - **Sessions 修复**: 直接读取 `~/.claude/projects/<project-path>/*.jsonl` 文件
  - **Skills 包含全局**: 项目级 + 全局级 (`~/.claude/skills/` + `~/.claude/commands/`)
  - **Rules 包含全局**: 项目级 + 全局级 (`~/.claude/rules/`)
  - **MCP 区分**:
    - Claude: 项目级 `.mcp.json`
    - Codex: 全局 `~/.codex/config.toml` 中的 `[mcp_servers.*]`
  - **全局标记**: 全局配置的 items 显示 `(global)` 后缀

- 2026-03-31: [AI Agent] AI Tools - Path format fix
  - **不同工具路径格式**:
    - Claude/Codex: `-Users-lang-path-to-project` (有 `-` 前缀)
    - Cursor/Trae: `Users-lang-path-to-project` (无 `-` 前缀)
  - **默认展开一级**: AI工具节点默认展开显示 categories

- 2026-03-31: [AI Agent] AI Tools - Sessions统计修复与交互对齐
  - **修复 sessions 统计缺失**: `scanSessions()` 改为递归扫描 `~/.<tool>/projects/<project>/` 下 `*.json/*.jsonl`，并去重，避免仅扫描一级目录导致漏计
  - **统一并增强路径匹配**: 为所有工具同时兼容「带前缀 -」与「无前缀 -」两种项目目录命名格式，减少路径格式差异导致的统计为 0 问题
  - **所有 AI 工具启用 sessions 存在性判定**: Claude/Codex/OpenCode/Gemini/Cursor/Trae 均基于真实会话文件判断是否显示工具节点（不仅依赖本地配置目录）
  - **默认展开根项目**: AI Tools 树改为与任务树一致，默认展开根节点项目
  - **补齐 AI Tools 双击交互**: 新增 `onAIToolItemDoubleClicked()`，双击 sessions/skills/rules/mcp 文件可直接打开

- 2026-03-31: [AI Agent] AI Tools - 多数据源 sessions/MCP 修正（测试目录对齐）
  - **Codex sessions**: 增加 `~/.codex/state_5.sqlite` 的 threads(cwd) 统计兜底，解决仅依赖 `~/.codex/projects/*` 导致的漏计
  - **Cursor/Trae sessions**: 增加 `~/Library/Application Support/{Cursor|Trae}/User/workspaceStorage/*/state.vscdb` 回退统计，按 `workspace.json` 与项目路径关系匹配会话键
  - **OpenCode sessions**: 从 `opencode.global.dat` 的 `globalSync.project` 做项目映射，支持父 worktree 匹配；显示文案改为 `mapped worktree: ...`，不再显示 `project:xxxx`
  - **OpenCode MCP**: 扫描 `.mcp.json`、`.opencode/opencode.json`、`~/.opencode/opencode.json` 合并去重，修复 OpenCode MCP 漏显示
  - **构建验证**: 已执行 `cmake --build build`，编译通过
