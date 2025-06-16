#!/bin/bash

# Flight Shared Types - WebAssembly Artifacts Publishing to oras.birb.homes
# Based on ORAS Registry Verification - PASSED ✅ (June 5, 2025)

set -e

# Configuration
REGISTRY="oras.birb.homes"
NAMESPACE="flight/shared-types"
VERSION="${FLIGHT_VERSION:-$(date +%Y.%m.%d)}"
BUILD_DATE=$(date -u +"%Y-%m-%dT%H:%M:%SZ")

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo "======================================================================"
echo "Flight Shared Types - WebAssembly Artifacts Publishing"
echo "======================================================================"
echo "Registry: ${REGISTRY}"
echo "Namespace: ${NAMESPACE}"
echo "Version: ${VERSION}"
echo "Build Date: ${BUILD_DATE}"
echo "======================================================================"

# Ensure required tools are available
check_dependencies() {
    echo -e "${BLUE}Checking dependencies...${NC}"
    
    if ! command -v oras &> /dev/null; then
        echo -e "${RED}✗${NC} ORAS CLI not found. Install with: brew install oras"
        exit 1
    fi
    
    if ! command -v cargo &> /dev/null; then
        echo -e "${RED}✗${NC} Cargo not found. Install Rust toolchain."
        exit 1
    fi
    
    if ! command -v wasm-pack &> /dev/null; then
        echo -e "${YELLOW}⚠${NC} wasm-pack not found. Installing..."
        cargo install wasm-pack
    fi
    
    echo -e "${GREEN}✓${NC} All dependencies available"
}

# Build WebAssembly components from Rust bindings
build_wasm_components() {
    echo -e "${BLUE}Building WebAssembly components...${NC}"
    
    # Create build directory
    mkdir -p build/wasm
    
    # Build complete Rust bindings as WASM
    echo "Building flight-shared-types-complete.wasm..."
    cd bindings/rust/complete
    
    # Add wasm32-unknown-unknown target if not present
    rustup target add wasm32-unknown-unknown
    
    # Build optimized WASM
    cargo build --target wasm32-unknown-unknown --release
    
    # Copy WASM file to build directory
    cp target/wasm32-unknown-unknown/release/flight_shared_types_complete.wasm \
       ../../../build/wasm/flight-shared-types-complete.wasm
    
    cd ../../..
    
    # Build individual validation components
    for component in memory error platform session authentication; do
        echo "Building flight-${component}-validation.wasm..."
        cd validation/${component}
        
        cargo build --target wasm32-unknown-unknown --release
        cp target/wasm32-unknown-unknown/release/flight_${component}_validation.wasm \
           ../../build/wasm/flight-${component}-validation.wasm
        
        cd ../..
    done
    
    echo -e "${GREEN}✓${NC} WebAssembly components built successfully"
}

