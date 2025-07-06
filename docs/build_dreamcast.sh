#!/bin/bash
# Convenient build script for Dreamcast development using Docker

set -e

CONTAINER="einsteinx2/dcdev-kos-toolchain:gcc-9"
WORKSPACE="/workspace"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

usage() {
    echo "Usage: $0 [command] [options]"
    echo ""
    echo "Commands:"
    echo "  build           Build the current project"
    echo "  clean           Clean build artifacts"
    echo "  shell           Open interactive shell in container"
    echo "  test            Build and run basic tests"
    echo ""
    echo "Options:"
    echo "  -h, --help      Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 build"
    echo "  $0 clean"
    echo "  $0 shell"
}

log() {
    echo -e "${GREEN}[DC-BUILD]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[DC-BUILD]${NC} $1"
}

error() {
    echo -e "${RED}[DC-BUILD]${NC} $1"
}

check_docker() {
    if ! command -v docker &> /dev/null; then
        error "Docker is not installed or not in PATH"
        exit 1
    fi
    
    if ! docker info &> /dev/null; then
        error "Docker daemon is not running"
        exit 1
    fi
}

pull_container() {
    log "Ensuring latest container is available..."
    docker pull "$CONTAINER" || {
        error "Failed to pull container $CONTAINER"
        exit 1
    }
}

run_in_container() {
    local cmd="$1"
    log "Running: $cmd"
    docker run --rm -v "$(pwd):$WORKSPACE" -w "$WORKSPACE" "$CONTAINER" bash -c "source /opt/toolchains/dc/kos/environ.sh && $cmd"
}

run_interactive() {
    log "Starting interactive shell in Dreamcast development container..."
    docker run --rm -it -v "$(pwd):$WORKSPACE" -w "$WORKSPACE" "$CONTAINER" bash -c "source /opt/toolchains/dc/kos/environ.sh && bash"
}

build_project() {
    log "Building Dreamcast project..."
    if [ ! -f "Makefile" ]; then
        error "No Makefile found in current directory"
        exit 1
    fi
    
    run_in_container "make"
    
    # Check for built ELF files
    if ls *.elf 1> /dev/null 2>&1; then
        log "Build successful! Generated files:"
        ls -la *.elf
    else
        error "Build completed but no .elf files found"
        exit 1
    fi
}

clean_project() {
    log "Cleaning build artifacts..."
    if [ -f "Makefile" ]; then
        run_in_container "make clean"
    fi
    
    # Also clean any stray object files
    rm -f *.o *.elf
    log "Clean completed"
}

test_build() {
    log "Running build test..."
    
    # Create a simple test if no source files exist
    if [ ! -f "*.c" ] && [ ! -f "test_dreamcast_hello.c" ]; then
        warn "No source files found, creating test program..."
        cat > test_hello.c << 'EOF'
#include <kos.h>

int main(int argc, char **argv) {
    printf("Hello from Dreamcast!\n");
    return 0;
}
EOF
        
        cat > Makefile << 'EOF'
TARGET = test_hello
OBJS = test_hello.o

all: $(TARGET).elf

include $(KOS_BASE)/Makefile.rules

clean:
	-rm -f $(TARGET).elf $(OBJS)

$(TARGET).elf: $(OBJS)
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $@ $(KOS_START) $^ $(OBJEXTRA) $(KOS_LIBS)

.PHONY: clean
EOF
    fi
    
    build_project
    log "Test build completed successfully"
}

# Main script logic
case "${1:-}" in
    build)
        check_docker
        build_project
        ;;
    clean)
        clean_project
        ;;
    shell)
        check_docker
        pull_container
        run_interactive
        ;;
    test)
        check_docker
        test_build
        ;;
    -h|--help|help)
        usage
        ;;
    "")
        log "No command specified, running build..."
        check_docker
        build_project
        ;;
    *)
        error "Unknown command: $1"
        usage
        exit 1
        ;;
esac