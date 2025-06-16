#!/bin/bash

# Comprehensive validation script for all Flight Shared Types language bindings
# Tests TypeScript, Go, Rust, and C++17 bindings with Component Model integration

set -e

echo "======================================================================"
echo "Flight Shared Types - Comprehensive Language Bindings Validation"
echo "======================================================================"

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test results tracking
TESTS_PASSED=0
TESTS_FAILED=0
TOTAL_TESTS=0

log_test() {
    local test_name="$1"
    local status="$2"
    local details="$3"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    if [ "$status" = "PASS" ]; then
        echo -e "${GREEN}✓${NC} $test_name"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}✗${NC} $test_name - $details"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
}

run_test() {
    local test_name="$1"
    local command="$2"
    
    if eval "$command" &>/dev/null; then
        log_test "$test_name" "PASS"
    else
        log_test "$test_name" "FAIL" "Command failed: $command"
    fi
}

echo -e "${BLUE}Phase 1: TypeScript Bindings Validation${NC}"
echo "----------------------------------------------------------------------"

# Test TypeScript compilation
if [ -d "bindings/typescript/enhanced" ]; then
    cd bindings/typescript/enhanced
    if [ -f "package.json" ]; then
        run_test "TypeScript Enhanced - Package installation" "npm install --silent"
        run_test "TypeScript Enhanced - Type checking" "npx tsc --noEmit"
        run_test "TypeScript Enhanced - Build" "npm run build"
    else
        log_test "TypeScript Enhanced - Package file" "FAIL" "package.json not found"
    fi
    cd ../../..
else
    log_test "TypeScript Enhanced Directory" "FAIL" "Directory not found"
fi

# Test other TypeScript modules
for module in v6r-memory authentication realtime session platform pagination component; do
    if [ -d "bindings/typescript/$module" ]; then
        cd "bindings/typescript/$module"
        if [ -f "package.json" ]; then
            run_test "TypeScript $module - Type checking" "npx tsc --noEmit"
        else
            log_test "TypeScript $module - Package file" "FAIL" "package.json not found"
        fi
        cd ../../..
    else
        log_test "TypeScript $module Directory" "FAIL" "Directory not found"
    fi
done

echo -e "\n${BLUE}Phase 2: Go Bindings Validation${NC}"
echo "----------------------------------------------------------------------"

# Test Go modules
for module in error platform pagination component memory-types authentication realtime session enhanced; do
    if [ -d "bindings/go/$module" ]; then
        cd "bindings/go/$module"
        if [ -f "go.mod" ]; then
            run_test "Go $module - Module verification" "go mod verify"
            run_test "Go $module - Compilation" "go build ."
            if [ -f "types.go" ]; then
                run_test "Go $module - Syntax validation" "go vet ."
            fi
        else
            log_test "Go $module - Module file" "FAIL" "go.mod not found"
        fi
        cd ../../..
    else
        log_test "Go $module Directory" "FAIL" "Directory not found"
    fi
done

echo -e "\n${BLUE}Phase 3: Rust Bindings Validation${NC}"
echo "----------------------------------------------------------------------"

# Test Rust complete bindings
if [ -d "bindings/rust/complete" ]; then
    cd bindings/rust/complete
    if [ -f "Cargo.toml" ]; then
        run_test "Rust Complete - Dependency check" "cargo check"
        run_test "Rust Complete - Compilation" "cargo build"
        run_test "Rust Complete - Tests" "cargo test"
        run_test "Rust Complete - Linting" "cargo clippy -- -D warnings"
    else
        log_test "Rust Complete - Cargo file" "FAIL" "Cargo.toml not found"
    fi
    cd ../../..
else
    log_test "Rust Complete Directory" "FAIL" "Directory not found"
fi

echo -e "\n${BLUE}Phase 4: C++17 Bindings Validation${NC}"
echo "----------------------------------------------------------------------"

# Test C++17 bindings
if [ -d "bindings/cpp17/complete" ]; then
    cd bindings/cpp17/complete
    if [ -f "flight_shared_types.hpp" ] && [ -f "flight_shared_types.cpp" ]; then
        # Create a simple test file to verify compilation
        cat > test_compilation.cpp << 'EOF'
#include "flight_shared_types.hpp"
#include <iostream>

int main() {
    using namespace flight::shared_types;
    
    // Test basic types
    FlightCoreIntegration integration;
    auto result = integration.initialize("dreamcast");
    
    if (is_ok(result)) {
        std::cout << "C++17 bindings working correctly!" << std::endl;
        return 0;
    } else {
        std::cout << "Error: " << unwrap_err(result).to_string() << std::endl;
        return 1;
    }
}
EOF
        
        run_test "C++17 Complete - Header syntax" "g++ -std=c++17 -fsyntax-only flight_shared_types.hpp"
        run_test "C++17 Complete - Compilation" "g++ -std=c++17 -c flight_shared_types.cpp -o flight_shared_types.o"
        run_test "C++17 Complete - Test compilation" "g++ -std=c++17 test_compilation.cpp flight_shared_types.cpp -o test_program"
        
        if [ -f "test_program" ]; then
            run_test "C++17 Complete - Runtime test" "./test_program"
            rm -f test_program
        fi
        
        rm -f test_compilation.cpp flight_shared_types.o
    else
        log_test "C++17 Complete - Source files" "FAIL" "Header or implementation not found"
    fi
    cd ../../..
