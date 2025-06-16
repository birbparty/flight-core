#!/bin/bash

# Complete dependencies generation for all teams

# Helper function to create dependencies.yaml
create_dependencies() {
    local team_path=$1
    local content=$2
    
    echo "Creating dependencies.yaml for $team_path"
    cat > "$team_path/dependencies.yaml" << EOF
$content
EOF
}

# Complete the missing platform memory teams
for platform in linux ios android steamdeck; do
    Platform=$(echo "$platform" | sed 's/\b\(.\)/\u\1/g')  # Capitalize first letter
    create_dependencies "teams/flight-hal-${platform}-memory" "team_info:
  name: \"${Platform} Memory Team\"
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

# Complete graphics teams for other platforms
create_dependencies "teams/flight-hal-windows-d3d12" "team_info:
  name: \"Windows D3D12 Team\"
  purpose: \"Implement DirectX 12 graphics backend\"
  
dependencies:
  - project: \"flight-hal-graphics-interface\"
    team: \"HAL Graphics Interface Team\"
    features:
      - \"Graphics context interface\"
      - \"Command buffer abstraction\"
  - project: \"flight-hal-windows-memory\"
    team: \"Windows Memory Team\"
    features:
      - \"GPU buffer allocation\"
  - project: \"flight-hal-interfaces\"
    team: \"HAL Core Interfaces Team\"
    features:
      - \"Windows platform types\""

create_dependencies "teams/flight-hal-web-webgl" "team_info:
  name: \"Web WebGL Team\"
  purpose: \"Implement WebGL2 graphics backend\"
  
dependencies:
  - project: \"flight-hal-graphics-interface\"
    team: \"HAL Graphics Interface Team\"
    features:
      - \"Graphics context interface\"
  - project: \"flight-hal-web-memory\"
    team: \"Web Memory Team\"
    features:
      - \"Buffer allocation\"
  - project: \"flight-hal-interfaces\"
    team: \"HAL Core Interfaces Team\"
    features:
      - \"Emscripten platform types\""

# Audio teams
create_dependencies "teams/flight-hal-psx-spu" "team_info:
  name: \"PSX SPU Team\"
  purpose: \"Implement PSX Sound Processing Unit\"
  
dependencies:
  - project: \"flight-hal-audio-interface\"
    team: \"HAL Audio Interface Team\"
    features:
      - \"Audio device interface\"
      - \"Audio buffer management\"
  - project: \"flight-hal-psx-memory\"
    team: \"PSX Memory Team\"
    features:
      - \"SPU RAM allocation\"
  - project: \"flight-hal-interfaces\"
    team: \"HAL Core Interfaces Team\"
    features:
      - \"PSX platform types\""

# Input teams
create_dependencies "teams/flight-hal-psx-pad" "team_info:
  name: \"PSX Pad Team\"
  purpose: \"Implement PSX controller and memory card access\"
  
dependencies:
  - project: \"flight-hal-input-interface\"
    team: \"HAL Input Interface Team\"
    features:
      - \"Input device interface\"
      - \"Event handling\"
  - project: \"flight-hal-io-interface\"
    team: \"HAL I/O Interface Team\"
    features:
      - \"Memory card storage\"
  - project: \"flight-hal-interfaces\"
    team: \"HAL Core Interfaces Team\"
    features:
      - \"PSX platform types\""

# Storage teams
create_dependencies "teams/flight-hal-psx-cdrom" "team_info:
  name: \"PSX CD-ROM Team\"
  purpose: \"Implement PSX CD-ROM file access\"
  
dependencies:
  - project: \"flight-hal-io-interface\"
    team: \"HAL I/O Interface Team\"
    features:
      - \"File access interface\"
      - \"Stream interface\"
  - project: \"flight-hal-interfaces\"
    team: \"HAL Core Interfaces Team\"
    features:
      - \"PSX platform types\""

# Test teams
create_dependencies "teams/flight-hal-test-minimal" "team_info:
  name: \"HAL Minimal Test Team\"
  purpose: \"Test core HAL functionality across all platforms\"
  
dependencies:
  - project: \"flight-hal-interfaces\"
    team: \"HAL Core Interfaces Team\"
    features:
      - \"All core interfaces\"
  - project: \"flight-hal-memory-interface\"
    team: \"HAL Memory Interface Team\"
    features:
      - \"Memory test interfaces\"
  - project: \"flight-hal-time-interface\"
    team: \"HAL Time Interface Team\"
    features:
      - \"Time test interfaces\""

create_dependencies "teams/flight-hal-test-memory" "team_info:
  name: \"HAL Memory Test Team\"
  purpose: \"Test memory subsystem implementations\"
  
dependencies:
  - project: \"flight-hal-memory-interface\"
    team: \"HAL Memory Interface Team\"
    features:
      - \"Memory interfaces\"
  - project: \"flight-hal-test-minimal\"
    team: \"HAL Minimal Test Team\"
    features:
      - \"Test framework\"
      - \"Platform detection\""

# Build tools
create_dependencies "teams/flight-hal-build-tools" "team_info:
  name: \"HAL Build Tools Team\"
  purpose: \"Provide cross-platform build tooling and toolchains\"
  
dependencies:
  - project: \"flight-hal-interfaces\"
    team: \"HAL Core Interfaces Team\"
    features:
      - \"Platform definitions\"
      - \"Build configuration types\""

# Complete missing integration teams
for platform in dreamcast windows web linux ios android steamdeck; do
    Platform=$(echo "$platform" | sed 's/\b\(.\)/\u\1/g')
    
    # Skip PSX as it's already done
    if [ "$platform" = "psx" ]; then
        continue
    fi
    
    create_dependencies "teams/flight-hal-${platform}-integration" "team_info:
  name: \"${Platform} Platform Integration Team\"
  purpose: \"Integrate all ${Platform} subsystems into cohesive platform\"
  
dependencies:
  - project: \"flight-hal-${platform}-memory\"
    team: \"${Platform} Memory Team\"
    features:
      - \"Complete memory management\"
  - project: \"flight-hal-interfaces\"
    team: \"HAL Core Interfaces Team\"
    features:
      - \"Platform interface\"
      - \"Driver registry\""
done

echo "All missing dependencies created successfully!"
