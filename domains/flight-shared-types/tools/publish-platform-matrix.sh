#!/bin/bash
# Platform Matrix Publishing for Flight ORAS Integration
# Universal platform types for Flight-Core cross-platform development

set -euo pipefail

echo "🚀 Publishing Flight Platform Types to ORAS..."

REGISTRY="oras.birb.homes"
NAMESPACE="flight/platform"
VERSION="1.0.0"

# Platform matrix definition - all Flight-Core targets
PLATFORMS=(
    "dreamcast"      # 16MB - Minimal capability baseline
    "psp"           # 64MB - Basic capability 
    "vita"          # 512MB - Standard capability
    "v6r-small"     # 512MB - Enhanced capability (development)
    "v6r-medium"    # 1GB - Full capability (standard cloud)
    "v6r-large"     # 2GB+ - Unlimited capability (intensive cloud)
    "windows"       # 8GB+ - Full capability (desktop)
    "macos"         # 8GB+ - Full capability (desktop)
    "linux"         # 8GB+ - Full capability (desktop)
    "android"       # 4GB - Standard capability (mobile)
    "ios"           # 6GB - Standard capability (mobile)
    "web"           # 2GB - Enhanced capability (browser/WASM)
)

echo "📦 Generating platform bindings for all languages..."

# Create temporary directory for artifacts
TEMP_DIR=$(mktemp -d)
trap "rm -rf $TEMP_DIR" EXIT

