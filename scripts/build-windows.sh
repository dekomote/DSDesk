#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${PROJECT_DIR}/build-win"

RED='\033[0;31m'
GREEN='\033[0;32m'
BOLD='\033[1m'
RESET='\033[0m'

echo -e "${BOLD}Building DSDesk for Windows${RESET}"

# Configure
echo -e "\n${BOLD}Configuring...${RESET}"
cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" \
    --preset windows

# Build
echo -e "\n${BOLD}Building...${RESET}"
cmake --build --preset windows --parallel

# Package with CPack
echo -e "\n"${BOLD}"Packaging with NSIS..."${RESET}
cd "$BUILD_DIR"
cpack -G NSIS -DCPACK_PACKAGE_FILE_NAME="DSDesk-Installer"

echo -e "\n${GREEN}Done!${RESET}"
ls -lh "$BUILD_DIR"/*.exe 2>/dev/null || true
