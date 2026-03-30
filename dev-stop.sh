#!/bin/bash
# dev-stop.sh - Stop running Obsidian Manager process
# Usage: ./dev-stop.sh

echo "=== Obsidian Manager Dev Stop ==="

# Find and kill ObsidianManager process
PID=$(pgrep -f "ObsidianManager" | head -1)

if [ -n "$PID" ]; then
    echo "Stopping process $PID..."
    kill "$PID" 2>/dev/null || true
    echo "Stopped."
else
    echo "No running ObsidianManager process found."
fi