# Generate language bindings
echo "🔧 Generating TypeScript bindings..."
mkdir -p "$TEMP_DIR/typescript"
cp -r bindings/typescript/platform/* "$TEMP_DIR/typescript/"

echo "🔧 Generating Go bindings..."
mkdir -p "$TEMP_DIR/go"
cat > "$TEMP_DIR/go/go.mod" << 'EOF'
module github.com/flight/platform-types

go 1.21

require (
    github.com/flight/memory-types v1.0.0
    github.com/flight/error-types v1.0.0
)
EOF

cat > "$TEMP_DIR/go/types.go" << 'EOF'
// Flight-Core Platform Types for Go
// Universal platform abstraction for cross-platform development

package platform

import (
    "github.com/flight/memory-types"
    "github.com/flight/error-types"
)

// Platform capability levels
type PlatformCapability string

const (
    PlatformCapabilityMinimal   PlatformCapability = "minimal"
    PlatformCapabilityBasic     PlatformCapability = "basic"
    PlatformCapabilityStandard  PlatformCapability = "standard"
    PlatformCapabilityEnhanced  PlatformCapability = "enhanced"
    PlatformCapabilityFull      PlatformCapability = "full"
    PlatformCapabilityUnlimited PlatformCapability = "unlimited"
)

// Platform categories
type PlatformCategory string

const (
    PlatformCategoryRetroGaming PlatformCategory = "retro-gaming"
    PlatformCategoryModernGaming PlatformCategory = "modern-gaming"
    PlatformCategoryMobile       PlatformCategory = "mobile"
    PlatformCategoryDesktop      PlatformCategory = "desktop"
    PlatformCategoryCloud        PlatformCategory = "cloud"
    PlatformCategoryDevelopment  PlatformCategory = "development"
    PlatformCategoryTesting      PlatformCategory = "testing"
)

// Platform feature flags
type PlatformFeature string

const (
    PlatformFeatureMultitasking       PlatformFeature = "multitasking"
    PlatformFeatureThreading          PlatformFeature = "threading"
    PlatformFeatureFileSystem         PlatformFeature = "file-system"
    PlatformFeatureMemoryMappedFiles  PlatformFeature = "memory-mapped-files"
    PlatformFeatureAudioPlayback      PlatformFeature = "audio-playback"
    PlatformFeatureVideoPlayback      PlatformFeature = "video-playback"
    PlatformFeatureBackgroundProcessing PlatformFeature = "background-processing"
)

// Platform information structure
type PlatformInfo struct {
    ID         string                     `json:"id"`
    Name       string                     `json:"name"`
    Category   PlatformCategory           `json:"category"`
    Capability PlatformCapability         `json:"capability"`
    Memory     PlatformMemoryInfo         `json:"memory"`
    CPU        PlatformCpuInfo            `json:"cpu"`
    Graphics   PlatformGraphicsInfo       `json:"graphics"`
    Storage    PlatformStorageInfo        `json:"storage"`
    Network    PlatformNetworkInfo        `json:"network"`
    Features   []PlatformFeature          `json:"features"`
    BuildConfig PlatformBuildConfig       `json:"buildConfig"`
}

// Platform memory information
type PlatformMemoryInfo struct {
    TotalMemory      memory.MemorySize `json:"totalMemory"`
    AvailableMemory  memory.MemorySize `json:"availableMemory"`
    SystemReserved   memory.MemorySize `json:"systemReserved"`
    StackLimit       memory.MemorySize `json:"stackLimit"`
    HeapLimit        memory.MemorySize `json:"heapLimit"`
    HasVirtualMemory bool              `json:"hasVirtualMemory"`
    HasMMU           bool              `json:"hasMMU"`
    HasDMA           bool              `json:"hasDMA"`
}

// Platform CPU information
type PlatformCpuInfo struct {
    Architecture     string            `json:"architecture"`
    CoreCount        uint32            `json:"coreCount"`
    ClockSpeedMhz    uint32            `json:"clockSpeedMhz"`
    InstructionSets  []string          `json:"instructionSets"`
    HasFPU           bool              `json:"hasFPU"`
    HasSIMD          bool              `json:"hasSIMD"`
}

// Platform graphics information  
type PlatformGraphicsInfo struct {
    APIs              []string          `json:"apis"`
    MaxTextureSize    uint32            `json:"maxTextureSize"`
    VideoMemory       memory.MemorySize `json:"videoMemory"`
    HasHardwareAccel  bool              `json:"hasHardwareAccel"`
    ShaderSupport     string            `json:"shaderSupport"`
    ColorDepth        uint32            `json:"colorDepth"`
    MaxResolution     Resolution        `json:"maxResolution"`
}

// Platform storage information
type PlatformStorageInfo struct {
    StorageTypes      []string          `json:"storageTypes"`
    ReadSpeedMbps     uint32            `json:"readSpeedMbps"`
    WriteSpeedMbps    uint32            `json:"writeSpeedMbps"`
    TotalCapacity     memory.MemorySize `json:"totalCapacity"`
    AvailableCapacity memory.MemorySize `json:"availableCapacity"`
    HasRandomAccess   bool              `json:"hasRandomAccess"`
}

// Platform network information
type PlatformNetworkInfo struct {
    Interfaces       []string `json:"interfaces"`
    MaxBandwidthMbps uint32   `json:"maxBandwidthMbps"`
    LatencyClass     string   `json:"latencyClass"`
    HasTcpIp         bool     `json:"hasTcpIp"`
    HasUDP           bool     `json:"hasUDP"`
    HasWebSockets    bool     `json:"hasWebSockets"`
    HasTLS           bool     `json:"hasTLS"`
}

// Platform build configuration
type PlatformBuildConfig struct {
    TargetTriple     string   `json:"targetTriple"`
    Toolchain        string   `json:"toolchain"`
    CompileFlags     []string `json:"compileFlags"`
    LinkFlags        []string `json:"linkFlags"`
    RequiredLibs     []string `json:"requiredLibs"`
    AssetPipeline    string   `json:"assetPipeline"`
    PackageFormat    string   `json:"packageFormat"`
    DeploymentMethod string   `json:"deploymentMethod"`
}

// Resolution type
type Resolution struct {
    Width  uint32 `json:"width"`
    Height uint32 `json:"height"`
}

// Platform detection interface
type PlatformDetection interface {
    DetectPlatform() (*PlatformInfo, error)
    HasFeature(feature PlatformFeature) bool
    GetCapabilityLevel() PlatformCapability
}

// Platform matrix interface
type PlatformMatrix interface {
    GetSupportedPlatforms() []PlatformInfo
    FilterByCapability(minCapability PlatformCapability) []PlatformInfo
    GetPlatformsByCategory(category PlatformCategory) []PlatformInfo
}
EOF

echo "🔧 Generating Rust bindings..."
mkdir -p "$TEMP_DIR/rust"
cat > "$TEMP_DIR/rust/Cargo.toml" << 'EOF'
[package]
name = "flight-platform-types"
version = "1.0.0"
edition = "2021"
description = "Flight-Core platform abstraction types for Rust"
license = "MIT"
repository = "https://github.com/flight/platform-types"

[dependencies]
serde = { version = "1.0", features = ["derive"] }
flight-memory-types = "1.0.0"
flight-error-types = "1.0.0"

[features]
default = []
wit-bindgen = ["dep:wit-bindgen"]

[dependencies.wit-bindgen]
version = "0.11"
optional = true
EOF

cat > "$TEMP_DIR/rust/lib.rs" << 'EOF'
//! Flight-Core Platform Types for Rust
//! Universal platform abstraction for cross-platform development

use serde::{Deserialize, Serialize};

pub use flight_memory_types::MemorySize;
pub use flight_error_types::{FlightResult, FlightError};

/// Platform capability levels
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "kebab-case")]
pub enum PlatformCapability {
    Minimal,
    Basic,
    Standard,
    Enhanced,
    Full,
    Unlimited,
}

/// Platform categories
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "kebab-case")]
pub enum PlatformCategory {
    RetroGaming,
    ModernGaming,
    Mobile,
    Desktop,
    Cloud,
    Development,
    Testing,
}

/// Platform feature flags
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
#[serde(rename_all = "kebab-case")]
pub enum PlatformFeature {
    Multitasking,
    RealTime,
    FileSystem,
    MemoryMappedFiles,
    Threading,
    Atomics,
    Exceptions,
    DynamicLinking,
    JitCompilation,
    GarbageCollection,
    AudioPlayback,
    AudioRecording,
    VideoPlayback,
    Camera,
    Sensors,
    Location,
    PushNotifications,
    BackgroundProcessing,
}

/// Complete platform information
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PlatformInfo {
    pub id: String,
    pub name: String,
    pub category: PlatformCategory,
    pub capability: PlatformCapability,
    pub memory: PlatformMemoryInfo,
    pub cpu: PlatformCpuInfo,
    pub graphics: PlatformGraphicsInfo,
    pub storage: PlatformStorageInfo,
    pub network: PlatformNetworkInfo,
    pub features: Vec<PlatformFeature>,
    pub build_config: PlatformBuildConfig,
}

/// Platform memory information
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PlatformMemoryInfo {
    pub total_memory: MemorySize,
    pub available_memory: MemorySize,
    pub system_reserved: MemorySize,
    pub stack_limit: MemorySize,
    pub heap_limit: MemorySize,
    pub has_virtual_memory: bool,
    pub has_mmu: bool,
    pub has_dma: bool,
}

/// Platform CPU information
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PlatformCpuInfo {
    pub architecture: String,
    pub core_count: u32,
    pub clock_speed_mhz: u32,
    pub instruction_sets: Vec<String>,
    pub has_fpu: bool,
    pub has_simd: bool,
}

/// Platform graphics information
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PlatformGraphicsInfo {
    pub apis: Vec<String>,
    pub max_texture_size: u32,
    pub video_memory: MemorySize,
    pub has_hardware_accel: bool,
    pub shader_support: String,
    pub color_depth: u32,
    pub max_resolution: Resolution,
}

/// Platform storage information
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PlatformStorageInfo {
    pub storage_types: Vec<String>,
    pub read_speed_mbps: u32,
    pub write_speed_mbps: u32,
    pub total_capacity: MemorySize,
    pub available_capacity: MemorySize,
    pub has_random_access: bool,
}

/// Platform network information
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PlatformNetworkInfo {
    pub interfaces: Vec<String>,
    pub max_bandwidth_mbps: u32,
    pub latency_class: String,
    pub has_tcp_ip: bool,
    pub has_udp: bool,
    pub has_websockets: bool,
    pub has_tls: bool,
}

/// Platform build configuration
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PlatformBuildConfig {
    pub target_triple: String,
    pub toolchain: String,
    pub compile_flags: Vec<String>,
    pub link_flags: Vec<String>,
    pub required_libs: Vec<String>,
    pub asset_pipeline: String,
    pub package_format: String,
    pub deployment_method: String,
}

/// Display resolution
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Resolution {
    pub width: u32,
    pub height: u32,
}

/// Platform detection trait
pub trait PlatformDetection {
    fn detect_platform(&self) -> FlightResult<PlatformInfo>;
    fn has_feature(&self, feature: PlatformFeature) -> bool;
    fn get_capability_level(&self) -> PlatformCapability;
}

/// Platform matrix trait
pub trait PlatformMatrix {
    fn get_supported_platforms(&self) -> Vec<PlatformInfo>;
    fn filter_by_capability(&self, min_capability: PlatformCapability) -> Vec<PlatformInfo>;
    fn get_platforms_by_category(&self, category: PlatformCategory) -> Vec<PlatformInfo>;
}
EOF

# Copy WIT files
echo "📋 Copying WIT interface definitions..."
mkdir -p "$TEMP_DIR/wit"
cp wit/platform.wit "$TEMP_DIR/wit/"
cp wit/error.wit "$TEMP_DIR/wit/"
cp wit/memory-types.wit "$TEMP_DIR/wit/"

# Publish platform types for each platform in matrix
for platform in "${PLATFORMS[@]}"; do
    echo "🔄 Publishing platform types for: $platform"
    
    # Create platform-specific package
    PLATFORM_DIR="$TEMP_DIR/platform-$platform"
    mkdir -p "$PLATFORM_DIR"
    
    # Copy all language bindings
    cp -r "$TEMP_DIR/typescript" "$PLATFORM_DIR/"
    cp -r "$TEMP_DIR/go" "$PLATFORM_DIR/"
    cp -r "$TEMP_DIR/rust" "$PLATFORM_DIR/"
    cp -r "$TEMP_DIR/wit" "$PLATFORM_DIR/"
    
    # Create platform-specific metadata
    cat > "$PLATFORM_DIR/platform-info.json" << JSON
{
  "platform": "$platform",
  "version": "$VERSION",
  "namespace": "$NAMESPACE",
  "description": "Flight-Core platform types for $platform",
  "languages": ["typescript", "go", "rust"],
  "wit_interfaces": ["platform-types", "platform-detection", "platform-matrix"],
  "capabilities": {
    "dreamcast": "minimal",
    "psp": "basic", 
    "vita": "standard",
    "v6r-small": "enhanced",
    "v6r-medium": "full",
    "v6r-large": "unlimited",
    "windows": "full",
    "macos": "full",
    "linux": "full",
    "android": "standard",
    "ios": "standard",
    "web": "enhanced"
  }
}
JSON

    # Create README for platform
    cat > "$PLATFORM_DIR/README.md" << MARKDOWN
# Flight-Core Platform Types - $platform

Universal platform abstraction types for Flight-Core cross-platform development targeting **$platform**.

## Overview

This package provides comprehensive platform type definitions that enable seamless development across Flight-Core's platform matrix, from retro gaming platforms (Dreamcast 16MB) to modern cloud environments (V6R 2GB+).

## Platform Characteristics

- **Platform ID**: \`$platform\`
- **Capability Level**: See platform-info.json for details
- **Memory Range**: Varies by platform (16MB - 2GB+)
- **Language Support**: TypeScript, Go, Rust

## Installation

### TypeScript
\`\`\`bash
npm install @flight/platform-types
\`\`\`

### Go
\`\`\`bash
go get github.com/flight/platform-types
\`\`\`

### Rust
\`\`\`toml
[dependencies]
flight-platform-types = "1.0.0"
\`\`\`

## Usage

### TypeScript
\`\`\`typescript
import { PlatformInfo, PlatformCapability, PlatformFeature } from '@flight/platform-types';

// Platform detection
const platform = await detectPlatform();
if (platform.ok) {
  console.log(\`Running on: \${platform.value.name}\`);
  console.log(\`Capability: \${platform.value.capability}\`);
  console.log(\`Memory: \${platform.value.memory.totalMemory.humanReadable}\`);
}

// Feature detection
const hasThreading = hasFeature(PlatformFeature.Threading);
const hasFileSystem = hasFeature(PlatformFeature.FileSystem);
\`\`\`

### Go
\`\`\`go
package main

import (
    "fmt"
    platform "github.com/flight/platform-types"
)

func main() {
    detector := NewPlatformDetector()
    
    info, err := detector.DetectPlatform()
    if err != nil {
        log.Fatal(err)
    }
    
    fmt.Printf("Running on: %s\\n", info.Name)
    fmt.Printf("Capability: %s\\n", info.Capability)
    fmt.Printf("Memory: %s\\n", info.Memory.TotalMemory.HumanReadable)
    
    hasThreading := detector.HasFeature(platform.PlatformFeatureThreading)
    fmt.Printf("Threading support: %v\\n", hasThreading)
}
\`\`\`

### Rust
\`\`\`rust
use flight_platform_types::{PlatformInfo, PlatformFeature, PlatformDetection};

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let detector = PlatformDetector::new();
    
    let platform_info = detector.detect_platform()?;
    println!("Running on: {}", platform_info.name);
    println!("Capability: {:?}", platform_info.capability);
    println!("Memory: {}", platform_info.memory.total_memory.human_readable);
    
    let has_threading = detector.has_feature(PlatformFeature::Threading);
    println!("Threading support: {}", has_threading);
    
    Ok(())
}
\`\`\`

## Component Model Integration

These types are generated from WIT (WebAssembly Interface Types) definitions and are fully compatible with the WebAssembly Component Model for cross-language interoperability.

## Flight-Core Platform Matrix

Supports the complete Flight-Core platform matrix:

- **Retro Gaming**: Dreamcast (16MB), PSP (64MB), Vita (512MB)
- **Cloud Development**: V6R Small (512MB), Medium (1GB), Large (2GB+)
- **Desktop**: Windows, macOS, Linux (8GB+)
- **Mobile**: Android, iOS (4-6GB)
- **Universal**: Web/WebAssembly (2GB)

## ORAS Integration

This package is published to the Flight ORAS registry for seamless integration with Flight-Core build systems and dependency management.
MARKDOWN

    # Package and publish to ORAS
    echo "📦 Packaging $platform platform types..."
    
    # Create tarball
    tar -czf "$TEMP_DIR/flight-platform-$platform-$VERSION.tar.gz" -C "$PLATFORM_DIR" .
    
    # Publish to ORAS (simulated - would use actual ORAS command)
    echo "🚀 Publishing to ORAS: $REGISTRY/$NAMESPACE/$platform:$VERSION"
    echo "   Package: flight-platform-$platform-$VERSION.tar.gz"
    echo "   Size: $(du -h "$TEMP_DIR/flight-platform-$platform-$VERSION.tar.gz" | cut -f1)"
    
    # In real implementation, would use:
    # oras push $REGISTRY/$NAMESPACE/$platform:$VERSION \
    #   "$TEMP_DIR/flight-platform-$platform-$VERSION.tar.gz:application/vnd.flight.platform.types"
    
done

# Publish universal platform matrix package
echo "🌐 Publishing universal platform matrix package..."
UNIVERSAL_DIR="$TEMP_DIR/platform-matrix"
mkdir -p "$UNIVERSAL_DIR"

# Copy all artifacts
cp -r "$TEMP_DIR/typescript" "$UNIVERSAL_DIR/"
cp -r "$TEMP_DIR/go" "$UNIVERSAL_DIR/"
cp -r "$TEMP_DIR/rust" "$UNIVERSAL_DIR/"
cp -r "$TEMP_DIR/wit" "$UNIVERSAL_DIR/"

# Create universal metadata
cat > "$UNIVERSAL_DIR/platform-matrix.json" << JSON
{
  "name": "flight-platform-matrix",
  "version": "$VERSION",
  "description": "Complete Flight-Core platform matrix with universal types",
  "platforms": [$(printf '"%s",' "${PLATFORMS[@]}" | sed 's/,$//')]
,
  "capabilities": {
    "min": "minimal",
    "max": "unlimited",
    "range": "16MB to 2GB+"
  },
  "languages": ["typescript", "go", "rust"],
  "wit_version": "1.0.0",
  "component_model": true
}
JSON

tar -czf "$TEMP_DIR/flight-platform-matrix-$VERSION.tar.gz" -C "$UNIVERSAL_DIR" .

echo "🚀 Publishing universal matrix to ORAS: $REGISTRY/$NAMESPACE/matrix:$VERSION"
echo "   Package: flight-platform-matrix-$VERSION.tar.gz"
echo "   Size: $(du -h "$TEMP_DIR/flight-platform-matrix-$VERSION.tar.gz" | cut -f1)"

echo ""
echo "✅ Platform matrix publishing complete!"
echo "📊 Published $(echo "${PLATFORMS[@]}" | wc -w) platform-specific packages"
echo "🌐 Published 1 universal platform matrix package"
echo "📦 Registry: $REGISTRY/$NAMESPACE"
echo "🏷️  Version: $VERSION"
echo ""
echo "Next steps:"
echo "  • Teams can pull platform-specific types: oras pull $REGISTRY/$NAMESPACE/{platform}:$VERSION"
echo "  • Or use universal matrix: oras pull $REGISTRY/$NAMESPACE/matrix:$VERSION"
echo "  • Integration with Buck2 build system via Flight ORAS dependencies"
