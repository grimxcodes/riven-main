#!/usr/bin/env bash
# ──────────────────────────────────────────────────────────────────────────
#  build.sh — Quick build script for the Riven Compiler (rivc)
#  Usage:  ./build.sh [--debug] [--clean]
# ──────────────────────────────────────────────────────────────────────────
set -e

BOLD="\033[1m"
GREEN="\033[1;32m"
CYAN="\033[1;36m"
RED="\033[1;31m"
RESET="\033[0m"

BUILD_TYPE="Release"
CLEAN=0

for arg in "$@"; do
    case $arg in
        --debug) BUILD_TYPE="Debug" ;;
        --clean) CLEAN=1 ;;
        --help)
            echo "Usage: ./build.sh [--debug] [--clean]"
            echo "  --debug   Build with debug symbols"
            echo "  --clean   Remove build directory first"
            exit 0 ;;
    esac
done

echo -e "${CYAN}Riven Compiler Build System${RESET}"
echo -e "${BOLD}Build type: ${BUILD_TYPE}${RESET}"

if [ "$CLEAN" = "1" ]; then
    echo -e "${CYAN}Cleaning build directory...${RESET}"
    rm -rf build/
fi

mkdir -p build
cd build

cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    2>&1 | grep -E "Riven|Build type|Compiler|Output|error|warning" || true

make -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"

cd ..

echo -e "${GREEN}✓ Build successful!${RESET}"
echo -e "${BOLD}Binary: ./build/rivc${RESET}"
echo ""
echo -e "Quick test:"
echo -e "  ${CYAN}./build/rivc examples/hello.rn -o hello && ./hello${RESET}"

