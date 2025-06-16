# Flight-Core Platform Types Integration Guide

## Overview

The Flight-Core Platform Types system provides a universal platform abstraction layer that enables seamless cross-platform development from Dreamcast (16MB) to V6R cloud environments (2GB+). This system bridges the massive capability gap across Flight-Core's 8+ target platforms while maintaining full type safety and performance.

## Architecture

### Core Components

#### 1. WIT Interface Definition (`wit/platform.wit`)
- **Platform Types Interface**: Core platform abstraction types
- **Platform Detection Interface**: Runtime platform detection and adaptation
- **Platform Matrix Interface**: Multi-platform compatibility operations
- **Component Model Entry Point**: WebAssembly Component Model integration

#### 2. Platform Registry (`src/platform/platform_registry.rs`)
- **Comprehensive Platform Definitions**: All 12 Flight-Core platforms
- **Capability Classification**: 6-level hierarchy (minimal → unlimited)
- **Feature Matrix**: 19 platform features with compatibility mapping
- **Memory Scaling**: 1000:1 ratio support (16MB → 2GB+)

#### 3. Language Bindings
- **TypeScript**: Enhanced types with V6R integration
- **Go**: Performance-optimized Go types
- **Rust**: Zero-cost abstractions with serde support
- **C++17**: Memory-efficient native bindings

#### 4. ORAS Integration
- **Platform Matrix Publishing**: Automated multi-platform package publishing
- **Buck2 Integration**: Seamless build system integration
- **Dependency Management**: Version-controlled platform types

## Platform Matrix

### Supported Platforms

| Platform | Memory | Capability | Category | Architecture | Features |
|----------|--------|------------|----------|--------------|----------|
| **Dreamcast** | 16MB | Minimal | Retro Gaming | SH4 | Audio, Video, FileSystem |
| **PSP** | 64MB | Basic | Retro Gaming | MIPS | + Threading |
| **Vita** | 512MB | Standard | Modern Gaming | ARM | + Multitasking, Camera, Sensors |
| **V6R Small** | 512MB | Enhanced | Development | x86_64 | + Memory Mapping, Dynamic Linking |
| **V6R Medium** | 1GB | Full | Cloud | x86_64 | + JIT, Background Processing |
| **V6R Large** | 2GB+ | Unlimited | Cloud | x86_64 | + Garbage Collection, Full Features |
| **Windows** | 8GB+ | Full | Desktop | x86_64 | DirectX, Full Desktop Features |
| **macOS** | 8GB+ | Full | Desktop | ARM64 | Metal, Unified Memory |
| **Linux** | 8GB+ | Full | Desktop | x86_64 | Open Source Stack |
| **Android** | 4GB | Standard | Mobile | ARM64 | Mobile Features, Sensors |
| **iOS** | 6GB | Standard | Mobile | ARM64 | Metal, iOS Features |
| **Web** | 2GB | Enhanced | Testing | WASM | WebAssembly Runtime |

### Capability Levels

1. **Minimal** (Dreamcast): Basic audio/video/filesystem
2. **Basic** (PSP): + Threading support
3. **Standard** (Vita, Mobile): + Multitasking, sensors, camera
4. **Enhanced** (V6R Small, Web): + Memory mapping, dynamic linking
5. **Full** (V6R Medium+, Desktop): + JIT, background processing
6. **Unlimited** (V6R Large): + Garbage collection, all features

## Integration Patterns

### TypeScript Usage

```typescript
import { 
  PlatformInfo, 
  PlatformCapability, 
  PlatformFeature,
  PlatformDetection 
} from '@flight-core/platform-types';

// Platform detection
const detector: PlatformDetection = new WebPlatformDetector();
const platform = await detector.detectPlatform();

if (platform.ok) {
  console.log(`Platform: ${platform.value.name}`);
  console.log(`Memory: ${platform.value.memory.totalMemory.humanReadable}`);
  console.log(`Capability: ${platform.value.capability}`);
  
  // Feature adaptation
  const strategy = detector.adaptFeature(
    PlatformFeature.Threading,
    PlatformCapability.Enhanced
  );
  
  switch (strategy) {
    case 'full-implementation':
      // Use full threading
      break;
    case 'alternative-implementation':
      // Use web workers
      break;
    case 'not-available':
      // Single-threaded fallback
      break;
  }
}
```

### Go Usage

```go
package main

import (
    "fmt"
    "log"
    platform "github.com/flight-core/platform-types"
)

func main() {
    detector := platform.NewPlatformDetector()
    
    info, err := detector.DetectPlatform()
    if err != nil {
        log.Fatal(err)
    }
    
    fmt.Printf("Platform: %s\n", info.Name)
    fmt.Printf("Memory: %s\n", info.Memory.TotalMemory.HumanReadable)
    fmt.Printf("Capability: %s\n", info.Capability)
    
    // Feature detection
    if detector.HasFeature(platform.PlatformFeatureThreading) {
        fmt.Println("Threading supported")
    }
    
    // Platform matrix operations
    matrix := platform.NewPlatformMatrix()
    
    // Get all cloud platforms
    cloudPlatforms := matrix.GetPlatformsByCategory(
        platform.PlatformCategoryCloud
    )
    
    for _, p := range cloudPlatforms {
        fmt.Printf("Cloud platform: %s (%s)\n", p.Name, p.Capability)
    }
}
```

