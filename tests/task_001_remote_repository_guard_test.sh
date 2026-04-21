#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
vault_model_header="$repo_root/src/models/vaultmodel.h"
vault_model_source="$repo_root/src/models/vaultmodel.cpp"
window_header="$repo_root/src/mainwindow.h"
window_source="$repo_root/src/mainwindow.cpp"
cmake_file="$repo_root/CMakeLists.txt"

require_pattern() {
    local pattern="$1"
    local file="$2"
    local message="$3"

    if ! grep -Eq "$pattern" "$file"; then
        echo "FAIL: $message" >&2
        exit 1
    fi
}

require_pattern 'enum class[[:space:]]+VaultKind' "$vault_model_header" \
    "Vault model must distinguish local and remote repositories."
require_pattern 'QString[[:space:]]+host;' "$vault_model_header" \
    "Remote repository records must persist the SSH host."
require_pattern 'QString[[:space:]]+username;' "$vault_model_header" \
    "Remote repository records must persist the SSH username."
require_pattern 'QString[[:space:]]+password;' "$vault_model_header" \
    "Remote repository records must persist the SSH password when password auth is enabled."
require_pattern 'bool[[:space:]]+useKeyAuth' "$vault_model_header" \
    "Remote repository records must persist whether key-based auth is enabled."
require_pattern 'QString[[:space:]]+remotePath;' "$vault_model_header" \
    "Remote repository records must persist the selected remote directory."
require_pattern 'connectionStatus' "$vault_model_header" \
    "Vault model must persist the latest remote connectivity status."
require_pattern 'lastConnectionCheck' "$vault_model_header" \
    "Vault model must persist the latest remote connectivity check timestamp."
require_pattern 'QTimer[[:space:]]*\\*[[:space:]]*m_remoteConnectivityTimer;' "$window_header" \
    "MainWindow must own a dedicated 30-second remote connectivity timer."
require_pattern 'm_remoteConnectivityTimer[[:space:]]*=[[:space:]]*new[[:space:]]+QTimer\\(this\\);' "$window_source" \
    "MainWindow must initialize the remote connectivity timer."
require_pattern 'm_remoteConnectivityTimer->setInterval\\([[:space:]]*30[[:space:]]*\\*[[:space:]]*1000[[:space:]]*\\)' "$window_source" \
    "Remote connectivity checks must run every 30 seconds."
require_pattern 'checkRemoteVaultConnectivityAsync' "$window_header" \
    "MainWindow must expose an async remote connectivity refresh entry point."
require_pattern 'refreshRemoteConnectivityStatuses' "$window_header" \
    "MainWindow must expose a helper to schedule remote connectivity checks."
require_pattern 'Remote Repositories' "$window_source" \
    "Project list UI must render a dedicated remote repositories group."
require_pattern 'Local Repositories' "$window_source" \
    "Project list UI must render a dedicated local repositories group."
require_pattern 'Add Remote Repository' "$window_source" \
    "UI must expose an add-remote-repository flow."
require_pattern 'Remote Address' "$window_source" \
    "Remote repository dialog must collect the SSH host/address."
require_pattern 'Passwordless Login' "$window_source" \
    "Remote repository dialog must support passwordless login mode."
require_pattern 'Show Password' "$window_source" \
    "Remote repository dialog must support password visibility toggling."
require_pattern 'Remote Directory' "$window_source" \
    "Remote repository dialog must support remote directory selection."
require_pattern 'src/utils/sshremoteexecutor.cpp' "$cmake_file" \
    "CMake must compile the SSH remote executor utility."

echo "PASS: task-001 remote repository guard rails are wired."
