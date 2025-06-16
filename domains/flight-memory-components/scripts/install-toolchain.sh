#!/usr/bin/env bash
set -euo pipefail

echo "🚀 Installing WIT toolchain for flight-memory-components..."

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check for required base tools
check_required_tool() {
    if ! command -v "$1" &> /dev/null; then
        echo -e "${RED}❌ $1 not found. $2${NC}"
        return 1
    fi
    return 0
}

# Rust toolchain (required base)
if ! check_required_tool "cargo" "Please install Rust from https://rustup.rs/"; then
    exit 1
fi

# Install WIT tools with specific versions
install_cargo_tool() {
    local tool=$1
    local version=$2
    echo -e "${YELLOW}📦 Installing $tool ($version)...${NC}"
    if cargo install "$tool" --version "$version" --locked; then
        echo -e "${GREEN}✅ $tool installed successfully${NC}"
    else
        echo -e "${RED}❌ Failed to install $tool${NC}"
        exit 1
    fi
}

# Core WIT tools
install_cargo_tool "wit-bindgen-cli" "0.24.0"
install_cargo_tool "wasm-tools" "1.0.60"
install_cargo_tool "cargo-component" "0.7.0"

# Language-specific tools
if command -v npm &> /dev/null; then
    echo -e "${YELLOW}📦 Installing TypeScript tools...${NC}"
    if npm install -g @bytecodealliance/jco; then
        echo -e "${GREEN}✅ TypeScript tools installed${NC}"
    else
        echo -e "${YELLOW}⚠️  Warning: Failed to install TypeScript tools${NC}"
    fi
else
    echo -e "${YELLOW}⚠️  npm not found - skipping TypeScript tools${NC}"
fi

if command -v go &> /dev/null; then
    echo -e "${YELLOW}📦 Installing Go tools...${NC}"
    if go install github.com/bytecodealliance/wit-bindgen-go@latest; then
        echo -e "${GREEN}✅ Go tools installed${NC}"
    else
        echo -e "${YELLOW}⚠️  Warning: Failed to install Go tools${NC}"
    fi
else
    echo -e "${YELLOW}⚠️  go not found - skipping Go tools${NC}"
fi

# Verify installations
echo -e "\n${YELLOW}🔍 Verifying installations...${NC}"
echo -e "wit-bindgen version: $(wit-bindgen --version 2>/dev/null || echo 'NOT FOUND')"
echo -e "wasm-tools version: $(wasm-tools --version 2>/dev/null || echo 'NOT FOUND')"
echo -e "cargo-component version: $(cargo component --version 2>/dev/null || echo 'NOT FOUND')"

echo -e "\n${GREEN}✅ WIT toolchain installation complete!${NC}"
echo -e "You can now run './scripts/validate-wit.sh' to validate WIT interfaces"
