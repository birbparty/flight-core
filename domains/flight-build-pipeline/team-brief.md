# Flight Build Pipeline Team Brief

## 🎯 Mission

You are building the complete build toolchain that orchestrates the entire Flight Core build process - from component development to platform-specific deployable binaries. Your pipeline must support incremental builds, caching, and seamless integration of all tools while maintaining sub-second iteration times during development.

## 🚀 What We Need From You

### 1. **End-to-End Build Orchestration**
Create a unified build system that:
- Detects component changes automatically
- Triggers appropriate rebuild steps
- Manages tool dependencies and versions
- Handles platform-specific build variants
- Provides clear progress feedback

### 2. **Incremental Build System**
Implement intelligent incremental builds:
- Track file dependencies accurately
- Cache intermediate build artifacts
- Invalidate only affected components
- Support distributed caching
- Minimize rebuild times (< 1 second for single component change)

### 3. **Platform Build Targets**
Support all Flight Core platforms:
- Generate platform-specific binaries (PSX .exe, PSP EBOOT, etc.)
- Handle toolchain management per platform
- Create bootable images/ISOs
- Support cross-compilation from any host
- Manage platform SDK dependencies

### 4. **Developer Experience**
Provide excellent developer workflow:
- Single command builds (`flight build`)
- Hot reload support where possible
- Clear error messages with fix suggestions
- IDE integration (VS Code, etc.)
- Build performance analytics

### 5. **CI/CD Integration**
Enable automated builds:
- GitHub Actions workflows
- Parallel builds across platforms
- Artifact publishing
- Release packaging
- Performance regression detection

## 📋 Key Requirements

### Performance Targets
- Clean build: < 30 seconds for typical project
- Incremental: < 1 second for component change
- Hot reload: < 500ms where supported
- CI build: < 5 minutes all platforms

### Developer Experience
- Zero configuration for common cases
- Smart defaults with override capability
- Helpful error messages
- Progress visualization

### Reliability
- Reproducible builds
- Dependency version locking
- Build artifact integrity
- Graceful failure handling

## 🔧 Technical Constraints

1. **Language**: Python/Rust for build tools
2. **Build System**: CMake + custom orchestration
3. **Caching**: Local + distributed support
4. **Platforms**: Linux, macOS, Windows hosts

## 📊 Success Metrics

1. **Speed**: Sub-second incremental builds
2. **Reliability**: 99.9% build success rate
3. **Adoption**: Used by all Flight Core developers
4. **Satisfaction**: 90%+ developer happiness

## 🎮 Example Use Case

A developer working on a game:
1. Edits physics component (Rust)
2. Saves file
3. Build system detects change
4. Rebuilds only physics component
5. Flattens with cached components
6. Hot reloads into running game
7. Total time: < 1 second

## 🚨 Critical Path

Your work enables:
- **All developers** to iterate quickly
- **Platform teams** to validate changes
- **QA teams** to test builds
- **Release management** to ship games

## 💡 Innovation Opportunities

1. **AI-Powered Optimization**: Predict build patterns
2. **Cloud Build Farm**: Distributed compilation
3. **Binary Diff Updates**: Minimal patch sizes
4. **Build Time Prediction**: Accurate ETAs

## 🏗️ Architecture Guidelines

### Build Pipeline Structure
```python
class FlightBuildPipeline:
    def __init__(self, config: BuildConfig):
        self.component_watcher = ComponentWatcher()
        self.build_graph = DependencyGraph()
        self.cache = BuildCache()
        self.platforms = PlatformRegistry()
    
    async def build(self, targets: List[str]) -> BuildResult:
        # 1. Detect changes
        changes = self.component_watcher.get_changes()
        
        # 2. Update build graph
        affected = self.build_graph.get_affected(changes)
        
        # 3. Check cache
        tasks = self.cache.filter_uncached(affected)
        
        # 4. Execute builds
        results = await self.execute_parallel(tasks)
        
        # 5. Package outputs
        return self.package_results(results)
```

### Incremental Build Cache
```yaml
# .flight-cache/build-state.yaml
components:
  physics:
    source_hash: "sha256:abc123..."
    last_modified: "2024-06-02T10:30:00Z"
    outputs:
      wasm: "cache/physics-abc123.wasm"
      metadata: "cache/physics-abc123.json"
    
  renderer:
    source_hash: "sha256:def456..."
    dependencies: ["physics"]
    outputs:
      wasm: "cache/renderer-def456.wasm"

flattened:
  hash: "sha256:combined123..."
  components: ["physics", "renderer"]
  output: "cache/game-combined123.wasm"
```

### Platform Build Configs
```toml
# platforms/psx.toml
[platform]
name = "PlayStation"
arch = "mipsel"
memory_limit = "2MB"

[toolchain]
compiler = "mipsel-none-elf-gcc"
sdk = "PSn00bSDK"
sdk_path = "${PSNOOB_PATH}"

[build]
flags = ["-O2", "-msoft-float", "-G0"]
link_script = "psx.ld"
output_format = "psx-exe"

[packaging]
type = "iso"
boot_file = "SYSTEM.CNF"
icon = "ICON0.TIM"
```

### Build Manifest
```yaml
# flight-project.yaml
name: "My Awesome Game"
version: "1.0.0"
flight_core_version: "0.2.0"

components:
  - name: physics
    path: components/physics
    language: rust
    
  - name: renderer
    path: components/renderer
    language: cpp
    
  - name: game_logic
    path: components/game
    language: assemblyscript

targets:
  - platform: psx
    memory_layout: "conservative"
    optimizations: "size"
    
  - platform: psp
    memory_layout: "standard"
    optimizations: "balanced"

dependencies:
  wasm-tools: "1.0.0"
  flight-flatten: "0.1.0"
```

## 🤝 Collaboration

You'll work closely with:
- **Flight Component Flattener Team**: Integrate their tool
- **Flight Simple Runtime Team**: Package runtime correctly
- **Platform HAL teams**: Handle platform specifics
- **Developer Experience Team**: IDE integration

## ⚠️ Special Considerations

### Platform Quirks
1. **PSX**: Requires specific EXE header format
2. **Dreamcast**: Needs 1ST_READ.BIN bootstrap
3. **PSP**: EBOOT.PBP packaging with icons
4. **Web**: Different WASM loading mechanism

### Incremental Build Challenges
- Component boundary detection
- Cross-language dependency tracking
- Binary diff generation
- Cache invalidation strategies

### CI/CD Requirements
- Matrix builds for all platforms
- Artifact size optimization
- Release note generation
- Version tagging automation

## 📚 Key Technologies to Research

1. **Build Systems**: Bazel, Buck2, Turborepo
2. **Caching**: ccache, sccache, BuildXL
3. **File Watching**: Watchman, chokidar
4. **Task Runners**: Just, Task, Make
5. **Package Formats**: Platform-specific (ISO, PBP, etc.)

Remember: Developer iteration speed is everything. Every second saved is a better game shipped!
