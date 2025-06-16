#!/bin/bash
set -euo pipefail

# Flight-Core C++17 Bindings Generator and Validator
# Generates and validates C++17 bindings for Flight Shared Types

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
CPP17_DIR="$PROJECT_ROOT/bindings/cpp17"

echo "🚀 Flight-Core C++17 Bindings Generator"
echo "========================================"

# Configuration
PLATFORM="${FLIGHT_TARGET_PLATFORM:-desktop}"
BUILD_TYPE="${CMAKE_BUILD_TYPE:-Release}"
BUILD_EXAMPLES="${FLIGHT_BUILD_CPP17_EXAMPLES:-ON}"
BUILD_TESTS="${FLIGHT_BUILD_CPP17_TESTS:-OFF}"

echo "Configuration:"
echo "  Platform: $PLATFORM"
echo "  Build Type: $BUILD_TYPE"
echo "  Examples: $BUILD_EXAMPLES"
echo "  Tests: $BUILD_TESTS"
echo ""

# Check prerequisites
check_prerequisites() {
    echo "📋 Checking prerequisites..."
    
    # Check CMake
    if ! command -v cmake &> /dev/null; then
        echo "❌ CMake not found. Please install CMake 3.17+"
        exit 1
    fi
    
    CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
    echo "  ✅ CMake $CMAKE_VERSION found"
    
    # Check C++ compiler
    if command -v clang++ &> /dev/null; then
        CXX_COMPILER="clang++"
        CXX_VERSION=$(clang++ --version | head -n1)
    elif command -v g++ &> /dev/null; then
        CXX_COMPILER="g++"
        CXX_VERSION=$(g++ --version | head -n1)
    else
        echo "❌ No C++17 compiler found (clang++ or g++)"
        exit 1
    fi
    
    echo "  ✅ $CXX_COMPILER found: $CXX_VERSION"
    echo ""
}

# Clean previous builds
clean_build() {
    echo "🧹 Cleaning previous builds..."
    rm -rf "$CPP17_DIR/build"
    echo "  ✅ Build directory cleaned"
    echo ""
}

# Configure build
configure_build() {
    echo "⚙️  Configuring build..."
    
    mkdir -p "$CPP17_DIR/build"
    cd "$CPP17_DIR/build"
    
    # Platform-specific configuration
    CMAKE_FLAGS=()
    
    case "$PLATFORM" in
        dreamcast)
            CMAKE_FLAGS+=(
                "-DFLIGHT_TARGET_PLATFORM=dreamcast"
                "-DCMAKE_BUILD_TYPE=MinSizeRel"  # Optimize for size
            )
            echo "  🎮 Configuring for Dreamcast (16MB, size-optimized)"
            ;;
        psp)
            CMAKE_FLAGS+=(
                "-DFLIGHT_TARGET_PLATFORM=psp"
                "-DCMAKE_BUILD_TYPE=MinSizeRel"
            )
            echo "  🎮 Configuring for PlayStation Portable (32MB)"
            ;;
        vita)
            CMAKE_FLAGS+=(
                "-DFLIGHT_TARGET_PLATFORM=vita"
                "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
            )
            echo "  🎮 Configuring for PlayStation Vita (512MB)"
            ;;
        v6r)
            CMAKE_FLAGS+=(
                "-DFLIGHT_TARGET_PLATFORM=v6r"
                "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
            )
            echo "  ☁️  Configuring for V6R Cloud (up to 2GB+)"
            ;;
        *)
            CMAKE_FLAGS+=(
                "-DFLIGHT_TARGET_PLATFORM=desktop"
                "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
            )
            echo "  🖥️  Configuring for Desktop development"
            ;;
    esac
    
    CMAKE_FLAGS+=(
        "-DFLIGHT_BUILD_CPP17_EXAMPLES=$BUILD_EXAMPLES"
        "-DFLIGHT_BUILD_CPP17_TESTS=$BUILD_TESTS"
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    )
    
    echo "  Running: cmake .. ${CMAKE_FLAGS[*]}"
    cmake .. "${CMAKE_FLAGS[@]}"
    
    echo "  ✅ Build configured successfully"
    echo ""
}

# Build the library
build_library() {
    echo "🔨 Building Flight-Core C++17 bindings..."
    
    cd "$CPP17_DIR/build"
    
    # Determine number of parallel jobs
    JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    
    echo "  Building with $JOBS parallel jobs..."
    make -j"$JOBS"
    
    echo "  ✅ Build completed successfully"
    echo ""
}

