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
WIT_DIR="$PROJECT_ROOT/wit"

echo -e "${BLUE}🔍 Validating WIT interfaces...${NC}"

# Check if wasm-tools is installed
if ! command -v wasm-tools &> /dev/null; then
    echo -e "${RED}❌ wasm-tools not found. Please run './scripts/install-toolchain.sh' first.${NC}"
    exit 1
fi

# Track validation results
TOTAL_FILES=0
VALID_FILES=0
FAILED_FILES=0

# Validate each WIT file
validate_wit_file() {
    local file=$1
    local relative_path=${file#$PROJECT_ROOT/}
    
    echo -n "  Validating $relative_path... "
    
    if wasm-tools parse "$file" &> /dev/null; then
        echo -e "${GREEN}✓${NC}"
        ((VALID_FILES++))
        return 0
    else
        echo -e "${RED}✗${NC}"
        echo -e "${RED}    Error: $(wasm-tools parse "$file" 2>&1)${NC}"
        ((FAILED_FILES++))
        return 1
    fi
}

# Find and validate all WIT files
echo -e "${YELLOW}Scanning for WIT files in: $WIT_DIR${NC}"

while IFS= read -r -d '' wit_file; do
    ((TOTAL_FILES++))
    validate_wit_file "$wit_file" || true
done < <(find "$WIT_DIR" -name "*.wit" -type f -print0 | sort -z)

# Validate world composition
echo -e "\n${BLUE}🌍 Validating world composition...${NC}"
if [ -f "$WIT_DIR/world.wit" ]; then
    echo -n "  Checking world.wit imports... "
    if wasm-tools parse "$WIT_DIR/world.wit" &> /dev/null; then
        echo -e "${GREEN}✓${NC}"
    else
        echo -e "${RED}✗${NC}"
        echo -e "${RED}    Error: World composition failed${NC}"
    fi
else
    echo -e "${YELLOW}  Warning: world.wit not found${NC}"
fi

# Summary
echo -e "\n${BLUE}📊 Validation Summary:${NC}"
echo -e "  Total files:  $TOTAL_FILES"
echo -e "  Valid files:  ${GREEN}$VALID_FILES${NC}"
echo -e "  Failed files: ${RED}$FAILED_FILES${NC}"

if [ $FAILED_FILES -eq 0 ]; then
    echo -e "\n${GREEN}✅ All WIT interfaces validated successfully!${NC}"
    exit 0
else
    echo -e "\n${RED}❌ WIT validation failed! Please fix the errors above.${NC}"
    exit 1
fi
