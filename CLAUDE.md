# Obsidian Manager - Qt C++ Project

## Project Overview

Obsidian Manager is a Qt 6.10.0 C++ desktop application for managing multiple Obsidian vaults from a centralized interface.

## Technology Stack

- **Qt Version**: 6.10.0 (macOS)
- **Qt Path**: `/Users/lang/Qt/6.10.0/macos`
- **Build System**: CMake 3.16+
- **C++ Standard**: C++17
- **Platform**: macOS (primary), cross-platform support planned

## Build Instructions

```bash
# Configure
cmake -B build -DCMAKE_PREFIX_PATH=/Users/lang/Qt/6.10.0/macos

# Build
cmake --build build

# Run
./build/ObsidianManager.app/Contents/MacOS/ObsidianManager
```

## Project Structure

```
r2mo-obsidian/
├── CMakeLists.txt          # CMake configuration
├── src/
│   ├── main.cpp            # Application entry point
│   ├── mainwindow.h        # Main window header
│   └── mainwindow.cpp      # Main window implementation
├── .r2mo/                  # Project metadata (preserved)
│   ├── task/               # Task definitions
│   ├── design/             # Design specifications
│   └── domain/             # Domain models
├── CLAUDE.md               # This file - development rules
├── AGENTS.md               # Agent processing rules
└── .gitignore              # Git ignore rules
```

## Development Guidelines

### Qt Coding Standards

1. **Naming Convention**
   - Classes: PascalCase (e.g., `MainWindow`)
   - Methods: camelCase (e.g., `onAddVault`)
   - Member variables: `m_` prefix (e.g., `m_vaultList`)
   - Signals: no prefix, camelCase
   - Slots: `on` prefix for handlers

2. **Memory Management**
   - Use Qt's parent-child ownership for automatic cleanup
   - Prefer `QScopedPointer` for non-Qt objects
   - Avoid raw `new` without parent assignment

3. **Signal/Slot Connections**
   - Use new-style connections: `connect(obj, &Class::signal, this, &Class::slot)`
   - Prefer lambda connections for simple handlers

4. **UI Layout**
   - Use layouts (`QVBoxLayout`, `QHBoxLayout`, `QGridLayout`)
   - Avoid fixed sizes; use size policies and hints
   - Support resizable windows

### File Organization

- Headers: `.h` files in `src/`
- Implementation: `.cpp` files in `src/`
- Resources: future `resources/` directory
- Tests: future `tests/` directory

### Configuration Storage

- Vault data: `QStandardPaths::AppConfigLocation` + `/vaults.json`
- Settings: `QSettings` with organization "R2MO", app "Obsidian Manager"

## Feature Roadmap

### Phase 1 (Current)
- Basic vault list management
- Add/remove vaults
- Open vaults in Obsidian
- Preview pane for vault info

### Phase 2 (Planned)
- File browser within vaults
- Markdown preview (read-only)
- Search across vaults
- Vault tagging/categorization

### Phase 3 (Future)
- Multi-agent integration
- Plugin system
- Cross-platform support (Windows, Linux)

## Obsidian Integration

- Default app path: `/Applications/Obsidian.app`
- Configurable in Settings menu
- Opens vaults using macOS `open -a Obsidian.app <path>` command

## .r2mo Directory

The `.r2mo/` directory contains project metadata and is **NOT** ignored by git. It includes:
- Task definitions (`task/*.md`)
- Design specifications (`design/*.md`)
- Domain models (`domain/`)
- API schemas (`api/`)

## Related Files

- `AGENTS.md`: Rules for AI agent processing
- `.r2mo/design/spec.md`: UI design specifications (Tailwind-based, for reference)
- `.r2mo/design/spec-page.md`: Page implementation specifications