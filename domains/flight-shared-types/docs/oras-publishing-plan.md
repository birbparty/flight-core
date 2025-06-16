# Flight Shared Types - ORAS Publishing Plan

## Overview

This document outlines the comprehensive plan for publishing Flight Shared Types WebAssembly artifacts and language bindings to the production ORAS registry at `https://oras.birb.homes`.

## 🏗️ **Platform Infrastructure Status**

Based on verification from birb-home documentation:

✅ **ORAS Registry**: `https://oras.birb.homes` (Zot v2.1.3)  
✅ **SSL Certificate**: Valid Let's Encrypt with auto-renewal  
✅ **Performance**: 94% success rate in comprehensive testing  
✅ **Security**: Rate limiting, HTTPS-only, network policies  
✅ **Storage**: 50GB persistent storage on DOKS cluster  
✅ **Monitoring**: Prometheus metrics, web UI, CVE scanning  

**Registry Capabilities Verified:**
- ✅ WASM module support (`application/vnd.wasm.content.layer.v1+wasm`)
- ✅ Archive bundles (`application/tar+gzip`)
- ✅ JSON manifests (`application/json`)
- ✅ Generic artifacts with annotations
- ✅ Repository discovery and catalog browsing

## 📦 **Artifact Publishing Structure**

### Registry Namespace
**Base URL**: `oras.birb.homes/flight/shared-types`

### Artifact Categories

| Category | Purpose | Example URL |
|----------|---------|-------------|
| **WASM Components** | WebAssembly modules | `flight/shared-types/wasm/complete:2025.06.06` |
| **Platform Bundles** | Target-specific packages | `flight/shared-types/bundles/dreamcast:2025.06.06` |
| **Language Bindings** | Source code packages | `flight/shared-types/bindings/typescript:2025.06.06` |
| **WIT Interfaces** | Component Model definitions | `flight/shared-types/interfaces/wit:2025.06.06` |
| **Master Manifest** | Discovery and metadata | `flight/shared-types/manifest:latest` |

## 🔧 **Build and Publishing Pipeline**

### Phase 1: WebAssembly Component Generation

**Source**: Rust bindings in `bindings/rust/complete/`  
**Target**: `wasm32-unknown-unknown`  
**Output**: Optimized `.wasm` files

```bash
# Complete shared types WASM
cargo build --target wasm32-unknown-unknown --release
# Output: flight-shared-types-complete.wasm

# Individual validation components  
cargo build --target wasm32-unknown-unknown --release
# Output: flight-{memory,error,platform,session,authentication}-validation.wasm
```

### Phase 2: Platform-Specific Bundles

**Platforms Supported:**
- **Dreamcast**: 16MB, single-threaded, basic components only
- **PlayStation Vita**: 512MB, multi-threaded, full feature set
- **V6R Small**: 128MB, dual-threaded, efficient serialization
- **V6R Medium**: 512MB, quad-threaded, balanced features
- **V6R Large**: 2GB+, octa-threaded, full performance optimization

**Bundle Contents:**
- All applicable WASM components
- Platform-specific WIT interfaces
- Platform manifest with constraints
- Configuration templates

### Phase 3: Language Binding Packages

**Languages**: TypeScript, Go, Rust, C++17  
**Format**: Compressed archives (`tar.gz`)  
**Contents**: Complete source trees with build configurations

### Phase 4: Component Model Interfaces

**Format**: WIT (WebAssembly Interface Types)  
**Files**: All `.wit` definitions from `wit/` directory  
**Purpose**: Cross-language component interoperability

## 🚀 **Publishing Process**

### Automated Publishing Script

**Script**: `tools/publish-wasm-artifacts.sh`  
**Execution**: `./tools/publish-wasm-artifacts.sh`  
**Dependencies**: ORAS CLI, Rust toolchain, wasm-pack

### Publishing Workflow

1. **Dependency Check**
   - Verify ORAS CLI installation
   - Confirm Rust toolchain with WASM target
   - Install wasm-pack if needed

2. **Build Phase**
   - Compile all Rust bindings to WASM
   - Create platform-specific bundles
   - Package language bindings
   - Generate manifests and metadata

