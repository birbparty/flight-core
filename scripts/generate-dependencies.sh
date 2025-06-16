#!/bin/bash

# Generate dependencies.yaml for each team

# Helper function to create dependencies.yaml
create_dependencies() {
    local team_path=$1
    local content=$2
    
    echo "Creating dependencies.yaml for $team_path"
    cat > "$team_path/dependencies.yaml" << EOF
$content
EOF
}

# Layer 1: Interface Teams (No dependencies)
echo "=== Creating Interface Team Dependencies ==="

create_dependencies "teams/flight-hal-interfaces" "team_info:
  name: \"HAL Core Interfaces Team\"
  purpose: \"Define core HAL abstractions and platform detection\"
  
dependencies: []  # Foundation layer - no dependencies"

create_dependencies "teams/flight-hal-memory-interface" "team_info:
  name: \"HAL Memory Interface Team\"
  purpose: \"Define memory allocation and management interfaces\"
  
dependencies: 
  - project: \"flight-hal-interfaces\"
    team: \"HAL Core Interfaces Team\"
    features:
      - \"Base types and error handling\"
      - \"Platform capability system\""

create_dependencies "teams/flight-hal-time-interface" "team_info:
  name: \"HAL Time Interface Team\"
  purpose: \"Define timing, clock, and synchronization interfaces\"
  
dependencies:
  - project: \"flight-hal-interfaces\"
    team: \"HAL Core Interfaces Team\"
    features:
      - \"Base types and error handling\""

create_dependencies "teams/flight-hal-io-interface" "team_info:
  name: \"HAL I/O Interface Team\"
  purpose: \"Define file, stream, and storage interfaces\"
  
dependencies:
  - project: \"flight-hal-interfaces\"
    team: \"HAL Core Interfaces Team\"
    features:
      - \"Base types and error handling\"
      - \"Result type for I/O operations\""

create_dependencies "teams/flight-hal-threading-interface" "team_info:
  name: \"HAL Threading Interface Team\"
  purpose: \"Define threading and synchronization interfaces\"
  
dependencies:
  - project: \"flight-hal-interfaces\"
    team: \"HAL Core Interfaces Team\"
    features:
      - \"Base types and error handling\"
      - \"Platform capability detection\""

create_dependencies "teams/flight-hal-graphics-interface" "team_info:
  name: \"HAL Graphics Interface Team\"
  purpose: \"Define graphics context and rendering interfaces\"
  
dependencies:
  - project: \"flight-hal-interfaces\"
    team: \"HAL Core Interfaces Team\"
    features:
      - \"Base types and error handling\"
  - project: \"flight-hal-memory-interface\"
    team: \"HAL Memory Interface Team\"
    features:
      - \"Memory allocation for buffers\""

create_dependencies "teams/flight-hal-audio-interface" "team_info:
  name: \"HAL Audio Interface Team\"
  purpose: \"Define audio device and buffer interfaces\"
  
dependencies:
  - project: \"flight-hal-interfaces\"
    team: \"HAL Core Interfaces Team\"
    features:
      - \"Base types and error handling\"
  - project: \"flight-hal-memory-interface\"
    team: \"HAL Memory Interface Team\"
    features:
      - \"Memory allocation for audio buffers\""

create_dependencies "teams/flight-hal-input-interface" "team_info:
  name: \"HAL Input Interface Team\"
  purpose: \"Define input device and event interfaces\"
  
dependencies:
  - project: \"flight-hal-interfaces\"
    team: \"HAL Core Interfaces Team\"
    features:
      - \"Base types and error handling\"
      - \"Event system types\""

# Layer 2: Platform Memory Teams
echo "=== Creating Platform Memory Team Dependencies ==="

create_dependencies "teams/flight-hal-psx-memory" "team_info:
  name: \"PSX Memory Team\"
  purpose: \"Implement PSX memory management (2MB RAM, VRAM, SPU RAM)\"
  
dependencies:
  - project: \"flight-hal-memory-interface\"
    team: \"HAL Memory Interface Team\"
    features:
      - \"Memory allocator interface\"
      - \"Memory region definitions\"
  - project: \"flight-hal-interfaces\"
    team: \"HAL Core Interfaces Team\"
    features:
      - \"PSX platform detection\"
      - \"Error types\""

create_dependencies "teams/flight-hal-dreamcast-memory" "team_info:
  name: \"Dreamcast Memory Team\"
  purpose: \"Implement Dreamcast memory management (16MB RAM, 8MB VRAM)\"
  
