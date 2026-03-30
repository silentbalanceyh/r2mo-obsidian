#!/bin/bash
# dev-start.sh - Start development environment
# Usage: ./dev-start.sh

set -e

QT_PATH="/Users/lang/Qt/6.10.0/macos"
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"

echo "=== Obsidian Manager Dev Start ==="
echo "Qt Path: $QT_PATH"
echo "Project: $PROJECT_DIR"

# Check Qt installation
if [ ! -d "$QT_PATH" ]; then
    echo "ERROR: Qt 6.10.0 not found at $QT_PATH"
    echo "Please install Qt 6.10.0 or update QT_PATH in this script"
    exit 1
fi

# Configure if build dir doesn't exist
if [ ! -d "$BUILD_DIR" ]; then
    echo "Configuring CMake..."
    cmake -B "$BUILD_DIR" -DCMAKE_PREFIX_PATH="$QT_PATH"
fi

# Build
echo "Building..."
cmake --build "$BUILD_DIR" --parallel

# Run
echo "Starting application..."
APP_PATH="$BUILD_DIR/ObsidianManager.app/Contents/MacOS/ObsidianManager"
if [ -f "$APP_PATH" ]; then
    "$APP_PATH"
else
    echo "ERROR: Application not found at $APP_PATH"
    exit 1
fi