### Rust Usage

```rust
use flight_platform_types::{
    PlatformInfo, 
    PlatformFeature, 
    PlatformDetection,
    PlatformMatrix
};

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let detector = PlatformDetector::new();
    
    // Detect current platform
    let platform = detector.detect_platform()?;
    println!("Platform: {}", platform.name);
    println!("Memory: {}", platform.memory.total_memory.human_readable);
    println!("Capability: {:?}", platform.capability);
    
    // Check feature availability
    let has_threading = detector.has_feature(PlatformFeature::Threading);
    println!("Threading: {}", has_threading);
    
    // Platform matrix operations
    let matrix = PlatformMatrix::new();
    let enhanced_platforms = matrix.filter_by_capability(
        PlatformCapability::Enhanced
    );
    
    println!("Enhanced platforms:");
    for platform in enhanced_platforms {
        println!("  {} ({}MB)", platform.name, 
                 platform.memory.total_memory.bytes / 1024 / 1024);
    }
    
    Ok(())
}
```

## Cross-Platform Development Patterns

### Memory Adaptation

```typescript
// Automatic memory scaling based on platform
function adaptMemoryUsage(platform: PlatformInfo): MemoryConfig {
  const totalMB = platform.memory.totalMemory.bytes / 1024 / 1024;
  
  if (totalMB <= 64) {
    // Dreamcast/PSP: Ultra-conservative
    return {
      cacheSize: Math.min(2 * 1024 * 1024, totalMB * 0.1),
      maxTextures: 16,
      strategy: 'conservative'
    };
  } else if (totalMB <= 512) {
    // Vita/V6R Small: Balanced
    return {
      cacheSize: totalMB * 0.2,
      maxTextures: 64,
      strategy: 'balanced'
    };
  } else {
    // V6R Medium+/Desktop: Aggressive
    return {
      cacheSize: totalMB * 0.4,
      maxTextures: 256,
      strategy: 'aggressive'
    };
  }
}
```

### Feature Detection

```typescript
function getOptimalRenderingStrategy(platform: PlatformInfo): RenderingStrategy {
  if (platform.graphics.hasHardwareAccel && 
      platform.graphics.shaderSupport !== 'none') {
    return {
      type: 'hardware',
      api: platform.graphics.apis[0],
      maxTextureSize: platform.graphics.maxTextureSize
    };
  }
  
  return {
    type: 'software',
    api: 'software',
    maxTextureSize: 512 // Safe fallback
  };
}
```

### Compatibility Checking

```typescript
async function checkCrossplatformCompatibility(
  source: string, 
  target: string
): Promise<CompatibilityReport> {
  const matrix = new PlatformMatrix();
  return await matrix.checkCompatibility(source, target);
}

// Usage
const report = await checkCrossplatformCompatibility('dreamcast', 'v6r-large');
if (!report.compatible) {
  console.warn('Compatibility issues found:');
  report.issues.forEach(issue => {
    console.warn(`  ${issue.severity}: ${issue.description}`);
  });
}
```

## V6R Cloud Integration

### VM Sizing Strategy

```typescript
function selectOptimalV6RSize(requirements: AppRequirements): V6RVmSize {
  const estimatedMemory = calculateMemoryRequirements(requirements);
  
  if (estimatedMemory <= 400 * 1024 * 1024) {
    return V6RVmSize.Small;  // 512MB
  } else if (estimatedMemory <= 800 * 1024 * 1024) {
    return V6RVmSize.Medium; // 1GB
  } else {
    return V6RVmSize.Large;  // 2GB+
  }
}
```

### Auto-scaling Configuration

```typescript
const v6rConfig: V6RPlatformExtensions = {
  vmSize: V6RVmSize.Medium,
  containerRuntime: 'containerd',
  orchestration: 'kubernetes',
  autoScaling: true,
  loadBalancing: true,
  serviceMesh: true,
  monitoring: true
};
```

## Build System Integration

### Buck2 Integration

```python
# BUCK file
load("@prelude//flight:defs.bzl", "flight_platform_target")

flight_platform_target(
    name = "my_component",
    srcs = ["src/main.rs"],
    platform_types = "//flight-shared-types:platform",
    target_platforms = [
        "dreamcast",
        "v6r-small",
        "v6r-medium",
        "web"
    ],
    capability_requirements = "enhanced",
)
```

### ORAS Publishing

```bash
# Publish platform-specific types
./tools/publish-platform-matrix.sh

# Pull platform types for specific platform
oras pull oras.birb.homes/flight/platform/dreamcast:1.0.0

# Pull universal platform matrix
oras pull oras.birb.homes/flight/platform/matrix:1.0.0
```

## Performance Characteristics

### Memory Scaling Performance

