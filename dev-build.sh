#!/bin/bash
# dev-build.sh - Build Obsidian Manager
# Usage: ./dev-build.sh [clean|rebuild]

set -e

QT_PATH="/Users/lang/Qt/6.10.0/macos"
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"

ACTION="${1:-build}"

echo "=== Obsidian Manager Dev Build ==="
echo "Action: $ACTION"
echo "Qt Path: $QT_PATH"

# Check Qt installation
if [ ! -d "$QT_PATH" ]; then
    echo "ERROR: Qt 6.10.0 not found at $QT_PATH"
    exit 1
fi

case "$ACTION" in
    clean)
        echo "Cleaning build directory..."
        rm -rf "$BUILD_DIR"
        echo "Cleaned."
        ;;
    rebuild)
        echo "Rebuilding..."
        rm -rf "$BUILD_DIR"
        cmake -B "$BUILD_DIR" -DCMAKE_PREFIX_PATH="$QT_PATH"
        cmake --build "$BUILD_DIR" --parallel
        echo "Rebuild complete."
        ;;
    build|*)
        if [ ! -d "$BUILD_DIR" ]; then
            echo "Configuring CMake..."
            cmake -B "$BUILD_DIR" -DCMAKE_PREFIX_PATH="$QT_PATH"
        fi
        echo "Building..."
        cmake --build "$BUILD_DIR" --parallel
        echo "Build complete."
        ;;
esac

# Show result
if [ -f "$BUILD_DIR/ObsidianManager.app/Contents/MacOS/ObsidianManager" ]; then
    echo "Application: $BUILD_DIR/ObsidianManager.app"
    ls -la "$BUILD_DIR/ObsidianManager.app/Contents/MacOS/ObsidianManager"
fi