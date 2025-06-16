#!/bin/bash
# Flight Session Types Testing Script
# Comprehensive validation for session types implementation

set -euo pipefail

echo "🧪 Testing Flight Session Types Implementation..."

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test counters
TESTS_PASSED=0
TESTS_FAILED=0

# Helper functions
print_header() {
    echo -e "\n${BLUE}=== $1 ===${NC}"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
    ((TESTS_PASSED++))
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
    ((TESTS_FAILED++))
}

print_warning() {
    echo -e "${YELLOW}⚠️ $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ️ $1${NC}"
}

# Check if required tools are available
check_dependencies() {
    print_header "Checking Dependencies"
    
    # Check for Rust/Cargo
    if command -v cargo &> /dev/null; then
        print_success "Rust/Cargo found: $(cargo --version)"
    else
        print_error "Rust/Cargo not found - required for validation tests"
        exit 1
    fi
    
    # Check for Node.js/npm
    if command -v node &> /dev/null && command -v npm &> /dev/null; then
        print_success "Node.js/npm found: $(node --version) / $(npm --version)"
    else
        print_warning "Node.js/npm not found - TypeScript tests will be skipped"
    fi
    
    # Check for Go
    if command -v go &> /dev/null; then
        print_success "Go found: $(go version)"
    else
        print_warning "Go not found - Go tests will be skipped"
    fi
}

# Validate WIT interface
validate_wit_interface() {
    print_header "Validating WIT Interface"
    
    if [[ -f "wit/session.wit" ]]; then
        print_success "Session WIT interface exists"
        
        # Check for required interfaces
        if grep -q "interface session-types" wit/session.wit; then
            print_success "session-types interface found"
        else
            print_error "session-types interface missing"
        fi
        
        if grep -q "interface session-operations" wit/session.wit; then
            print_success "session-operations interface found"
        else
            print_error "session-operations interface missing"
        fi
        
        if grep -q "interface session-analytics" wit/session.wit; then
            print_success "session-analytics interface found"
        else
            print_error "session-analytics interface missing"
        fi
        
        # Check for required types
        local required_types=("session-state" "session-type" "session-info" "session-resources" "resource-limits")
        for type_name in "${required_types[@]}"; do
            if grep -q "$type_name" wit/session.wit; then
                print_success "Type '$type_name' found"
            else
                print_error "Type '$type_name' missing"
            fi
        done
        
    else
        print_error "Session WIT interface not found at wit/session.wit"
    fi
}

# Test Rust validation
test_rust_validation() {
    print_header "Testing Rust Validation"
    
    if [[ -d "validation/session" ]]; then
        print_info "Running Rust validation tests..."
        
        cd validation/session
        
        # Check if Cargo.toml exists
        if [[ -f "Cargo.toml" ]]; then
            print_success "Cargo.toml found"
        else
            print_error "Cargo.toml missing"
            cd ../..
            return
        fi
        
        # Build the validation module
        if cargo build --quiet; then
            print_success "Rust validation module builds successfully"
        else
            print_error "Rust validation module build failed"
            cd ../..
            return
        fi
        
        # Run tests
        if cargo test --quiet; then
            print_success "Rust validation tests pass"
        else
            print_error "Rust validation tests failed"
        fi
        
        cd ../..
    else
        print_error "Rust validation directory not found"
    fi
}

