#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
window_header="$repo_root/src/mainwindow.h"
window_source="$repo_root/src/mainwindow.cpp"

require_pattern() {
    local pattern="$1"
    local file="$2"
    local message="$3"

    if ! grep -Eq "$pattern" "$file"; then
        echo "FAIL: $message" >&2
        exit 1
    fi
}

forbid_pattern() {
    local pattern="$1"
    local file="$2"
    local message="$3"

    if grep -Eq "$pattern" "$file"; then
        echo "FAIL: $message" >&2
        exit 1
    fi
}

require_pattern 'class[[:space:]]+MonitorToolGridDelegate' "$window_source" \
    "Monitor board must render AI sessions through a dedicated tool-grid delegate."
require_pattern 'class[[:space:]]+MonitorBoardHeaderView' "$window_source" \
    "Monitor board must render the grouped tool header through a custom header view."
require_pattern 'struct[[:space:]]+MonitorToolCell' "$window_source" \
    "Monitor board must keep per-tool cell data for Claude/Codex/OpenCode grid slots."
require_pattern 'toolIconPath' "$window_source" \
    "Monitor tool cells must keep the AI tool icon path for in-cell rendering."
require_pattern 'QList<MonitorToolCell>[[:space:]]+buildMonitorToolCells' "$window_source" \
    "Monitor board must group project sessions into tool-grid cells before rendering."
require_pattern 'QStringList[[:space:]]+monitorToolOrder' "$window_source" \
    "Monitor board must use a stable Claude/Codex/OpenCode column order."
require_pattern 'QStringLiteral\("Claude"\)[[:space:]]*,[[:space:]]*QStringLiteral\("Codex"\)[[:space:]]*,[[:space:]]*QStringLiteral\("OpenCode"\)' "$window_source" \
    "Monitor grid column order must be Claude, Codex, then OpenCode."
require_pattern 'monitorToolRestoreCommand' "$window_source" \
    "Monitor grid copy buttons must generate tool-specific resume commands."
require_pattern 'claude --resume' "$window_source" \
    "Claude restore command must use a Claude-specific resume form."
require_pattern 'codex resume' "$window_source" \
    "Codex restore command must use a Codex-specific resume form."
require_pattern 'opencode' "$window_source" \
    "OpenCode restore command must use an OpenCode-specific resume form."
require_pattern 'QApplication::clipboard\(\)->setText' "$window_source" \
    "Monitor grid copy clicks must copy the generated restore command to the clipboard."
require_pattern 'kMonitorGridCellsRole' "$window_source" \
    "Monitor rows must store serialized grid cells on the project row."
require_pattern 'kMonitorCopyHitRectsRole' "$window_source" \
    "Monitor grid delegate must expose copy-button hit rectangles for click handling."
require_pattern 'monitorGridRowHeight' "$window_source" \
    "Monitor grid rows must compute height from the maximum session count across tool columns."
require_pattern 'setHeader\(new[[:space:]]+MonitorBoardHeaderView\(Qt::Horizontal,[[:space:]]*tree\)\)' "$window_source" \
    "Monitor board must install the custom grouped header."
require_pattern 'monitorToolTitle\(toolName\)' "$window_source" \
    "Monitor board header should render tool labels from the shared title helper."
forbid_pattern 'painter->drawText\(titleTextRect,[[:space:]]*Qt::AlignLeft[[:space:]]*\|[[:space:]]*Qt::AlignVCenter,[[:space:]]*monitorToolTitle\(toolName\)\)' "$window_source" \
    "Per-row grid cells must not repeat tool titles once they move into the header."
require_pattern 'scaled\(24,[[:space:]]*24,[[:space:]]*Qt::KeepAspectRatio,[[:space:]]*Qt::SmoothTransformation\)' "$window_source" \
    "Terminal icons in monitor cells should be rendered larger for readability."
require_pattern 'scaled\(18,[[:space:]]*18,[[:space:]]*Qt::KeepAspectRatio,[[:space:]]*Qt::SmoothTransformation\)' "$window_source" \
    "AI tool icons should be rendered beside the terminal icon inside each cell."
require_pattern 'drawRoundedRect\(copyRect,[[:space:]]*6,[[:space:]]*6\)' "$window_source" \
    "Copy buttons in monitor cells should be rendered larger and with a softer radius."
require_pattern 'const int availableBarWidth = qMax\(36,[[:space:]]*cellRect.right\(\) - copyRect.right\(\) - 24\)' "$window_source" \
    "Monitor cell status bars should compute the available width before shortening."
require_pattern 'const int barWidth = qMax\(28,[[:space:]]*availableBarWidth / 3\)' "$window_source" \
    "Monitor cell status bars should use about one third of the previous width."
require_pattern 'const QRect barRect\(copyRect.right\(\) \+ 16,' "$window_source" \
    "Monitor cell status bars should start after the larger icon and copy button spacing."
require_pattern '10\);' "$window_source" \
    "Monitor cell status bars should be thicker again."
require_pattern 'return 8 \+ qMax\(1,[[:space:]]*rowCount\) \* 40;' "$window_source" \
    "Monitor grid rows should trim bottom whitespace between projects."
require_pattern 'tr\("Project"\)[[:space:]]*<<[[:space:]]*QString\(\)[[:space:]]*<<[[:space:]]*tr\("Action"\)' "$window_source" \
    "Monitor board must keep only Project and Action text headers while the middle header is custom drawn."
forbid_pattern 'tr\("AI Sessions' "$window_source" \
    "Monitor board should no longer render the literal AI Sessions label."
forbid_pattern '<<[[:space:]]*tr\("Terminal"\)[[:space:]]*<<[[:space:]]*tr\("AI Tool"\)[[:space:]]*<<[[:space:]]*tr\("Session"\)[[:space:]]*<<[[:space:]]*tr\("Status"\)' "$window_source" \
    "Monitor board must no longer expose Terminal/AI Tool/Session/Status as separate columns."
require_pattern 'const[[:space:]]+QString[[:space:]]+statusText[[:space:]]*=' "$window_source" \
    "Monitor grid status cells must render Working/Ready text on the right side."
require_pattern 'void[[:space:]]+copyMonitorRestoreCommand[[:space:]]*\([[:space:]]*const[[:space:]]+QString&[[:space:]]+command[[:space:]]*\)' "$window_header" \
    "MainWindow must expose an internal helper for monitor copy actions."

echo "PASS: task-001 monitor grid guard rails are wired."