# Create platform-specific artifact bundles
create_platform_bundles() {
    echo -e "${BLUE}Creating platform-specific bundles...${NC}"
    
    mkdir -p build/bundles
    
    # Platform configurations
    declare -A PLATFORMS=(
        ["dreamcast"]="16MB single-threaded"
        ["vita"]="512MB multi-threaded" 
        ["v6r-small"]="128MB dual-threaded"
        ["v6r-medium"]="512MB quad-threaded"
        ["v6r-large"]="2GB+ octa-threaded"
    )
    
    for platform in "${!PLATFORMS[@]}"; do
        echo "Creating bundle for ${platform}..."
        
        bundle_dir="build/bundles/${platform}"
        mkdir -p "${bundle_dir}"
        
        # Copy WASM components
        cp build/wasm/*.wasm "${bundle_dir}/"
        
        # Copy WIT files
        cp wit/*.wit "${bundle_dir}/"
        
        # Create platform manifest
        cat > "${bundle_dir}/platform-manifest.json" << EOF
{
  "platform": "${platform}",
  "description": "${PLATFORMS[$platform]}",
  "version": "${VERSION}",
  "build_date": "${BUILD_DATE}",
  "components": [
    "flight-shared-types-complete.wasm",
    "flight-memory-validation.wasm",
    "flight-error-validation.wasm",
    "flight-platform-validation.wasm",
    "flight-session-validation.wasm",
    "flight-authentication-validation.wasm"
  ],
  "interfaces": [
    "memory-types.wit",
    "error.wit", 
    "platform.wit",
    "session.wit",
    "authentication.wit",
    "realtime.wit",
    "pagination.wit",
    "component.wit"
  ],
  "flight": {
    "project": "flight",
    "category": "shared-types",
    "team": "shared-types",
    "target_platform": "${platform}"
  }
}
EOF
        
        # Create platform bundle archive
        cd "${bundle_dir}"
        tar -czf "../${platform}-bundle.tar.gz" .
        cd ../../..
        
        echo -e "${GREEN}✓${NC} Created bundle for ${platform}"
    done
}

# Package language bindings
package_language_bindings() {
    echo -e "${BLUE}Packaging language bindings...${NC}"
    
    mkdir -p build/bindings
    
    # TypeScript bindings
    echo "Packaging TypeScript bindings..."
    tar -czf build/bindings/typescript-bindings.tar.gz bindings/typescript/
    
    # Go bindings  
    echo "Packaging Go bindings..."
    tar -czf build/bindings/go-bindings.tar.gz bindings/go/
    
    # Rust bindings
    echo "Packaging Rust bindings..."
    tar -czf build/bindings/rust-bindings.tar.gz bindings/rust/
    
    # C++17 bindings
    echo "Packaging C++17 bindings..."
    tar -czf build/bindings/cpp17-bindings.tar.gz bindings/cpp17/
    
    echo -e "${GREEN}✓${NC} Language bindings packaged"
}

# Publish WebAssembly components
publish_wasm_components() {
    echo -e "${BLUE}Publishing WebAssembly components...${NC}"
    
    # Publish complete WASM component
    echo "Publishing flight-shared-types-complete.wasm..."
    oras push ${REGISTRY}/${NAMESPACE}/wasm/complete:${VERSION} \
        build/wasm/flight-shared-types-complete.wasm:application/vnd.wasm.content.layer.v1+wasm \
        --annotation "flight.project=flight" \
        --annotation "flight.category=shared-types" \
        --annotation "flight.component=complete" \
        --annotation "flight.version=${VERSION}" \
        --annotation "flight.build-date=${BUILD_DATE}" \
        --annotation "org.opencontainers.image.source=https://github.com/flight/flight"
    
    # Publish validation components
    for component in memory error platform session authentication; do
        echo "Publishing flight-${component}-validation.wasm..."
        oras push ${REGISTRY}/${NAMESPACE}/wasm/${component}:${VERSION} \
            build/wasm/flight-${component}-validation.wasm:application/vnd.wasm.content.layer.v1+wasm \
            --annotation "flight.project=flight" \
            --annotation "flight.category=shared-types" \
            --annotation "flight.component=${component}" \
            --annotation "flight.version=${VERSION}" \
            --annotation "flight.build-date=${BUILD_DATE}"
    done
    
    echo -e "${GREEN}✓${NC} WebAssembly components published"
}

# Publish platform bundles
publish_platform_bundles() {
    echo -e "${BLUE}Publishing platform bundles...${NC}"
    
    for platform in dreamcast vita v6r-small v6r-medium v6r-large; do
        echo "Publishing ${platform} bundle..."
        oras push ${REGISTRY}/${NAMESPACE}/bundles/${platform}:${VERSION} \
            build/bundles/${platform}-bundle.tar.gz:application/tar+gzip \
            --annotation "flight.project=flight" \
            --annotation "flight.category=shared-types" \
            --annotation "flight.target-platform=${platform}" \
            --annotation "flight.version=${VERSION}" \
            --annotation "flight.build-date=${BUILD_DATE}" \
            --annotation "flight.bundle-type=platform-complete"
        
        echo -e "${GREEN}✓${NC} Published ${platform} bundle"
    done
}

# Publish language bindings
publish_language_bindings() {
    echo -e "${BLUE}Publishing language bindings...${NC}"
    
    # TypeScript bindings
    echo "Publishing TypeScript bindings..."
    oras push ${REGISTRY}/${NAMESPACE}/bindings/typescript:${VERSION} \
        build/bindings/typescript-bindings.tar.gz:application/tar+gzip \
        --annotation "flight.project=flight" \
        --annotation "flight.category=shared-types" \
        --annotation "flight.language=typescript" \
        --annotation "flight.version=${VERSION}" \
        --annotation "flight.build-date=${BUILD_DATE}"
    
    # Go bindings
    echo "Publishing Go bindings..."
    oras push ${REGISTRY}/${NAMESPACE}/bindings/go:${VERSION} \
        build/bindings/go-bindings.tar.gz:application/tar+gzip \
        --annotation "flight.project=flight" \
        --annotation "flight.category=shared-types" \
        --annotation "flight.language=go" \
        --annotation "flight.version=${VERSION}" \
        --annotation "flight.build-date=${BUILD_DATE}"
    
    # Rust bindings
    echo "Publishing Rust bindings..."
    oras push ${REGISTRY}/${NAMESPACE}/bindings/rust:${VERSION} \
        build/bindings/rust-bindings.tar.gz:application/tar+gzip \
        --annotation "flight.project=flight" \
        --annotation "flight.category=shared-types" \
        --annotation "flight.language=rust" \
        --annotation "flight.version=${VERSION}" \
        --annotation "flight.build-date=${BUILD_DATE}"
    
    # C++17 bindings
    echo "Publishing C++17 bindings..."
    oras push ${REGISTRY}/${NAMESPACE}/bindings/cpp17:${VERSION} \
        build/bindings/cpp17-bindings.tar.gz:application/tar+gzip \
        --annotation "flight.project=flight" \
        --annotation "flight.category=shared-types" \
        --annotation "flight.language=cpp17" \
        --annotation "flight.cpp.standard=17" \
        --annotation "flight.version=${VERSION}" \
        --annotation "flight.build-date=${BUILD_DATE}"
    
    echo -e "${GREEN}✓${NC} Language bindings published"
}

# Publish WIT interface definitions
publish_wit_interfaces() {
    echo -e "${BLUE}Publishing WIT interface definitions...${NC}"
    
    # Create WIT bundle
    mkdir -p build/wit
    tar -czf build/wit/wit-interfaces.tar.gz wit/
    
    echo "Publishing WIT interfaces..."
    oras push ${REGISTRY}/${NAMESPACE}/interfaces/wit:${VERSION} \
        build/wit/wit-interfaces.tar.gz:application/tar+gzip \
        --annotation "flight.project=flight" \
        --annotation "flight.category=shared-types" \
        --annotation "flight.artifact-type=wit-interfaces" \
        --annotation "flight.component-model=true" \
        --annotation "flight.version=${VERSION}" \
        --annotation "flight.build-date=${BUILD_DATE}"
    
    echo -e "${GREEN}✓${NC} WIT interfaces published"
}

# Create and publish master manifest
publish_master_manifest() {
    echo -e "${BLUE}Creating master manifest...${NC}"
    
    mkdir -p build/manifest
    
    cat > build/manifest/flight-shared-types-manifest.json << EOF
{
  "name": "flight-shared-types",
  "version": "${VERSION}",
  "build_date": "${BUILD_DATE}",
  "registry": "${REGISTRY}",
  "namespace": "${NAMESPACE}",
  "description": "Flight Core Shared Types - Complete language bindings and WebAssembly components",
  "artifacts": {
    "wasm_components": {
      "complete": "${REGISTRY}/${NAMESPACE}/wasm/complete:${VERSION}",
      "memory": "${REGISTRY}/${NAMESPACE}/wasm/memory:${VERSION}",
      "error": "${REGISTRY}/${NAMESPACE}/wasm/error:${VERSION}",
      "platform": "${REGISTRY}/${NAMESPACE}/wasm/platform:${VERSION}",
      "session": "${REGISTRY}/${NAMESPACE}/wasm/session:${VERSION}",
      "authentication": "${REGISTRY}/${NAMESPACE}/wasm/authentication:${VERSION}"
    },
    "platform_bundles": {
      "dreamcast": "${REGISTRY}/${NAMESPACE}/bundles/dreamcast:${VERSION}",
      "vita": "${REGISTRY}/${NAMESPACE}/bundles/vita:${VERSION}",
      "v6r-small": "${REGISTRY}/${NAMESPACE}/bundles/v6r-small:${VERSION}",
      "v6r-medium": "${REGISTRY}/${NAMESPACE}/bundles/v6r-medium:${VERSION}",
      "v6r-large": "${REGISTRY}/${NAMESPACE}/bundles/v6r-large:${VERSION}"
    },
    "language_bindings": {
      "typescript": "${REGISTRY}/${NAMESPACE}/bindings/typescript:${VERSION}",
      "go": "${REGISTRY}/${NAMESPACE}/bindings/go:${VERSION}",
      "rust": "${REGISTRY}/${NAMESPACE}/bindings/rust:${VERSION}",
      "cpp17": "${REGISTRY}/${NAMESPACE}/bindings/cpp17:${VERSION}"
    },
    "interfaces": {
      "wit": "${REGISTRY}/${NAMESPACE}/interfaces/wit:${VERSION}"
    }
  },
  "supported_platforms": [
    "dreamcast", "vita", "v6r-small", "v6r-medium", "v6r-large",
    "windows", "macos", "linux"
  ],
  "supported_languages": [
    "typescript", "go", "rust", "cpp17"
  ],
  "component_model": {
    "version": "0.2.0",
    "wit_bindgen_compatible": true,
    "wasm_component_model": true
  },
  "flight": {
    "project": "flight",
    "category": "shared-types",
    "team": "shared-types"
  }
}
EOF
    
    echo "Publishing master manifest..."
    oras push ${REGISTRY}/${NAMESPACE}/manifest:${VERSION} \
        build/manifest/flight-shared-types-manifest.json:application/json \
        --annotation "flight.project=flight" \
        --annotation "flight.category=shared-types" \
        --annotation "flight.artifact-type=master-manifest" \
        --annotation "flight.version=${VERSION}" \
        --annotation "flight.build-date=${BUILD_DATE}"
    
    # Also tag as latest
    oras push ${REGISTRY}/${NAMESPACE}/manifest:latest \
        build/manifest/flight-shared-types-manifest.json:application/json \
        --annotation "flight.project=flight" \
        --annotation "flight.category=shared-types" \
        --annotation "flight.artifact-type=master-manifest" \
        --annotation "flight.version=${VERSION}" \
        --annotation "flight.build-date=${BUILD_DATE}"
    
    echo -e "${GREEN}✓${NC} Master manifest published"
}

# Verify published artifacts
verify_published_artifacts() {
    echo -e "${BLUE}Verifying published artifacts...${NC}"
    
    # Check registry catalog
    echo "Checking registry catalog..."
    oras repo ls ${REGISTRY}/${NAMESPACE}
    
    # Verify specific artifacts
    artifacts=(
        "wasm/complete:${VERSION}"
        "bundles/dreamcast:${VERSION}"
        "bindings/typescript:${VERSION}"
        "interfaces/wit:${VERSION}"
        "manifest:${VERSION}"
    )
    
    for artifact in "${artifacts[@]}"; do
        echo "Verifying ${artifact}..."
        if oras manifest fetch ${REGISTRY}/${NAMESPACE}/${artifact} > /dev/null; then
            echo -e "${GREEN}✓${NC} ${artifact} verified"
        else
            echo -e "${RED}✗${NC} ${artifact} verification failed"
        fi
    done
    
    echo -e "${GREEN}✓${NC} Artifact verification complete"
}

# Main execution
main() {
    check_dependencies
    build_wasm_components
    create_platform_bundles
    package_language_bindings
    publish_wasm_components
    publish_platform_bundles
    publish_language_bindings
    publish_wit_interfaces
    publish_master_manifest
    verify_published_artifacts
    
    echo "======================================================================"
    echo -e "${GREEN}🎉 ALL ARTIFACTS PUBLISHED SUCCESSFULLY!${NC}"
    echo "======================================================================"
    echo "Registry: https://${REGISTRY}"
    echo "Namespace: ${NAMESPACE}"
    echo "Version: ${VERSION}"
    echo ""
    echo "Access published artifacts:"
    echo "• Master manifest: oras pull ${REGISTRY}/${NAMESPACE}/manifest:latest"
    echo "• WASM complete: oras pull ${REGISTRY}/${NAMESPACE}/wasm/complete:${VERSION}"
    echo "• Platform bundles: oras pull ${REGISTRY}/${NAMESPACE}/bundles/{platform}:${VERSION}"
    echo "• Language bindings: oras pull ${REGISTRY}/${NAMESPACE}/bindings/{language}:${VERSION}"
    echo ""
    echo "Browse at: https://${REGISTRY}/v2/_catalog"
    echo "======================================================================"
}

# Execute main function
main "$@"
