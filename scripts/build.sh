#!/bin/bash

# Flight Build Script

set -e

# Default values
BUILD_TYPE="Debug"
BUILD_DIR="build"
GENERATOR=""
CLEAN=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -r|--release)
            BUILD_TYPE="Release"
            shift
            ;;
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -c|--clean)
            CLEAN=true
            shift
            ;;
        -g|--generator)
            GENERATOR="-G $2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  -r, --release     Build in Release mode"
            echo "  -d, --debug       Build in Debug mode (default)"
            echo "  -c, --clean       Clean build directory before building"
            echo "  -g, --generator   Specify CMake generator (e.g., 'Ninja', 'Unix Makefiles')"
            echo "  -h, --help        Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Clean if requested
if [ "$CLEAN" = true ]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"

# Configure
echo "Configuring with CMake (Build Type: $BUILD_TYPE)..."
cmake -S . -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    $GENERATOR

# Build
echo "Building..."
cmake --build "$BUILD_DIR" --parallel

echo "Build complete!"
echo "Binaries are in: $BUILD_DIR"