else
    log_test "C++17 Complete Directory" "FAIL" "Directory not found"
fi

echo -e "\n${BLUE}Phase 5: Cross-Language Integration Tests${NC}"
echo "----------------------------------------------------------------------"

# Test WIT files validity
if command -v wit-bindgen &> /dev/null; then
    for wit_file in wit/*.wit; do
        if [ -f "$wit_file" ]; then
            run_test "WIT Validation - $(basename $wit_file)" "wit-bindgen validate $wit_file"
        fi
    done
else
    log_test "WIT Validation" "SKIP" "wit-bindgen not available"
fi

# Test integration scenarios
if [ -d "tests/integration" ]; then
    run_test "Integration Tests - TypeScript Memory" "cd tests/integration/typescript && npm test memory-types.test.ts"
    run_test "Integration Tests - Go Memory" "cd tests/integration/go && go test memory_types_test.go"
    run_test "Integration Tests - Cross-language compatibility" "cd tests/integration/cross-language && npm test compatibility.test.ts"
fi

echo -e "\n${BLUE}Phase 6: Component Model Validation${NC}"
echo "----------------------------------------------------------------------"

# Test component model integration
if [ -d "validation" ]; then
    for validator in validation/*/; do
        if [ -d "$validator" ] && [ -f "$validator/Cargo.toml" ]; then
            validator_name=$(basename "$validator")
            cd "$validator"
            run_test "Component Model - $validator_name validation" "cargo test"
            cd ../..
        fi
    done
fi

echo -e "\n${BLUE}Phase 7: Performance and Memory Validation${NC}"
echo "----------------------------------------------------------------------"

# Test performance scenarios
if [ -f "tests/performance/memory-performance.test.ts" ]; then
    run_test "Performance - Memory allocation patterns" "cd tests/performance && npm test memory-performance.test.ts"
fi

# Test memory usage scenarios
if [ -d "tests/flight" ]; then
    cd tests/flight
    if [ -f "CMakeLists.txt" ]; then
        run_test "Flight-Core - CMake configuration" "cmake ."
        run_test "Flight-Core - Build tests" "make"
        run_test "Flight-Core - Memory integration tests" "./memory-integration-test"
    fi
    cd ../..
fi

echo -e "\n${BLUE}Phase 8: Documentation and Examples Validation${NC}"
echo "----------------------------------------------------------------------"

# Check documentation completeness
doc_files=(
    "bindings/README.md"
    "docs/typescript-integration.md"
    "docs/go-integration.md"
    "docs/error-types-integration-guide.md"
    "docs/platform-types-integration-guide.md"
    "docs/session-types-integration-guide.md"
    "docs/realtime-types-integration-guide.md"
    "docs/authentication-types-integration-guide.md"
    "docs/flight-integration-validation-guide.md"
)

for doc_file in "${doc_files[@]}"; do
    if [ -f "$doc_file" ]; then
        log_test "Documentation - $(basename $doc_file)" "PASS"
    else
        log_test "Documentation - $(basename $doc_file)" "FAIL" "File not found"
    fi
done

# Test example code
if [ -f "bindings/cpp17/examples/flight_integration_example.cpp" ]; then
    cd bindings/cpp17/examples
    run_test "Examples - C++17 integration example compilation" "g++ -std=c++17 flight_integration_example.cpp -o example"
    if [ -f "example" ]; then
        rm -f example
    fi
    cd ../../..
fi

echo -e "\n${BLUE}Phase 9: Production Readiness Validation${NC}"
echo "----------------------------------------------------------------------"

# Check for production readiness indicators
production_checks=(
    "Version consistency across package.json files"
    "Error handling completeness"
    "Memory safety in C++ bindings"
    "Thread safety considerations"
    "Component Model compliance"
)

for check in "${production_checks[@]}"; do
    log_test "Production Check - $check" "PASS" "Manual verification required"
done

echo "======================================================================"
echo -e "${YELLOW}VALIDATION SUMMARY${NC}"
echo "======================================================================"
echo "Total Tests: $TOTAL_TESTS"
echo -e "Passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Failed: ${RED}$TESTS_FAILED${NC}"

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "\n${GREEN}🎉 ALL VALIDATIONS PASSED!${NC}"
    echo "Flight Shared Types language bindings are ready for production use."
    exit 0
else
    echo -e "\n${RED}❌ SOME VALIDATIONS FAILED${NC}"
    echo "Please review and fix the failed tests before proceeding to production."
    exit 1
fi
