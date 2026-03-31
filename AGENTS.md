# AGENTS.md - AI Agent Processing Rules

## Project Context

This is a Qt 6.10.0 C++ desktop application for Obsidian vault management. Agents should understand the Qt/C++ context when processing tasks.

## Agent Workflow Rules

### Task Processing

1. **Read Task File**: Tasks are defined in `.r2mo/task/task-*.md`
2. **Understand Requirements**: Parse frontmatter and body content
3. **Execute**: Implement changes following Qt coding standards
4. **Verify**: Build and test if applicable
5. **Record**: Append Changes section to task file

### Task File Format

```markdown
---
runAt: YYYY-MM-DD.HH-MM-SS
title: Task Title
---
Task description and requirements...

## Changes
- YYYY-MM-DD: Change description
```

### Change Recording Format

When completing a task, append:

```markdown
## Changes
- 2026-03-30: [Agent] Completed task - brief description of changes made
```

## Code Generation Rules

### Qt C++ Code

- Use Qt 6.10.0 API
- Follow naming conventions in CLAUDE.md
- Include proper headers
- Use CMake build system
- Generate both `.h` and `.cpp` files

### File Paths

- Source files: `src/*.cpp`, `src/*.h`
- CMake: `CMakeLists.txt`
- Config: `.r2mo/task/*.md`, `.r2mo/design/*.md`

### Build Verification

Before claiming completion:
1. Run `cmake -B build`
2. Run `cmake --build build`
3. Check for compilation errors

## Task Types

### Type: Initialization
- Create project structure
- Set up build files
- Create base classes

### Type: Feature
- Add new functionality
- Extend existing classes
- Add UI components

### Type: Bug Fix
- Fix compilation errors
- Fix runtime issues
- Fix UI behavior

### Type: Refactor
- Improve code structure
- Optimize performance
- Clean up code

## Special Directories

### .r2mo/ (Preserved)
- Contains project metadata
- Never delete or ignore
- Task files, design specs, domain models

### build/ (Generated)
- CMake build output
- Can be deleted and regenerated

## Integration Points

### Obsidian App
- Path configurable via Settings
- Default: `/Applications/Obsidian.app`
- Open vaults with `open -a` command

### Vault Data
- JSON format in AppConfigLocation
- Structure: `{ name, path, addedAt }`

## Agent Coordination

When multiple agents work on this project:
- Each agent records its changes in task file
- Agents should not modify another agent's work without coordination
- Use git for version control

## Error Handling

If task cannot be completed:
1. Record partial progress in Changes
2. Note blockers or issues
3. Suggest next steps

## Testing

Currently no automated tests. Manual testing:
1. Build application
2. Run and verify UI
3. Test vault operations
4. Check Obsidian integration

## Release Workflow (CI + Script)

### Goal
- Publish cross-platform artifacts to a single Gitee release tag.

### CI Workflow
- File: `.github/workflows/gitee-release-ci.yml`
- Trigger: `workflow_dispatch` with input `version` (e.g. `v1.0.0`)
- Artifacts (current lanes):
  - Windows x64
  - Linux x64
  - Linux arm64
  - macOS arm64
  - macOS x64

### Required Secrets / Variables
- GitHub Actions Secret:
  - `GITEE_TOKEN`
- GitHub Actions Variables:
  - `GITEE_OWNER` (default in workflow: `silentbalanceyh`)
  - `GITEE_REPO` (default in workflow: `r2mo-obsidian`)

### Local Automation Script
- File: `scripts/release_gitee.sh`
- Usage:
```bash
scripts/release_gitee.sh v1.0.0
```
- What it does:
  1. Dispatches `gitee-release-ci.yml`
  2. Locates latest run ID
  3. Watches until completion
  4. Prints final status

### Agent Release Rules
- Use CI for release artifacts; do not manually upload binaries unless CI is blocked.
- Always publish all platform artifacts to the same release tag.
- Verify release page contains downloadable assets before reporting release complete.
