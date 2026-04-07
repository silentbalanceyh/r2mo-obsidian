#!/bin/bash
# dev-stop.sh - Stop running Obsidian Manager process
# Usage: ./dev-stop.sh

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
PID_FILE="$PROJECT_DIR/.dev-app.pid"

echo "=== Obsidian Manager Dev Stop ==="

# Stop via PID file
if [ -f "$PID_FILE" ]; then
    PID=$(cat "$PID_FILE")
    if kill -0 "$PID" 2>/dev/null; then
        echo "Stopping process $PID..."
        kill "$PID" 2>/dev/null || true
        echo "Stopped."
    else
        echo "Process $PID not running (stale PID file)"
    fi
    rm -f "$PID_FILE"
else
    # Fallback: find by process name
    PID=$(pgrep -f "ObsidianManager" | head -1)
    if [ -n "$PID" ]; then
        echo "Stopping process $PID..."
        kill "$PID" 2>/dev/null || true
        echo "Stopped."
    else
        echo "No running ObsidianManager process found."
    fi
fi