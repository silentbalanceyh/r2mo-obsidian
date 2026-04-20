#!/bin/bash
# dev-start.sh - Rebuild and restart Obsidian Manager for local development
# Usage: ./dev-start.sh

set -euo pipefail

QT_PATH="/Users/lang/Qt/6.10.0/macos"
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
PID_FILE="$PROJECT_DIR/.dev-app.pid"
LOG_FILE="$PROJECT_DIR/.dev-build.log"
APP_BUNDLE="$BUILD_DIR/ObsidianManager.app"
APP_EXECUTABLE="$APP_BUNDLE/Contents/MacOS/ObsidianManager"
APP_BUNDLE_ID="com.r2mo.obsidianmanager"

timestamp() {
    date '+%Y-%m-%d %H:%M:%S'
}

log() {
    printf '[%s] %s\n' "$(timestamp)" "$*" | tee -a "$LOG_FILE"
}

activate_app() {
    osascript <<EOF >/dev/null 2>&1 || true
tell application id "$APP_BUNDLE_ID" to activate
EOF
}

collect_app_pids() {
    pgrep -f "$APP_EXECUTABLE" || true
}

wait_for_live_app() {
    local timeout_seconds="$1"
    local waited=0
    while [ "$waited" -lt "$timeout_seconds" ]; do
        if [ -n "$(collect_app_pids)" ]; then
            return 0
        fi
        sleep 1
        waited=$((waited + 1))
    done
    return 1
}

: > "$LOG_FILE"

log "=== Obsidian Manager Dev Start ==="
log "Project: $PROJECT_DIR"
log "Qt Path: $QT_PATH"

if [ ! -d "$QT_PATH" ]; then
    log "ERROR: Qt 6.10.0 not found at $QT_PATH"
    exit 1
fi

log "Stopping previous instances..."
"$PROJECT_DIR/dev-stop.sh" | tee -a "$LOG_FILE"

log "Configuring CMake..."
cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" \
    -DCMAKE_PREFIX_PATH="$QT_PATH" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON 2>&1 | tee -a "$LOG_FILE"

log "Building..."
cmake --build "$BUILD_DIR" --parallel 2>&1 | tee -a "$LOG_FILE"

if [ ! -f "$APP_EXECUTABLE" ]; then
    log "ERROR: Application not found at $APP_EXECUTABLE"
    exit 1
fi

log "Starting application..."
open -n "$APP_BUNDLE" 2>&1 | tee -a "$LOG_FILE"

if ! wait_for_live_app 5; then
    log "ERROR: Application process did not stay alive after launch."
    rm -f "$PID_FILE"
    exit 1
fi

APP_PID="$(collect_app_pids | tail -n 1)"
echo "$APP_PID" > "$PID_FILE"

sleep 1
activate_app

log "Application started successfully."
log "PID: $APP_PID"
log "Bundle: $APP_BUNDLE"

printf '\nRestart complete. PID: %s\nLog: %s\n' "$APP_PID" "$LOG_FILE"