dependencies:
  - project: \"flight-hal-memory-interface\"
    team: \"HAL Memory Interface Team\"
    features:
      - \"Memory allocator interface\"
      - \"Memory region definitions\"
  - project: \"flight-hal-interfaces\"
    team: \"HAL Core Interfaces Team\"
    features:
      - \"Dreamcast platform detection\"
      - \"Error types\""

create_dependencies "teams/flight-hal-psp-memory" "team_info:
  name: \"PSP Memory Team\"
  purpose: \"Implement PSP memory management (32/64MB RAM, Media Engine RAM)\"
  
dependencies:
  - project: \"flight-hal-memory-interface\"
    team: \"HAL Memory Interface Team\"
    features:
      - \"Memory allocator interface\"
      - \"Memory region definitions\"
  - project: \"flight-hal-interfaces\"
    team: \"HAL Core Interfaces Team\"
    features:
      - \"PSP platform detection\"
      - \"Error types\""

create_dependencies "teams/flight-hal-macos-memory" "team_info:
  name: \"macOS Memory Team\"
  purpose: \"Implement macOS memory management with mmap and virtual memory\"
  
dependencies:
  - project: \"flight-hal-memory-interface\"
    team: \"HAL Memory Interface Team\"
    features:
      - \"Memory allocator interface\"
      - \"Virtual memory support\"
  - project: \"flight-hal-interfaces\"
    team: \"HAL Core Interfaces Team\"
    features:
      - \"macOS platform detection\"
      - \"Error types\""

create_dependencies "teams/flight-hal-windows-memory" "team_info:
  name: \"Windows Memory Team\"
  purpose: \"Implement Windows memory management with VirtualAlloc\"
  
dependencies:
  - project: \"flight-hal-memory-interface\"
    team: \"HAL Memory Interface Team\"
    features:
      - \"Memory allocator interface\"
      - \"Virtual memory support\"
  - project: \"flight-hal-interfaces\"
    team: \"HAL Core Interfaces Team\"
    features:
      - \"Windows platform detection\"
      - \"Error types\""

create_dependencies "teams/flight-hal-web-memory" "team_info:
  name: \"Web Memory Team\"
  purpose: \"Implement WebAssembly memory management\"
  
dependencies:
  - project: \"flight-hal-memory-interface\"
    team: \"HAL Memory Interface Team\"
    features:
      - \"Memory allocator interface\"
      - \"Linear memory abstraction\"
  - project: \"flight-hal-interfaces\"
    team: \"HAL Core Interfaces Team\"
    features:
      - \"Emscripten platform detection\"
      - \"Error types\""

# Continue for other memory teams...
for platform in linux ios android steamdeck; do
    create_dependencies "teams/flight-hal-${platform}-memory" "team_info:
  name: \"${platform^} Memory Team\"
  purpose: \"Implement ${platform} memory management\"
  
dependencies:
  - project: \"flight-hal-memory-interface\"
    team: \"HAL Memory Interface Team\"
    features:
      - \"Memory allocator interface\"
  - project: \"flight-hal-interfaces\"
    team: \"HAL Core Interfaces Team\"
    features:
      - \"Platform detection\"
      - \"Error types\""
done

# Layer 2: Graphics Teams
echo "=== Creating Platform Graphics Team Dependencies ==="

create_dependencies "teams/flight-hal-psx-gpu" "team_info:
  name: \"PSX GPU Team\"
  purpose: \"Implement PSX GPU with fixed-function pipeline\"
  
dependencies:
  - project: \"flight-hal-graphics-interface\"
    team: \"HAL Graphics Interface Team\"
    features:
      - \"Graphics context interface\"
      - \"Command buffer abstraction\"
  - project: \"flight-hal-psx-memory\"
    team: \"PSX Memory Team\"
    features:
      - \"VRAM allocation\"
      - \"Display list memory\"
  - project: \"flight-hal-interfaces\"
    team: \"HAL Core Interfaces Team\"
    features:
      - \"PSX platform types\""

create_dependencies "teams/flight-hal-dreamcast-pvr2" "team_info:
  name: \"Dreamcast PowerVR2 Team\"
  purpose: \"Implement Dreamcast tile-based rendering\"
  
