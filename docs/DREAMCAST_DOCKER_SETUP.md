# Dreamcast Development with Docker on Apple Silicon

This document describes the complete setup for Dreamcast development using Docker containers on Apple Silicon Macs.

## Overview

After encountering compilation issues with building the Dreamcast toolchain natively on Apple Silicon (due to libcc1 and C++ STL conflicts), we successfully implemented a Docker-based development environment using KallistiOS.

## Prerequisites

- macOS with Apple Silicon (M1/M2/M3)
- Docker Desktop for Mac installed and running
- Basic knowledge of command line tools

## Setup

### 1. Docker Container

We use the `einsteinx2/dcdev-kos-toolchain:gcc-9` container which provides:
- GCC 9.3.0 cross-compiler for SuperH (sh-elf)
- Complete KallistiOS SDK
- All necessary development tools

### 2. Verification

Test that the container works:

```bash
docker run --rm einsteinx2/dcdev-kos-toolchain:gcc-9 sh-elf-gcc --version
```

Expected output:
```
sh-elf-gcc (GCC) 9.3.0
```

## Development Workflow

### Build Scripts

Two convenient scripts are provided in `dc/kos/utils/dc-chain/`:

#### build_dreamcast.sh

Main build script with commands:
- `./build_dreamcast.sh build` - Build current project
- `./build_dreamcast.sh clean` - Clean build artifacts  
- `./build_dreamcast.sh shell` - Interactive container shell
- `./build_dreamcast.sh test` - Build test project

#### init_dreamcast_project.sh

Project initialization script:
```bash
./init_dreamcast_project.sh my_project_name
```

Creates a new project with:
- `main.c` - Basic KallistiOS program
- `Makefile` - Build configuration
- `README.md` - Project documentation

### Manual Building

For direct Docker usage:

```bash
# Build project
docker run --rm -v "$(pwd):/workspace" -w /workspace \
  einsteinx2/dcdev-kos-toolchain:gcc-9 \
  bash -c "source /opt/toolchains/dc/kos/environ.sh && make"

# Interactive development
docker run --rm -it -v "$(pwd):/workspace" -w /workspace \
  einsteinx2/dcdev-kos-toolchain:gcc-9 \
  bash -c "source /opt/toolchains/dc/kos/environ.sh && bash"
```

## Project Structure

### Basic Project Layout

```
my_dreamcast_project/
├── main.c          # Main source file
├── Makefile        # Build configuration
└── README.md       # Documentation
```

### Sample main.c

```c
#include <kos.h>

int main(int argc, char **argv) {
    printf("Hello from Dreamcast!\n");
    printf("KallistiOS is working!\n");
    return 0;
}
```

### Sample Makefile

```makefile
TARGET = main
OBJS = main.o

all: $(TARGET).elf

include $(KOS_BASE)/Makefile.rules

clean:
	-rm -f $(TARGET).elf $(OBJS)

$(TARGET).elf: $(OBJS)
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $@ $(KOS_START) $^ $(OBJEXTRA) $(KOS_LIBS)

.PHONY: clean all
```

## Output Files

Successful compilation produces:
- `*.elf` files - Dreamcast executables (typically 2-3MB)
- Targeting Renesas SH (SuperH) architecture
- Statically linked with KallistiOS libraries

## Deployment Options

The generated `.elf` files can run on:

1. **Real Dreamcast Hardware**
   - Via dc-tool over network/serial
   - Burned to CD-R as homebrew

2. **Emulators**
   - Flycast (recommended)
   - redream
   - nullDC

3. **Development Boards**
   - HKT-0120 Dreamcast devkit
   - NAOMI arcade hardware

## Integration with Flight Project

For the Flight project specifically:

### Environment Variables

The container environment includes:
- `KOS_BASE=/opt/toolchains/dc/kos`
- `KOS_CC_BASE=/opt/toolchains/dc/sh-elf`
- `KOS_CC_PREFIX=sh-elf`

### Build Integration

Add to your Flight project's build system:

```bash
# In your main build script
if [ "$TARGET_PLATFORM" = "dreamcast" ]; then
    echo "Building for Dreamcast..."
    docker run --rm -v "$(pwd):/workspace" -w /workspace \
        einsteinx2/dcdev-kos-toolchain:gcc-9 \
        bash -c "source /opt/toolchains/dc/kos/environ.sh && make -f Makefile.dreamcast"
fi
```

## Troubleshooting

### Common Issues

1. **TTY Errors**: Use `docker run --rm` instead of `docker run --rm -it` for non-interactive builds

2. **File Permissions**: Ensure scripts are executable:
   ```bash
   chmod +x build_dreamcast.sh init_dreamcast_project.sh
   ```

3. **Container Updates**: Periodically pull latest container:
   ```bash
   docker pull einsteinx2/dcdev-kos-toolchain:gcc-9
   ```

### Build Failures

- Check Makefile syntax (tabs vs spaces)
- Verify source files don't have shell artifacts (EOF markers)
- Ensure KallistiOS headers are included correctly

## Performance Notes

- Docker emulation on Apple Silicon adds ~20-30% build time overhead
- Consider using Docker BuildKit for improved performance
- Container startup time is ~2-3 seconds per build

## Alternative Containers

If the primary container becomes unavailable:
- `nold360/kallistios-sdk` (may have issues)
- Build custom container from KallistiOS source

## Conclusion

This Docker-based approach provides a reliable, reproducible Dreamcast development environment on Apple Silicon, avoiding the native toolchain compilation issues while maintaining full KallistiOS functionality.