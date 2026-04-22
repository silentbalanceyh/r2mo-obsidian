#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
window_header="$repo_root/src/mainwindow.h"
window_source="$repo_root/src/mainwindow.cpp"
theme_source="$repo_root/src/theme/thememanager.cpp"
ts_en="$repo_root/translations/obsidianmanager_en_US.ts"
ts_zh="$repo_root/translations/obsidianmanager_zh_CN.ts"

require_pattern() {
    local pattern="$1"
    local file="$2"
    local message="$3"

    if ! grep -Fq "$pattern" "$file"; then
        echo "FAIL: $message" >&2
        exit 1
    fi
}

forbid_pattern() {
    local pattern="$1"
    local file="$2"
    local message="$3"

    if grep -Fq "$pattern" "$file"; then
        echo "FAIL: $message" >&2
        exit 1
    fi
}

require_pattern 'm_addRemoteBtn;' "$window_header" \
    "MainWindow must keep a dedicated remote toolbar button for retranslation."
require_pattern 'addRemoteRepository' "$window_header" \
    "MainWindow must expose a dedicated add-remote action."
require_pattern 'setupToolBar' "$window_header" \
    "MainWindow must own toolbar setup for the new button layout."
require_pattern 'm_addRemoteBtn = new QPushButton' "$window_source" \
    "Toolbar setup must create the remote repository button."
require_pattern 'm_addRemoteBtn->setText' "$window_source" \
    "Remote repository button must remain an icon-style control."
require_pattern 'm_addRemoteBtn->setToolTip' "$window_source" \
    "Remote toolbar button must advertise the remote-repository action."
require_pattern 'connect(m_addRemoteBtn' "$window_source" \
    "Remote toolbar button must open the dedicated remote repository flow."
forbid_pattern 'QInputDialog::getItem' "$window_source" \
    "Adding repositories must no longer prompt for local-vs-remote selection."
require_pattern 'dialog.setWindowTitle(tr("Add Local Repository"))' "$window_source" \
    "Local add flow must present the localized local-repository title."
require_pattern 'dialog.setWindowTitle(tr("Add Remote Repository"))' "$window_source" \
    "Remote add flow must present the localized remote-repository title."
require_pattern 'dialog.setMinimumSize(900, 0)' "$window_source" \
    "Remote repository dialog must reserve a clearly wider minimum width."
require_pattern 'dialog.resize(980, 460)' "$window_source" \
    "Remote repository dialog must apply a roomier default size."
require_pattern 'layout->setContentsMargins(22, 18, 22, 18)' "$window_source" \
    "Remote repository dialog must keep visible side margins instead of full-width fields."
require_pattern 'layout->setSpacing(14)' "$window_source" \
    "Remote repository dialog must keep balanced spacing between form and actions."
require_pattern 'form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow)' "$window_source" \
    "Remote repository form must allow input controls to expand."
require_pattern 'form->setHorizontalSpacing(18)' "$window_source" \
    "Remote repository form must increase horizontal spacing for readability."
require_pattern 'form->setVerticalSpacing(14)' "$window_source" \
    "Remote repository form must increase vertical spacing for readability."
require_pattern 'form->setContentsMargins(0, 4, 0, 0)' "$window_source" \
    "Remote repository form must preserve inner top breathing room."
require_pattern 'edit->setMinimumWidth(520)' "$window_source" \
    "Remote repository text inputs must use a larger minimum width."
require_pattern 'edit->setMaximumWidth(760)' "$window_source" \
    "Remote repository text inputs must stop short of fully spanning the dialog width."
require_pattern 'edit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed)' "$window_source" \
    "Remote repository text inputs must stay roomy without flattening against both edges."
require_pattern 'browseRemoteDirBtn->setMaximumWidth(220)' "$window_source" \
    "Remote directory browse button must stay compact within the wider dialog."
require_pattern 'buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"))' "$window_source" \
    "Remote repository dialog must localize the OK button text explicitly."
require_pattern 'buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"))' "$window_source" \
    "Remote repository dialog must localize the Cancel button text explicitly."
require_pattern 'MainWindow::selectRemoteDirectory' "$window_source" \
    "Remote repository selection must be factored into a tree dialog helper."
require_pattern 'QTreeWidget *treeWidget = new QTreeWidget' "$window_source" \
    "Remote directory selector must use a tree widget."
require_pattern 'treeWidget->setRootIsDecorated(true)' "$window_source" \
    "Remote directory selector must support tree-style hierarchy."
require_pattern 'connect(treeWidget, &QTreeWidget::itemExpanded' "$window_source" \
    "Remote directory selector must lazy-load child directories."
require_pattern 'connect(treeWidget, &QTreeWidget::itemDoubleClicked' "$window_source" \
    "Remote directory selector must support double-click selection."
require_pattern 'tr("Select a remote project directory:")' "$window_source" \
    "Remote directory selector must show localized helper text."
require_pattern 'tr("📂 = Remote directory (double-click to select or expand for children)")' "$window_source" \
    "Remote directory selector must explain the tree interaction."
require_pattern 'tr("Remote Address and Username are required before browsing remote directories.")' "$window_source" \
    "Remote directory browsing must guard against incomplete SSH credentials."
require_pattern 'tr("Local Repositories")' "$window_source" \
    "Vault list must render a localized local repositories group."
require_pattern 'tr("Remote Repositories")' "$window_source" \
    "Vault list must render a localized remote repositories group."
require_pattern 'm_addBtn->setText' "$window_source" \
    "Retranslation must preserve the local add icon button."
require_pattern 'm_removeBtn->setText' "$window_source" \
    "Retranslation must preserve the remove icon button."
require_pattern 'm_addRemoteBtn->setText' "$window_source" \
    "Retranslation must preserve the remote add icon button."
require_pattern 'updateVaultList();' "$window_source" \
    "Language changes must refresh the repository group headings."
require_pattern 'QComboBox QAbstractItemView' "$theme_source" \
    "Theme styling must explicitly cover combo box popup readability."
require_pattern 'QPushButton#addRemoteBtnMiddle' "$theme_source" \
    "Theme styling must include the remote toolbar button."
require_pattern '<source>Add Local Repository</source>' "$ts_en" \
    "English translations must include the local repository action."
require_pattern '<source>Add Remote Repository</source>' "$ts_en" \
    "English translations must include the remote repository action."
require_pattern '<source>Local Repositories</source>' "$ts_en" \
    "English translations must include the repositories group titles."
require_pattern '<source>Remote Repositories</source>' "$ts_en" \
    "English translations must include the remote group title."
require_pattern '<source>OK</source>' "$ts_en" \
    "English translations must include the OK action label."
require_pattern '<source>Cancel</source>' "$ts_en" \
    "English translations must include the Cancel action label."
require_pattern '<source>Add Local Repository</source>' "$ts_zh" \
    "Chinese translations must include the local repository action."
require_pattern '<translation>添加远程仓库</translation>' "$ts_zh" \
    "Chinese translations must include the remote repository action."
require_pattern '<translation>本地仓库</translation>' "$ts_zh" \
    "Chinese translations must include the local group title."
require_pattern '<translation>远程仓库</translation>' "$ts_zh" \
    "Chinese translations must include the remote group title."
require_pattern '<translation>确定</translation>' "$ts_zh" \
    "Chinese translations must include the OK action label."
require_pattern '<translation>取消</translation>' "$ts_zh" \
    "Chinese translations must include the Cancel action label."

echo "PASS: task-001 UI repository and i18n guard rails are wired."
