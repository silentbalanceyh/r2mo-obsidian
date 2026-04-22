#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
window_source="$repo_root/src/mainwindow.cpp"
task_file="$repo_root/.r2mo/task/task-002.md"

require_pattern() {
    local pattern="$1"
    local file="$2"
    local message="$3"

    if ! grep -Fq "$pattern" "$file"; then
        echo "FAIL: $message" >&2
        exit 1
    fi
}

require_regex() {
    local pattern="$1"
    local file="$2"
    local message="$3"

    if ! grep -Eq "$pattern" "$file"; then
        echo "FAIL: $message" >&2
        exit 1
    fi
}

require_pattern 'QStringLiteral("/media")' "$window_source" \
    "Remote directory selector must expose /media as a first-class root entry."
require_pattern 'QTreeWidgetItem *homeItem = new QTreeWidgetItem(treeWidget);' "$window_source" \
    "Remote directory selector must create a dedicated home root item."
require_pattern 'homeItem->setText(0, QStringLiteral("🏠 ~"));' "$window_source" \
    "Remote directory selector must label the personal root with the ~ symbol only."
require_pattern 'homeItem->setData(0, pathRole, QStringLiteral("."));' "$window_source" \
    "Remote directory selector must map the displayed ~ root back to the HOME working directory."
require_pattern 'QTreeWidgetItem *mediaItem = new QTreeWidgetItem(treeWidget);' "$window_source" \
    "Remote directory selector must create a dedicated /media root item."
require_pattern 'mediaItem->setText(0, QStringLiteral("💽 /media"));' "$window_source" \
    "Remote directory selector must label the /media root with emoji text."
require_pattern 'treeWidget->setRootIsDecorated(false);' "$window_source" \
    "Remote directory selector must hide the native branch indicator and use the custom disclosure style."
require_pattern 'treeWidget->setIconSize(QSize(14, 14));' "$window_source" \
    "Remote directory selector must use the same disclosure icon sizing as the local selector."
require_pattern 'treeWidget->viewport()->setProperty("isProjectSelectionTreeViewport", true);' "$window_source" \
    "Remote directory selector must mark the viewport like the local selector."
require_pattern 'treeWidget->viewport()->installEventFilter(const_cast<MainWindow*>(this));' "$window_source" \
    "Remote directory selector must install the shared event filter for disclosure behavior."
require_pattern 'const QColor disclosureColor = ThemeManager::instance()->currentTheme() == ThemeManager::Light' "$window_source" \
    "Remote directory selector must compute disclosure colors like the local selector."
require_pattern 'auto createDisclosureIcon = [](const QColor& color, bool expanded)' "$window_source" \
    "Remote directory selector must draw custom disclosure triangles."
require_pattern 'std::function<void(QTreeWidgetItem*)> updateRemoteTreeIcons;' "$window_source" \
    "Remote directory selector must refresh custom disclosure icons recursively."
require_pattern 'const int expandableRole = Qt::UserRole + 4;' "$window_source" \
    "Remote directory selector must reuse the local expandable role so disclosure clicks work."
require_pattern 'connect(treeWidget, &QTreeWidget::itemCollapsed, &dialog' "$window_source" \
    "Remote directory selector must refresh disclosure icons when collapsing."
require_pattern 'mediaItem->setData(0, expandableRole, !mediaLoadFailed);' "$window_source" \
    "Remote directory selector must keep /media visible even when permissions block expansion."
require_pattern 'mediaItem->setText(1, mediaLoadFailed ? tr("Permission denied") : QStringLiteral("/media"));' "$window_source" \
    "Remote directory selector must surface the /media path even when expansion is unavailable."
require_pattern 'mediaItem->setData(0, loadedRole, false);' "$window_source" \
    "Remote directory selector must keep /media children lazy-loaded instead of preloading the subtree."
require_pattern 'homeItem->setData(0, loadedRole, false);' "$window_source" \
    "Remote directory selector must keep the home root lazy-loaded instead of preloading the subtree."
if grep -Fq 'QTreeWidgetItem *startItem = new QTreeWidgetItem(treeWidget);' "$window_source"; then
    echo "FAIL: Remote directory selector must not create a third top-level start-path root." >&2
    exit 1
fi
require_pattern 'connect(treeWidget, &QTreeWidget::itemExpanded, &dialog, loadChildren);' "$window_source" \
    "Remote directory selector must keep lazy-loading on expansion."
if grep -Fq 'treeWidget->expandItem(rootItem);' "$window_source"; then
    echo "FAIL: Remote directory selector must not auto-expand the root branch on open." >&2
    exit 1
fi
if grep -Fq 'QTreeWidgetItem *rootItem = new QTreeWidgetItem(treeWidget);' "$window_source"; then
    echo "FAIL: Remote directory selector must not collapse home and /media under a single synthetic root." >&2
    exit 1
fi
require_pattern 'QString derivedRemoteName = m_vaultValidator->getVaultName(vault.remotePath);' "$window_source" \
    "Remote repository add flow must derive a default repository name from the selected path."
require_pattern 'vault.name = nameEdit->text().trimmed().isEmpty()' "$window_source" \
    "Remote repository add flow must branch on whether the repository name was left blank."
require_pattern '? derivedRemoteName' "$window_source" \
    "Remote repository add flow must derive the repository name from the selected path when left blank."
require_pattern '不用提示再输入项目名称' "$task_file" \
    "Task-002 file must continue to describe the no-extra-project-name requirement."

echo "PASS: task-002 remote repository selector guard rails are wired."
