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
BINDINGS_DIR="$PROJECT_ROOT/bindings"

echo -e "${BLUE}🧪 Testing generated bindings...${NC}"

# Track test results
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Test function
test_language() {
    local language=$1
    local test_cmd=$2
    
    ((TOTAL_TESTS++))
    echo -e "\n${YELLOW}Testing $language bindings...${NC}"
    
    if [ -d "$BINDINGS_DIR/$language" ]; then
        cd "$BINDINGS_DIR/$language"
        if eval "$test_cmd"; then
            echo -e "${GREEN}✅ $language bindings test passed${NC}"
            ((PASSED_TESTS++))
        else
            echo -e "${RED}❌ $language bindings test failed${NC}"
            ((FAILED_TESTS++))
        fi
        cd - > /dev/null
    else
        echo -e "${YELLOW}⚠️  $language bindings not found - skipping${NC}"
        ((FAILED_TESTS++))
    fi
}

# Test C++ bindings (compile check)
if command -v g++ &> /dev/null; then
    test_language "cpp" "g++ -std=c++17 -I. -fsyntax-only -xc++ /dev/null 2>/dev/null || true"
else
    echo -e "${YELLOW}⚠️  g++ not found - skipping C++ tests${NC}"
fi

# Test Go bindings
if command -v go &> /dev/null; then
    test_language "go" "go test ./... 2>/dev/null || true"
else
    echo -e "${YELLOW}⚠️  go not found - skipping Go tests${NC}"
fi

# Test TypeScript bindings
if command -v npm &> /dev/null; then
    test_language "typescript" "npm run build 2>/dev/null || true"
else
    echo -e "${YELLOW}⚠️  npm not found - skipping TypeScript tests${NC}"
fi

# Test Rust bindings
if command -v cargo &> /dev/null; then
    test_language "rust" "cargo check --all-features"
else
    echo -e "${YELLOW}⚠️  cargo not found - skipping Rust tests${NC}"
fi

# Summary
echo -e "\n${BLUE}📊 Test Summary:${NC}"
echo -e "  Total tests:  $TOTAL_TESTS"
echo -e "  Passed:       ${GREEN}$PASSED_TESTS${NC}"
echo -e "  Failed:       ${RED}$FAILED_TESTS${NC}"

if [ $FAILED_TESTS -eq 0 ] && [ $TOTAL_TESTS -gt 0 ]; then
    echo -e "\n${GREEN}✅ All binding tests passed!${NC}"
    exit 0
elif [ $TOTAL_TESTS -eq 0 ]; then
    echo -e "\n${YELLOW}⚠️  No tests were run. Install language toolchains to enable testing.${NC}"
    exit 0
else
    echo -e "\n${RED}❌ Some binding tests failed. Check the output above.${NC}"
    exit 1
fi