3. **Publishing Phase**
   - Push WASM components with proper media types
   - Upload platform bundles with annotations
   - Publish language binding archives
   - Deploy WIT interface definitions
   - Create master manifest for discovery

4. **Verification Phase**
   - Confirm artifact availability
   - Validate manifest integrity
   - Test repository browsing
   - Generate usage examples

## 📋 **Artifact Metadata and Annotations**

### Standard Annotations

All artifacts include standardized OCI annotations:

```json
{
  "flight.project": "flight",
  "flight.category": "shared-types", 
  "flight.team": "shared-types",
  "flight.version": "2025.06.06",
  "flight.build-date": "2025-06-06T23:49:18Z",
  "org.opencontainers.image.source": "https://github.com/birbparty/flight"
}
```

### Component-Specific Annotations

**WASM Components:**
```json
{
  "flight.component": "complete",
  "flight.wasm.component-model": "true",
  "flight.wasm.wit-bindgen": "compatible"
}
```

**Platform Bundles:**
```json
{
  "flight.target-platform": "dreamcast",
  "flight.bundle-type": "platform-complete",
  "flight.memory-limit": "16MB",
  "flight.cpu-threads": "1"
}
```

**Language Bindings:**
```json
{
  "flight.language": "typescript",
  "flight.cpp.standard": "17",
  "flight.binding-type": "source-package"
}
```

## 🔍 **Discovery and Usage**

### Master Manifest

**Location**: `oras.birb.homes/flight/shared-types/manifest:latest`

**Contents**: Complete artifact catalog with URLs, metadata, and usage instructions

### Artifact Discovery

```bash
# List all Flight shared types repositories
oras repo ls oras.birb.homes/flight/shared-types

# Get master manifest
oras pull oras.birb.homes/flight/shared-types/manifest:latest

# Browse specific categories
oras repo ls oras.birb.homes/flight/shared-types/wasm
oras repo ls oras.birb.homes/flight/shared-types/bundles
oras repo ls oras.birb.homes/flight/shared-types/bindings
```

### Web UI Access

**Registry Browser**: `https://oras.birb.homes`  
**Search**: Available for all published artifacts  
**Metadata View**: Complete annotation display  

## 👥 **Team Usage Patterns**

### Development Teams

**Pull Complete Language Bindings:**
```bash
# Get TypeScript bindings
oras pull oras.birb.homes/flight/shared-types/bindings/typescript:latest

# Get Go bindings  
oras pull oras.birb.homes/flight/shared-types/bindings/go:latest

# Get C++17 bindings
oras pull oras.birb.homes/flight/shared-types/bindings/cpp17:latest
```

### Platform Teams

**Pull Platform-Specific Bundles:**
```bash
# Dreamcast development
oras pull oras.birb.homes/flight/shared-types/bundles/dreamcast:latest

# PlayStation Vita development
oras pull oras.birb.homes/flight/shared-types/bundles/vita:latest

# V6R deployment
oras pull oras.birb.homes/flight/shared-types/bundles/v6r-medium:latest
```

### Component Model Integration

**Pull WIT Interfaces:**
```bash
# Get all WIT definitions
oras pull oras.birb.homes/flight/shared-types/interfaces/wit:latest

# Pull individual WASM components
oras pull oras.birb.homes/flight/shared-types/wasm/complete:latest
oras pull oras.birb.homes/flight/shared-types/wasm/memory:latest
```

## 🔄 **CI/CD Integration**

### GitHub Actions Integration

```yaml
- name: Pull Flight Shared Types
  run: |
    oras pull oras.birb.homes/flight/shared-types/bindings/typescript:latest
    tar -xzf typescript-bindings.tar.gz
```

### Docker Integration

```dockerfile
# Pull WASM components in Dockerfile
RUN oras pull oras.birb.homes/flight/shared-types/wasm/complete:latest
COPY flight-shared-types-complete.wasm /app/wasm/
```

### Rust Cargo Integration

```toml
# Reference in Cargo.toml (future enhancement)
[dependencies]
flight-shared-types = { registry = "oras.birb.homes" }
```

## 📊 **Versioning Strategy**

### Version Format
**Pattern**: `YYYY.MM.DD` (date-based versioning)  
**Example**: `2025.06.06`  
**Latest Tag**: Always points to most recent version

