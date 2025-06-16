#!/usr/bin/env bash
set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"

echo -e "${BLUE}🔧 Generating bindings for all languages...${NC}"

# Check if wit-bindgen is installed
if ! command -v wit-bindgen &> /dev/null; then
    echo -e "${RED}❌ wit-bindgen not found. Please run './scripts/install-toolchain.sh' first.${NC}"
    exit 1
fi

# Create bindings directory if it doesn't exist
mkdir -p "$PROJECT_ROOT/bindings"

# Clean previous bindings
echo -e "${YELLOW}🧹 Cleaning previous bindings...${NC}"
rm -rf "$PROJECT_ROOT/bindings"/*

# Track generation results
TOTAL_LANGUAGES=0
SUCCESS_COUNT=0
FAILED_COUNT=0

# Generate bindings for each language
generate_language() {
    local script=$1
    local language=$2
    
    ((TOTAL_LANGUAGES++))
    echo -e "\n${BLUE}📦 Generating $language bindings...${NC}"
    
    if [ -x "$SCRIPT_DIR/$script" ]; then
        if "$SCRIPT_DIR/$script"; then
            echo -e "${GREEN}✅ $language bindings generated successfully${NC}"
            ((SUCCESS_COUNT++))
        else
            echo -e "${RED}❌ Failed to generate $language bindings${NC}"
            ((FAILED_COUNT++))
        fi
    else
        echo -e "${YELLOW}⚠️  $script not found or not executable${NC}"
        ((FAILED_COUNT++))
    fi
}

# Generate each language
generate_language "generate-cpp.sh" "C++"
generate_language "generate-go.sh" "Go"
generate_language "generate-typescript.sh" "TypeScript"
generate_language "generate-rust.sh" "Rust"

# Summary
echo -e "\n${BLUE}📊 Generation Summary:${NC}"
echo -e "  Total languages: $TOTAL_LANGUAGES"
echo -e "  Successful:      ${GREEN}$SUCCESS_COUNT${NC}"
echo -e "  Failed:          ${RED}$FAILED_COUNT${NC}"

if [ $FAILED_COUNT -eq 0 ]; then
    echo -e "\n${GREEN}✅ All bindings generated successfully!${NC}"
    
    # List generated bindings
    echo -e "\n${BLUE}📁 Generated bindings:${NC}"
    find "$PROJECT_ROOT/bindings" -type d -mindepth 1 -maxdepth 1 | while read -r dir; do
        echo -e "  - $(basename "$dir")"
    done
    
    exit 0
else
    echo -e "\n${YELLOW}⚠️  Some bindings failed to generate. Check the errors above.${NC}"
    exit 1
fi
