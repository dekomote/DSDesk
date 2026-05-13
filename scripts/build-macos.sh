#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${PROJECT_DIR}/build-macos"
DIST_DIR="${PROJECT_DIR}/dist"

RED='\033[0;31m'
GREEN='\033[0;32m'
BOLD='\033[1m'
RESET='\033[0m'

mkdir -p "$DIST_DIR"

echo -e "${BOLD}Building DSDesk for macOS${RESET}"

# Configure
echo -e "\n${BOLD}Configuring...${RESET}"
cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES="arm64;" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0

# Build
echo -e "\n${BOLD}Building...${RESET}"
cmake --build "$BUILD_DIR" --parallel

# Package with CPack (creates DMG)
echo -e "\n${BOLD}Packaging DMG...${RESET}"
cd "$BUILD_DIR"
cpack -G DragNDrop -DCPACK_PACKAGE_FILE_NAME="DSDesk"

# Copy to dist/
cp -v "$BUILD_DIR"/*.dmg "$DIST_DIR/" 2>/dev/null || true

echo -e "\n${GREEN}Done!${RESET}"
ls -lh "$DIST_DIR"/*.dmg 2>/dev/null || true