- **Dreamcast (16MB)**: <1ms platform detection, minimal overhead
- **V6R Small (512MB)**: <5ms platform detection, enhanced features
- **V6R Large (2GB+)**: <10ms platform detection, full feature set

### Type Safety Guarantees

- **Compile-time Platform Validation**: All platform constraints checked at build time
- **Runtime Feature Detection**: Safe feature availability checking
- **Cross-language Compatibility**: WIT-based type safety across all languages

### Build Performance

- **Platform Matrix Generation**: <30 seconds for all 12 platforms
- **ORAS Publishing**: <2 minutes for complete platform matrix
- **Buck2 Integration**: Zero overhead for platform-specific builds

## Error Handling

### Platform Detection Errors

```typescript
const result = await detector.detectPlatform();
if (!result.ok) {
  switch (result.error.id) {
    case 'platform-unknown':
      // Fallback to safe defaults
      break;
    case 'memory-detection-failed':
      // Use conservative memory estimates
      break;
    case 'capability-detection-failed':
      // Assume minimal capabilities
      break;
  }
}
```

### Compatibility Validation

```typescript
const compatibility = await matrix.checkCompatibility('dreamcast', 'v6r-large');
compatibility.issues.forEach(issue => {
  if (issue.severity === 'blocking') {
    throw new Error(`Cannot deploy to v6r-large: ${issue.description}`);
  } else if (issue.severity === 'major') {
    console.warn(`Performance impact: ${issue.description}`);
  }
});
```

## Migration Guide

### From Legacy Platform Detection

```typescript
// Before: Manual platform detection
if (navigator.userAgent.includes('Dreamcast')) {
  useMinimalFeatures();
} else if (window.v6r) {
  useEnhancedFeatures();
}

// After: Type-safe platform detection
const platform = await detector.detectPlatform();
if (platform.ok) {
  switch (platform.value.capability) {
    case PlatformCapability.Minimal:
      useMinimalFeatures();
      break;
    case PlatformCapability.Enhanced:
    case PlatformCapability.Full:
      useEnhancedFeatures();
      break;
  }
}
```

### Integration with Existing Systems

1. **Add Platform Types Dependency**
   ```bash
   npm install @flight-core/platform-types
   ```

2. **Initialize Platform Detection**
   ```typescript
   const detector = createPlatformDetector();
   ```

3. **Adapt Existing Code**
   - Replace manual platform checks with capability queries
   - Use memory adaptation patterns for resource allocation
   - Implement graceful degradation based on platform features

## Best Practices

### Platform-Agnostic Development

1. **Always Check Capabilities**: Don't assume features are available
2. **Graceful Degradation**: Provide fallbacks for missing features
3. **Memory Awareness**: Adapt memory usage to platform constraints
4. **Performance Adaptation**: Scale algorithms based on platform capabilities

### V6R Cloud Optimization

1. **Right-size VMs**: Use platform detection to select optimal VM size
2. **Auto-scaling**: Enable auto-scaling for variable workloads
3. **Resource Monitoring**: Use platform extensions for monitoring integration
4. **Cost Optimization**: Match VM capabilities to actual requirements

### Cross-Language Consistency

1. **Use WIT Types**: Stick to WIT-generated types for maximum compatibility
2. **Version Alignment**: Keep all language bindings on same version
3. **Test Matrix**: Validate functionality across all target platforms
4. **Documentation**: Document platform-specific behavior clearly

## Troubleshooting

### Common Issues

1. **Platform Not Detected**
   - Check platform registry has platform definition
   - Verify detection logic for environment

2. **Memory Constraints Violated**
   - Review memory adaptation logic
   - Check platform memory limits

3. **Feature Not Available**
   - Verify feature is supported on target platform
   - Implement appropriate fallback

4. **Build Failures**
   - Check Buck2 platform target configuration
   - Verify ORAS dependencies are available

### Debug Tools

```typescript
// Enable debug logging
const detector = createPlatformDetector({ debug: true });

// Get detailed platform information
const platform = await detector.detectPlatform();
console.log(JSON.stringify(platform, null, 2));

// Check all available features
Object.values(PlatformFeature).forEach(feature => {
  console.log(`${feature}: ${detector.hasFeature(feature)}`);
});
```

## Future Roadmap

### Planned Enhancements

1. **Additional Platforms**: PlayStation 5, Xbox Series X support
2. **Enhanced V6R Integration**: Deeper Kubernetes and service mesh integration
3. **Performance Profiling**: Platform-specific performance characteristics
4. **AI/ML Adaptations**: Platform-aware model optimization

### Contributing

The Flight-Core Platform Types system is designed for extensibility. To add new platforms:

1. Define platform in `platform_registry.rs`
2. Add WIT type definitions
3. Generate language bindings
4. Update ORAS publishing matrix
5. Add integration tests

## Conclusion

The Flight-Core Platform Types system provides a robust, type-safe foundation for cross-platform development across Flight-Core's diverse platform matrix. By leveraging this system, teams can build applications that seamlessly scale from constrained retro gaming platforms to modern cloud environments while maintaining optimal performance and user experience.
