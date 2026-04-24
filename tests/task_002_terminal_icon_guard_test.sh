#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
scanner_source="$repo_root/src/utils/sessionscanner.cpp"
window_source="$repo_root/src/mainwindow.cpp"
icons_qrc="$repo_root/src/icons/ui-icons.qrc"
ubuntu_icon="$repo_root/src/icons/ui/terminal-ubuntu.svg"

require_pattern() {
    local pattern="$1"
    local file="$2"
    local message="$3"

    if ! grep -Fq "$pattern" "$file"; then
        echo "FAIL: $message" >&2
        exit 1
    fi
}

require_pattern 'QFile::exists(QStringLiteral("/Applications/WezTerm.app"))' "$scanner_source" \
    "WezTerm terminal icon mapping must document the source app bundle used for extraction."
require_pattern 'QFile::exists(QStringLiteral("/Applications/iTerm.app"))' "$scanner_source" \
    "iTerm terminal icon mapping must document the source app bundle used for extraction."
require_pattern 'QFile::exists(QStringLiteral("/Applications/Ghostty.app"))' "$scanner_source" \
    "Ghostty terminal icon mapping must document the source app bundle used for extraction."
require_pattern 'QFile::exists(QStringLiteral("/Applications/Utilities/Terminal.app"))' "$scanner_source" \
    "Terminal.app icon mapping must document the source app bundle used for extraction."
require_pattern 'return ":/icons/ui/terminal-wezterm.png";' "$scanner_source" \
    "WezTerm terminal icon mapping must use a checked-in project asset."
require_pattern 'return ":/icons/ui/terminal-iterm.png";' "$scanner_source" \
    "iTerm terminal icon mapping must use a checked-in project asset."
require_pattern 'return ":/icons/ui/terminal-ghostty.png";' "$scanner_source" \
    "Ghostty terminal icon mapping must use a checked-in project asset."
require_pattern 'return ":/icons/ui/terminal-terminalapp.png";' "$scanner_source" \
    "Terminal.app icon mapping must use a checked-in project asset."
require_pattern 'if (lower.contains("ssh")) return ":/icons/ui/terminal-ubuntu.svg";' "$scanner_source" \
    "SSH terminal rows must use the Ubuntu-style icon."
require_pattern 'const QPixmap terminalPix = cachedScaledPixmap(cell.terminalIconPath, 14, 14);' "$window_source" \
    "Monitor terminal icons must be normalized to 14x14 for alignment with text."
require_pattern '<file>ui/terminal-wezterm.png</file>' "$icons_qrc" \
    "The extracted WezTerm terminal icon must be registered in the UI icon bundle."
require_pattern '<file>ui/terminal-iterm.png</file>' "$icons_qrc" \
    "The extracted iTerm terminal icon must be registered in the UI icon bundle."
require_pattern '<file>ui/terminal-ghostty.png</file>' "$icons_qrc" \
    "The extracted Ghostty terminal icon must be registered in the UI icon bundle."
require_pattern '<file>ui/terminal-terminalapp.png</file>' "$icons_qrc" \
    "The extracted Terminal.app icon must be registered in the UI icon bundle."
require_pattern '<file>ui/terminal-ubuntu.svg</file>' "$icons_qrc" \
    "The Ubuntu terminal icon resource must be registered in the UI icon bundle."
require_pattern '<rect x="2" y="3" width="16" height="14" rx="2.5" fill="#E95420"/>' "$ubuntu_icon" \
    "The Ubuntu terminal icon asset must be present."

echo "PASS: terminal icon guard rails are wired."
