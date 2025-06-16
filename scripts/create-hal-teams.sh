#!/bin/bash

# Create HAL team directories with dependencies

# Helper function to create team directory structure
create_team() {
    local team_name=$1
    local team_path="teams/$team_name"
    
    echo "Creating team: $team_name"
    mkdir -p "$team_path/proompts"
    mkdir -p "$team_path/src"
    mkdir -p "$team_path/include"
    mkdir -p "$team_path/tests"
}

# Create Interface Teams (Layer 1 - No dependencies)
echo "=== Creating Interface Teams ==="
create_team "flight-hal-interfaces"
create_team "flight-hal-memory-interface"
create_team "flight-hal-time-interface"
create_team "flight-hal-io-interface"
create_team "flight-hal-threading-interface"
create_team "flight-hal-graphics-interface"
create_team "flight-hal-audio-interface"
create_team "flight-hal-input-interface"

# Create Platform Memory Teams (Layer 2)
echo "=== Creating Platform Memory Teams ==="
create_team "flight-hal-dreamcast-memory"
create_team "flight-hal-psp-memory"
create_team "flight-hal-macos-memory"
create_team "flight-hal-windows-memory"
create_team "flight-hal-web-memory"
create_team "flight-hal-linux-memory"
create_team "flight-hal-ios-memory"
create_team "flight-hal-android-memory"
create_team "flight-hal-steamdeck-memory"

# Create Platform Subsystem Teams (Layer 2)
echo "=== Creating Platform Subsystem Teams ==="
# Graphics
create_team "flight-hal-dreamcast-pvr2"
create_team "flight-hal-psp-gu"
create_team "flight-hal-macos-metal"
create_team "flight-hal-windows-d3d12"
create_team "flight-hal-web-webgl"
create_team "flight-hal-linux-vulkan"
create_team "flight-hal-ios-metal"
create_team "flight-hal-android-vulkan"
create_team "flight-hal-steamdeck-vulkan"

# Audio
create_team "flight-hal-dreamcast-aica"
create_team "flight-hal-psp-audio"
create_team "flight-hal-macos-coreaudio"
create_team "flight-hal-windows-wasapi"
create_team "flight-hal-web-webaudio"
create_team "flight-hal-linux-alsa"
create_team "flight-hal-ios-coreaudio"
create_team "flight-hal-android-aaudio"
create_team "flight-hal-steamdeck-pipewire"

# Input
create_team "flight-hal-dreamcast-maple"
create_team "flight-hal-psp-ctrl"
create_team "flight-hal-macos-iokit"
create_team "flight-hal-windows-xinput"
create_team "flight-hal-web-gamepad"
create_team "flight-hal-linux-evdev"
create_team "flight-hal-ios-touch"
create_team "flight-hal-android-input"
create_team "flight-hal-steamdeck-steam-input"

# Storage/IO
create_team "flight-hal-dreamcast-gdrom"
create_team "flight-hal-psp-ms"

# Terminal Platform Teams
echo "=== Creating Terminal Platform Teams ==="
create_team "flight-hal-terminal-memory"
create_team "flight-hal-terminal-renderer"
create_team "flight-hal-terminal-input"

# Create Platform Integration Teams (Layer 3)
echo "=== Creating Platform Integration Teams ==="
create_team "flight-hal-dreamcast-integration"
create_team "flight-hal-psp-integration"
create_team "flight-hal-macos-integration"
create_team "flight-hal-windows-integration"
create_team "flight-hal-web-integration"
create_team "flight-hal-linux-integration"
create_team "flight-hal-ios-integration"
create_team "flight-hal-android-integration"
create_team "flight-hal-steamdeck-integration"
create_team "flight-hal-terminal-integration"

# Create Testing Teams (Layer 4)
echo "=== Creating Testing Teams ==="
create_team "flight-hal-test-minimal"
create_team "flight-hal-test-memory"
create_team "flight-hal-test-timing"
create_team "flight-hal-test-graphics"
create_team "flight-hal-test-audio"
create_team "flight-hal-test-input"
create_team "flight-hal-test-integration"
create_team "flight-hal-test-performance"

# Create Tooling Teams (Layer 4)
echo "=== Creating Tooling Teams ==="
create_team "flight-hal-build-tools"
create_team "flight-hal-dev-tools"
create_team "flight-hal-profiling-tools"

echo "All teams created successfully!"
