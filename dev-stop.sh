#!/bin/bash
# dev-stop.sh - Stop all running Obsidian Manager dev instances
# Usage: ./dev-stop.sh

set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
PID_FILE="$PROJECT_DIR/.dev-app.pid"
APP_BUNDLE="$BUILD_DIR/ObsidianManager.app"
APP_EXECUTABLE="$APP_BUNDLE/Contents/MacOS/ObsidianManager"
APP_BUNDLE_ID="com.r2mo.obsidianmanager"

collect_pids() {
    local pids=()

    if [ -f "$PID_FILE" ]; then
        local pid=""
        pid="$(cat "$PID_FILE" 2>/dev/null || true)"
        if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
            pids+=("$pid")
        fi
    fi

    while IFS= read -r pid; do
        [ -n "$pid" ] && pids+=("$pid")
    done < <(pgrep -f "$APP_EXECUTABLE" || true)

    while IFS= read -r pid; do
        [ -n "$pid" ] && pids+=("$pid")
    done < <(pgrep -x "ObsidianManager" || true)

    printf '%s\n' "${pids[@]:-}" | awk 'NF && !seen[$0]++'
}

quit_gui_app() {
osascript <<EOF >/dev/null 2>&1 || true
tell application id "$APP_BUNDLE_ID" to quit
EOF
}

wait_for_exit() {
    local timeout_seconds="$1"
    local waited=0
    while [ "$waited" -lt "$timeout_seconds" ]; do
        if [ -z "$(collect_pids)" ]; then
            return 0
        fi
        sleep 1
        waited=$((waited + 1))
    done
    return 1
}

echo "=== Obsidian Manager Dev Stop ==="

quit_gui_app

running_pids=()
while IFS= read -r pid; do
    [ -n "$pid" ] && running_pids+=("$pid")
done < <(collect_pids)
if [ "${#running_pids[@]}" -eq 0 ]; then
    rm -f "$PID_FILE"
    echo "No running Obsidian Manager instance found."
    exit 0
fi

echo "Stopping PIDs: ${running_pids[*]}"
kill "${running_pids[@]}" 2>/dev/null || true

if ! wait_for_exit 5; then
    running_pids=()
    while IFS= read -r pid; do
        [ -n "$pid" ] && running_pids+=("$pid")
    done < <(collect_pids)
    if [ "${#running_pids[@]}" -gt 0 ]; then
        echo "Processes still alive after SIGTERM, forcing stop: ${running_pids[*]}"
        kill -9 "${running_pids[@]}" 2>/dev/null || true
        wait_for_exit 2 || true
    fi
fi

rm -f "$PID_FILE"

if [ -n "$(collect_pids)" ]; then
    echo "Warning: some Obsidian Manager processes may still be alive."
    exit 1
fi

echo "Stopped."