# Validate the build
validate_build() {
    echo "🔍 Validating build artifacts..."
    
    cd "$CPP17_DIR/build"
    
    # Check library files
    if [[ -f "libflight_shared_types_cpp17.so" || -f "libflight_shared_types_cpp17.dylib" ]]; then
        echo "  ✅ Shared library built successfully"
    else
        echo "  ❌ Shared library not found"
        return 1
    fi
    
    if [[ -f "libflight_shared_types_cpp17_static.a" ]]; then
        echo "  ✅ Static library built successfully"
    else
        echo "  ❌ Static library not found"
        return 1
    fi
    
    # Check examples if enabled
    if [[ "$BUILD_EXAMPLES" == "ON" ]]; then
        if [[ -f "examples/flight_integration_example" ]]; then
            echo "  ✅ Integration example built successfully"
        else
            echo "  ❌ Integration example not found"
            return 1
        fi
    fi
    
    echo "  ✅ All build artifacts validated"
    echo ""
}

# Run integration example
run_example() {
    if [[ "$BUILD_EXAMPLES" != "ON" ]]; then
        echo "⏭️  Skipping example (BUILD_EXAMPLES=OFF)"
        return 0
    fi
    
    echo "🧪 Running integration example..."
    
    cd "$CPP17_DIR/build"
    
    if [[ -f "examples/flight_integration_example" ]]; then
        echo "  Executing: ./examples/flight_integration_example"
        echo "  ----------------------------------------"
        ./examples/flight_integration_example
        echo "  ----------------------------------------"
        echo "  ✅ Integration example completed successfully"
    else
        echo "  ❌ Integration example not found"
        return 1
    fi
    
    echo ""
}

# Generate documentation
generate_docs() {
    echo "📚 Generating documentation..."
    
    # Check if header file is properly documented
    HEADER_FILE="$CPP17_DIR/flight/flight_shared_types.hpp"
    if [[ -f "$HEADER_FILE" ]]; then
        DOC_COMMENTS=$(grep -c "^ *\*\|^ */\*\|^ *///" "$HEADER_FILE" || echo 0)
        echo "  ✅ Header file contains $DOC_COMMENTS documentation lines"
    fi
    
    # Check if README exists
    README_FILE="$CPP17_DIR/README.md"
    if [[ -f "$README_FILE" ]]; then
        README_SIZE=$(wc -l < "$README_FILE")
        echo "  ✅ README.md exists ($README_SIZE lines)"
    fi
    
    echo "  ✅ Documentation generation completed"
    echo ""
}

# Display build summary
show_summary() {
    echo "📊 Build Summary"
    echo "================"
    echo "Platform: $PLATFORM"
    echo "Build Type: $BUILD_TYPE"
    echo "Build Directory: $CPP17_DIR/build"
    echo ""
    
    cd "$CPP17_DIR/build"
    
    echo "Generated Files:"
    find . -name "*.so" -o -name "*.dylib" -o -name "*.a" -o -name "*example*" | sort | while read -r file; do
        SIZE=$(du -h "$file" | cut -f1)
        echo "  📁 $file ($SIZE)"
    done
    echo ""
    
    echo "Integration Points:"
    echo "  🔗 Include: #include \"flight_shared_types.hpp\""
    echo "  🔗 Link: -lflight_shared_types_cpp17"
    echo "  🔗 Namespace: flight::shared_types"
    echo ""
    
    echo "Next Steps:"
    echo "  📖 Read: $CPP17_DIR/README.md"
    echo "  🧪 Test: make test (if tests enabled)"
    echo "  🚀 Install: make install"
    echo "  📦 Package: cpack (if configured)"
}

# Main execution
main() {
    echo "Starting Flight-Core C++17 bindings generation..."
    echo ""
    
    check_prerequisites
    clean_build
    configure_build
    build_library
    validate_build
    run_example
    generate_docs
    show_summary
    
    echo "🎉 Flight-Core C++17 bindings generated successfully!"
    echo ""
    echo "The modern C++17 bindings are now ready for integration with Flight-Core."
    echo "They provide zero-cost abstractions for multi-platform development"
    echo "from Dreamcast (16MB) to V6R cloud (2GB+) environments."
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --platform)
            PLATFORM="$2"
            shift 2
            ;;
        --build-type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        --examples)
            BUILD_EXAMPLES="$2"
            shift 2
            ;;
        --tests)
            BUILD_TESTS="$2"
            shift 2
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  --platform <platform>   Target platform (dreamcast|psp|vita|v6r|desktop)"
            echo "  --build-type <type>     Build type (Debug|Release|MinSizeRel)"
            echo "  --examples <ON|OFF>     Build examples (default: ON)"
            echo "  --tests <ON|OFF>        Build tests (default: OFF)"
            echo "  --help                  Show this help"
            echo ""
            echo "Examples:"
            echo "  $0 --platform dreamcast --build-type MinSizeRel"
            echo "  $0 --platform v6r --examples ON --tests ON"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Execute main function
main
