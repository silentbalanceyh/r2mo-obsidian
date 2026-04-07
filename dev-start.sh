#!/bin/bash
# dev-start.sh - Start development environment (restart mode)
# Usage: ./dev-start.sh
# Kills existing app, rebuilds, and starts fresh
# Auto-backgrounds itself so terminal remains usable

QT_PATH="/Users/lang/Qt/6.10.0/macos"
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
PID_FILE="$PROJECT_DIR/.dev-app.pid"
APP_PATH="$BUILD_DIR/ObsidianManager.app/Contents/MacOS/ObsidianManager"
LOG_FILE="$PROJECT_DIR/.dev-build.log"

# Auto-background if running in foreground
if [ -z "$DEV_START_BG" ]; then
    echo "Starting dev build in background..."
    export DEV_START_BG=1
    nohup "$0" > "$LOG_FILE" 2>&1 &
    echo "Build started. Log: $LOG_FILE"
    echo "Terminal is free. Run './dev-start.sh' again to restart."
    exit 0
fi

# --- Below runs in background ---

# --- Below runs in background ---
set -e

echo "=== Obsidian Manager Dev Start ===" >> "$LOG_FILE"
echo "Qt Path: $QT_PATH" >> "$LOG_FILE"
echo "Project: $PROJECT_DIR" >> "$LOG_FILE"

# Check Qt installation
if [ ! -d "$QT_PATH" ]; then
    echo "ERROR: Qt 6.10.0 not found at $QT_PATH" >> "$LOG_FILE"
    exit 1
fi

# Kill existing app if running
if [ -f "$PID_FILE" ]; then
    OLD_PID=$(cat "$PID_FILE")
    if kill -0 "$OLD_PID" 2>/dev/null; then
        echo "Stopping previous app (PID: $OLD_PID)..." >> "$LOG_FILE"
        kill "$OLD_PID" 2>/dev/null || true
        sleep 0.5
    fi
    rm -f "$PID_FILE"
fi

# Configure if build dir doesn't exist
if [ ! -d "$BUILD_DIR" ]; then
    echo "Configuring CMake..." >> "$LOG_FILE"
    cmake -B "$BUILD_DIR" -DCMAKE_PREFIX_PATH="$QT_PATH" >> "$LOG_FILE" 2>&1
fi

# Build
echo "Building..." >> "$LOG_FILE"
cmake --build "$BUILD_DIR" --parallel >> "$LOG_FILE" 2>&1

# Run in background, detach from terminal
echo "Starting application..." >> "$LOG_FILE"
if [ -f "$APP_PATH" ]; then
    "$APP_PATH" &
    APP_PID=$!
    echo "$APP_PID" > "$PID_FILE"
    echo "App started (PID: $APP_PID)" >> "$LOG_FILE"
else
    echo "ERROR: Application not found at $APP_PATH" >> "$LOG_FILE"
    exit 1
fi