dependencies:
  - project: \"flight-hal-graphics-interface\"
    team: \"HAL Graphics Interface Team\"
    features:
      - \"Graphics context interface\"
      - \"Render surface abstraction\"
  - project: \"flight-hal-dreamcast-memory\"
    team: \"Dreamcast Memory Team\"
    features:
      - \"VRAM allocation\"
      - \"Texture memory management\"
  - project: \"flight-hal-interfaces\"
    team: \"HAL Core Interfaces Team\"
    features:
      - \"Dreamcast platform types\""

create_dependencies "teams/flight-hal-psp-gu" "team_info:
  name: \"PSP GU Team\"
  purpose: \"Implement PSP Graphics Unit interface\"
  
dependencies:
  - project: \"flight-hal-graphics-interface\"
    team: \"HAL Graphics Interface Team\"
    features:
      - \"Graphics context interface\"
      - \"Command buffer abstraction\"
  - project: \"flight-hal-psp-memory\"
    team: \"PSP Memory Team\"
    features:
      - \"VRAM allocation\"
      - \"Display list memory\"
  - project: \"flight-hal-interfaces\"
    team: \"HAL Core Interfaces Team\"
    features:
      - \"PSP platform types\""

create_dependencies "teams/flight-hal-macos-metal" "team_info:
  name: \"macOS Metal Team\"
  purpose: \"Implement Metal graphics backend for macOS\"
  
dependencies:
  - project: \"flight-hal-graphics-interface\"
    team: \"HAL Graphics Interface Team\"
    features:
      - \"Graphics context interface\"
      - \"Command buffer abstraction\"
      - \"Shader interface\"
  - project: \"flight-hal-macos-memory\"
    team: \"macOS Memory Team\"
    features:
      - \"GPU buffer allocation\"
      - \"Shared memory support\"
  - project: \"flight-hal-interfaces\"
    team: \"HAL Core Interfaces Team\"
    features:
      - \"macOS platform types\""

# Continue for other graphics teams...

# Layer 3: Platform Integration Teams
echo "=== Creating Platform Integration Team Dependencies ==="

create_dependencies "teams/flight-hal-psx-integration" "team_info:
  name: \"PSX Platform Integration Team\"
  purpose: \"Integrate all PSX subsystems into cohesive platform\"
  
dependencies:
  - project: \"flight-hal-psx-memory\"
    team: \"PSX Memory Team\"
    features:
      - \"Complete memory management\"
      - \"All memory regions\"
  - project: \"flight-hal-psx-gpu\"
    team: \"PSX GPU Team\"
    features:
      - \"Graphics initialization\"
      - \"Display management\"
  - project: \"flight-hal-psx-spu\"
    team: \"PSX SPU Team\"
    features:
      - \"Audio initialization\"
      - \"SPU memory management\"
  - project: \"flight-hal-psx-pad\"
    team: \"PSX Pad Team\"
    features:
      - \"Controller input\"
      - \"Memory card access\"
  - project: \"flight-hal-psx-cdrom\"
    team: \"PSX CD-ROM Team\"
    features:
      - \"CD-ROM file access\"
      - \"Streaming support\"
  - project: \"flight-hal-interfaces\"
    team: \"HAL Core Interfaces Team\"
    features:
      - \"Platform interface\"
      - \"Driver registry\""

# Create README and project.json for each team
echo "=== Creating README files ==="
for team in teams/*/; do
    if [ -d "$team" ]; then
        team_name=$(basename "$team")
        echo "Creating README for $team_name"
        cat > "$team/README.md" << EOF
# $team_name

This directory contains the implementation for the $team_name.

## Structure

- \`src/\` - Source code implementation
- \`include/\` - Public headers
- \`tests/\` - Unit and integration tests
- \`proompts/\` - Team-specific prompts and documentation
- \`dependencies.yaml\` - Team dependencies

## Building

This project is built as part of the Flight HAL system using CMake and CPM.

\`\`\`bash
cmake -B build
cmake --build build
\`\`\`

## Dependencies

See \`dependencies.yaml\` for a complete list of project dependencies.
EOF

        # Create project.json for Nx
        cat > "$team/project.json" << EOF
{
  "name": "$team_name",
  "projectType": "library",
  "sourceRoot": "$team/src",
  "targets": {
    "build": {
      "executor": "@nrwl/cmake:build",
      "options": {
        "buildDirectory": "build"
      }
    },
    "test": {
      "executor": "@nrwl/cmake:test",
      "options": {
        "testDirectory": "build/tests"
      }
    }
  }
}
EOF
    fi
done

echo "All dependencies and project files created successfully!"