### Release Cadence
- **Major Updates**: Monthly (new features, breaking changes)
- **Minor Updates**: Weekly (bug fixes, optimizations)  
- **Patch Updates**: As needed (critical fixes)

### Retention Policy
- **Latest**: Always available
- **Last 12 months**: Full retention
- **Historical**: Major versions only

## 🛡️ **Security and Access Control**

### Current Configuration
- **Authentication**: Open registry (no auth required)
- **HTTPS**: Required for all operations
- **Rate Limiting**: 50 requests/second, 100 burst
- **Network Access**: Public internet access

### Future Security Enhancements
- **Team-based Access**: RBAC for sensitive artifacts
- **Package Signing**: Cosign integration for artifact verification
- **Vulnerability Scanning**: Automated security scanning
- **Audit Logging**: Comprehensive access logging

## 📈 **Monitoring and Metrics**

### Registry Health
- **Availability**: 99.9% uptime target
- **Response Time**: <100ms for catalog operations
- **Storage Usage**: Monitor against 50GB limit
- **Transfer Metrics**: Track download patterns

### Usage Analytics
- **Popular Artifacts**: Track most-downloaded components
- **Team Usage**: Monitor per-team consumption
- **Platform Distribution**: Analyze platform preferences
- **Version Adoption**: Track version upgrade patterns

## 🔧 **Maintenance and Operations**

### Regular Maintenance
- **Storage Cleanup**: Remove old versions beyond retention policy
- **Performance Monitoring**: Registry response time optimization
- **SSL Certificate**: Auto-renewal verification
- **Backup Strategy**: Include in registry backup procedures

### Disaster Recovery
- **Registry Backup**: Daily backup of artifact metadata
- **Storage Replication**: Cross-region storage replication
- **Rebuild Process**: Automated artifact regeneration
- **Documentation**: Runbook for registry restoration

## 🎯 **Success Metrics**

### Technical Metrics
- **Artifact Availability**: 99.9%+ uptime
- **Download Success Rate**: >95%
- **Build Success Rate**: >90%
- **Registry Performance**: <100ms response time

### Adoption Metrics  
- **Team Onboarding**: Number of teams using artifacts
- **Download Volume**: Daily/weekly download counts
- **Platform Coverage**: Usage across all target platforms
- **Integration Success**: CI/CD pipeline adoption

## 🚀 **Rollout Plan**

### Phase 1: Initial Publishing (Week 1)
- ✅ Registry setup and verification complete
- 🔄 **IN PROGRESS**: Publish initial artifact set
- 📝 Create usage documentation
- 👥 Notify development teams

### Phase 2: Team Onboarding (Week 2-3)
- 🎓 Team training sessions
- 🔧 CI/CD integration examples
- 📖 Best practices documentation
- 🐛 Issue tracking and resolution

### Phase 3: Production Optimization (Week 4+)
- 📊 Performance monitoring and optimization
- 🔒 Security enhancements
- 🤖 Automation improvements
- 📈 Usage analytics implementation

## 📞 **Support and Contact**

### Registry Issues
- **Registry Status**: Monitor at `https://oras.birb.homes/health`
- **Platform Team**: Contact for infrastructure issues
- **Documentation**: This guide and birb-home repository

### Artifact Issues
- **Flight Shared Types Team**: For binding and WASM component issues
- **GitHub Issues**: flight-core repository issue tracker
- **Validation**: Use `tools/validate-all-bindings.sh`

---

## 🎉 **Ready for Production**

The Flight Shared Types ORAS publishing infrastructure is **production-ready** and provides:

- ✅ **Comprehensive Artifact Support**: WASM, bundles, bindings, interfaces
- ✅ **Platform Coverage**: Dreamcast, Vita, V6R variants, modern platforms  
- ✅ **Language Support**: TypeScript, Go, Rust, C++17
- ✅ **Enterprise Features**: SSL, monitoring, rate limiting, backup
- ✅ **Developer Experience**: Easy discovery, CLI tools, documentation
- ✅ **CI/CD Integration**: Automated workflows, versioning, validation

**Next Step**: Execute `./tools/publish-wasm-artifacts.sh` to begin publishing!
