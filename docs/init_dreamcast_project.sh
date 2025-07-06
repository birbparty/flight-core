#!/bin/bash
# Initialize a new Dreamcast project with basic template files

set -e

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log() {
    echo -e "${GREEN}[DC-INIT]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[DC-INIT]${NC} $1"
}

PROJECT_NAME="${1:-dreamcast_project}"

if [ -d "$PROJECT_NAME" ]; then
    warn "Directory $PROJECT_NAME already exists. Contents may be overwritten."
    read -p "Continue? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

mkdir -p "$PROJECT_NAME"
cd "$PROJECT_NAME"

log "Creating basic Dreamcast project: $PROJECT_NAME"

# Create main.c
cat > main.c << 'EOF'
#include <kos.h>

int main(int argc, char **argv) {
    printf("Hello from %s!\n", "Dreamcast");
    printf("KallistiOS is running successfully.\n");
    
    // Keep the program running for a few seconds
    thd_sleep(3000);
    
    return 0;
}
EOF

# Create Makefile
cat > Makefile << 'EOF'
TARGET = main
OBJS = main.o

all: $(TARGET).elf

include $(KOS_BASE)/Makefile.rules

clean:
	-rm -f $(TARGET).elf $(OBJS)

$(TARGET).elf: $(OBJS)
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $@ $(KOS_START) $^ $(OBJEXTRA) $(KOS_LIBS)

.PHONY: clean all
EOF

# Create README
cat > README.md << EOF
# $PROJECT_NAME

A Dreamcast project using KallistiOS.

## Building

Use the provided build script:

\`\`\`bash
../build_dreamcast.sh build
\`\`\`

Or manually with Docker:

\`\`\`bash
docker run --rm -v "\$(pwd):/workspace" -w /workspace einsteinx2/dcdev-kos-toolchain:gcc-9 bash -c "source /opt/toolchains/dc/kos/environ.sh && make"
\`\`\`

## Output

The build process creates \`$PROJECT_NAME.elf\` which can be run on:
- Dreamcast hardware (via dc-tool)
- Dreamcast emulators (flycast, redream)
- Development boards

## Cleaning

\`\`\`bash
../build_dreamcast.sh clean
\`\`\`
EOF

log "Project created successfully!"
log "Files created:"
echo "  main.c      - Main source file"
echo "  Makefile    - Build configuration"
echo "  README.md   - Project documentation"
echo ""
log "To build: cd $PROJECT_NAME && ../build_dreamcast.sh build"