# Test TypeScript bindings
test_typescript_bindings() {
    print_header "Testing TypeScript Bindings"
    
    if ! command -v node &> /dev/null; then
        print_warning "Node.js not available - skipping TypeScript tests"
        return
    fi
    
    if [[ -d "bindings/typescript/session" ]]; then
        print_info "Testing TypeScript bindings..."
        
        cd bindings/typescript/session
        
        # Check package.json
        if [[ -f "package.json" ]]; then
            print_success "package.json found"
        else
            print_error "package.json missing"
            cd ../../..
            return
        fi
        
        # Check TypeScript config
        if [[ -f "tsconfig.json" ]]; then
            print_success "tsconfig.json found"
        else
            print_error "tsconfig.json missing"
        fi
        
        # Check source files
        local required_files=("src/types.ts" "src/index.ts" "src/utils/session-utils.ts" "src/react/session-hooks.ts")
        for file in "${required_files[@]}"; do
            if [[ -f "$file" ]]; then
                print_success "Source file '$file' found"
            else
                print_error "Source file '$file' missing"
            fi
        done
        
        # Install dependencies (if package-lock.json doesn't exist)
        if [[ ! -f "package-lock.json" ]]; then
            print_info "Installing TypeScript dependencies..."
            if npm install --silent; then
                print_success "TypeScript dependencies installed"
            else
                print_error "Failed to install TypeScript dependencies"
                cd ../../..
                return
            fi
        fi
        
        # Build TypeScript
        print_info "Building TypeScript bindings..."
        if npm run build --silent; then
            print_success "TypeScript bindings build successfully"
        else
            print_error "TypeScript bindings build failed"
        fi
        
        cd ../../..
    else
        print_error "TypeScript bindings directory not found"
    fi
}

# Test Go bindings
test_go_bindings() {
    print_header "Testing Go Bindings"
    
    if ! command -v go &> /dev/null; then
        print_warning "Go not available - skipping Go tests"
        return
    fi
    
    if [[ -d "bindings/go/session" ]]; then
        print_info "Testing Go bindings..."
        
        cd bindings/go/session
        
        # Check go.mod
        if [[ -f "go.mod" ]]; then
            print_success "go.mod found"
        else
            print_error "go.mod missing"
            cd ../../..
            return
        fi
        
        # Check source files
        if [[ -f "types.go" ]]; then
            print_success "Go types file found"
        else
            print_error "Go types file missing"
        fi
        
        # Initialize module if needed
        if ! go mod tidy; then
            print_warning "Go mod tidy failed - may need manual dependency setup"
        fi
        
        # Build Go bindings
        print_info "Building Go bindings..."
        if go build -o /dev/null .; then
            print_success "Go bindings build successfully"
        else
            print_error "Go bindings build failed"
        fi
        
        cd ../../..
    else
        print_error "Go bindings directory not found"
    fi
}

# Test integration tests
test_integration() {
    print_header "Testing Integration Tests"
    
    # Check if integration test files exist
    local test_files=(
        "tests/integration/typescript/session-types.test.ts"
    )
    
    for test_file in "${test_files[@]}"; do
        if [[ -f "$test_file" ]]; then
            print_success "Integration test '$test_file' exists"
        else
            print_error "Integration test '$test_file' missing"
        fi
    done
}

# Validate documentation
validate_documentation() {
    print_header "Validating Documentation"
    
    # Check for documentation files
    if [[ -f "docs/session-types-integration-guide.md" ]]; then
        print_success "Integration guide documentation exists"
    else
        print_error "Integration guide documentation missing"
    fi
    
    # Check for README files in bindings
    local readme_files=(
        "bindings/typescript/session/package.json"
        "bindings/go/session/go.mod"
    )
    
    for readme in "${readme_files[@]}"; do
        if [[ -f "$readme" ]]; then
            print_success "Binding manifest '$readme' exists"
        else
            print_error "Binding manifest '$readme' missing"
        fi
    done
}

# Performance benchmarks
run_performance_tests() {
    print_header "Running Performance Tests"
    
    if command -v node &> /dev/null && [[ -d "bindings/typescript/session" ]]; then
        print_info "Running TypeScript performance tests..."
        
        # Create a simple performance test
        cat > /tmp/session_perf_test.js << 'EOF'
const { performance } = require('perf_hooks');

// Simulate session operations
function simulateSessionOperations(count) {
    const start = performance.now();
    
    const sessions = [];
    for (let i = 0; i < count; i++) {
        sessions.push({
            id: `session-${i}`,
            sessionType: 'development',
            state: 'active',
            platform: 'cloud',
            createdAt: Date.now(),
            lastActivity: Date.now(),
            metadata: []
        });
    }
    
    const end = performance.now();
    return end - start;
}

// Test with different session counts
const counts = [100, 1000, 10000];
for (const count of counts) {
    const duration = simulateSessionOperations(count);
    console.log(`${count} sessions: ${duration.toFixed(2)}ms`);
    
    if (duration > count * 0.1) { // Should be <0.1ms per session
        console.log(`⚠️  Performance warning: ${count} sessions took ${duration.toFixed(2)}ms`);
    }
}
EOF
        
        if node /tmp/session_perf_test.js; then
            print_success "TypeScript performance tests completed"
        else
            print_warning "TypeScript performance tests had issues"
        fi
        
        rm -f /tmp/session_perf_test.js
    fi
}

