# Flight HAL Teams

This directory contains all specialized teams working on the Flight Hardware Abstraction Layer.

## Team Organization

Teams are organized by their specialization:

### Interface Teams (Layer 1)
Foundation teams that define the HAL contracts:
- `flight-hal-interfaces` - Core abstractions
- `flight-hal-*-interface` - Subsystem interfaces

### Platform Implementation Teams (Layer 2)
Teams implementing specific platform features:
- `flight-hal-{platform}-memory` - Memory management
- `flight-hal-{platform}-{subsystem}` - Platform subsystems (gpu, audio, input, etc.)

### Platform Integration Teams (Layer 3)
Teams that integrate all subsystems:
- `flight-hal-{platform}-integration` - Complete platform implementations

### Testing Teams (Layer 4)
Teams ensuring quality across platforms:
- `flight-hal-test-*` - Various test specializations

### Tooling Teams (Layer 4)
Teams providing development support:
- `flight-hal-*-tools` - Build, development, and profiling tools

## Platform Coverage

### Tier 1: Retro Consoles
- **PSX** (PlayStation 1) - 2MB RAM, no threading
- **Dreamcast** - 16MB RAM, PowerVR2
- **PSP** - 32/64MB RAM, MIPS architecture

### Tier 2: Modern Platforms
- **macOS** - Metal, Core Audio
- **Windows** - D3D12, WASAPI
- **Linux** - Vulkan, ALSA
- **Web** - WebGL2, Web Audio

### Tier 3: Mobile/Handheld
- **iOS** - Metal, Core Audio
- **Android** - Vulkan, AAudio
- **Steam Deck** - Vulkan, Pipewire

## Dependencies

Each team has a `dependencies.yaml` file that specifies:
- Team information
- Project dependencies
- Required features from dependencies

## Building

Each team follows the standard CMake/CPM structure:
```bash
cd teams/{team-name}
cmake -B build
cmake --build build
```

## Documentation

- See `docs/hal-team-dependency-tree.md` for the complete dependency graph
- Each team's README contains specific implementation details
- The `proompts/` folder in each team contains team-specific documentation

## Nx Integration

All teams are Nx-compatible with `project.json` files for:
- Build targets
- Test targets
- Dependency management

## Total Team Count: 72

This ultra-specialized structure enables:
- Maximum parallelization
- Deep domain expertise
- Clear ownership boundaries
- Efficient cross-platform development