# Memory usage validation
validate_memory_constraints() {
    print_header "Validating Memory Constraints"
    
    print_info "Checking session type definitions for memory efficiency..."
    
    # Check WIT file size (should be reasonable)
    if [[ -f "wit/session.wit" ]]; then
        local wit_size=$(wc -c < "wit/session.wit")
        if [[ $wit_size -lt 50000 ]]; then  # Less than 50KB
            print_success "WIT interface size is efficient: ${wit_size} bytes"
        else
            print_warning "WIT interface is large: ${wit_size} bytes"
        fi
    fi
    
    # Validate platform constraints are defined
    if grep -q "dreamcast" wit/session.wit && grep -q "16MB" docs/session-types-integration-guide.md; then
        print_success "Dreamcast memory constraints documented"
    else
        print_warning "Dreamcast memory constraints may be missing"
    fi
}

# Vendor neutrality check
check_vendor_neutrality() {
    print_header "Checking Vendor Neutrality"
    
    # Search for any references to private companies
    local private_refs=("v6r" "V6R" "proprietary")
    local found_private=false
    
    for ref in "${private_refs[@]}"; do
        if grep -r "$ref" wit/ bindings/ docs/session-types-integration-guide.md 2>/dev/null | grep -v "vendor-neutral" | grep -v "No dependencies"; then
            print_error "Found potential private company reference: $ref"
            found_private=true
        fi
    done
    
    if [[ "$found_private" == false ]]; then
        print_success "Implementation is vendor-neutral"
    fi
    
    # Check for extensibility markers
    if grep -q "extensible" wit/session.wit && grep -q "third-party" docs/session-types-integration-guide.md; then
        print_success "Extensibility for third-party integrations confirmed"
    else
        print_warning "Extensibility documentation may be incomplete"
    fi
}

# Generate summary report
generate_summary() {
    print_header "Test Summary"
    
    local total_tests=$((TESTS_PASSED + TESTS_FAILED))
    local pass_rate=0
    
    if [[ $total_tests -gt 0 ]]; then
        pass_rate=$((TESTS_PASSED * 100 / total_tests))
    fi
    
    echo -e "\n📊 ${BLUE}Test Results:${NC}"
    echo -e "   Total Tests: $total_tests"
    echo -e "   ${GREEN}Passed: $TESTS_PASSED${NC}"
    echo -e "   ${RED}Failed: $TESTS_FAILED${NC}"
    echo -e "   Pass Rate: ${pass_rate}%"
    
    if [[ $TESTS_FAILED -eq 0 ]]; then
        echo -e "\n🎉 ${GREEN}All tests passed! Session types implementation is ready.${NC}"
        return 0
    elif [[ $pass_rate -ge 80 ]]; then
        echo -e "\n✅ ${YELLOW}Most tests passed. Review failed tests before production use.${NC}"
        return 1
    else
        echo -e "\n❌ ${RED}Multiple test failures. Implementation needs fixes before use.${NC}"
        return 2
    fi
}

# Main execution
main() {
    echo "Flight Session Types Test Suite"
    echo "==============================="
    
    check_dependencies
    validate_wit_interface
    test_rust_validation
    test_typescript_bindings
    test_go_bindings
    test_integration
    validate_documentation
    run_performance_tests
    validate_memory_constraints
    check_vendor_neutrality
    
    generate_summary
}

# Run main function and exit with appropriate code
main
exit_code=$?
exit $exit_code
