#!/bin/bash
# Test ORAS Publishing for Flight Platform Types
# Verifies connectivity and publishing to oras.birb.homes

set -euo pipefail

echo "🧪 Testing ORAS Publishing to oras.birb.homes..."

REGISTRY="oras.birb.homes"
NAMESPACE="flight/shared-types"
VERSION="test-$(date +%Y%m%d-%H%M%S)"

# Test platforms for verification
TEST_PLATFORMS=("dreamcast" "v6r-small" "web")

echo "📋 Testing with platforms: ${TEST_PLATFORMS[*]}"
echo "🏷️  Test version: $VERSION"
echo ""

# Create temporary directory for test artifacts
TEMP_DIR=$(mktemp -d)
trap "rm -rf $TEMP_DIR" EXIT

echo "📦 Creating test platform type artifacts..."

# Create basic platform type test artifact
mkdir -p "$TEMP_DIR/platform-types"

# Create test TypeScript types
cat > "$TEMP_DIR/platform-types/types.ts" << 'EOF'
// Test Flight Platform Types
export enum PlatformCapability {
  Minimal = 'minimal',
  Enhanced = 'enhanced',
  Full = 'full',
}

export interface PlatformInfo {
  id: string;
  name: string;
  capability: PlatformCapability;
}
EOF

# Create test package.json
cat > "$TEMP_DIR/platform-types/package.json" << EOF
{
  "name": "@flight/platform-types-test",
  "version": "$VERSION",
  "description": "Test platform types for ORAS publishing verification",
  "main": "types.ts"
}
EOF

# Create test README
cat > "$TEMP_DIR/platform-types/README.md" << EOF
# Flight Platform Types Test Package

This is a test package to verify ORAS publishing functionality.

- Version: $VERSION
- Registry: $REGISTRY
- Namespace: $NAMESPACE
- Created: $(date)

## Platforms Tested
$(printf -- "- %s\n" "${TEST_PLATFORMS[@]}")
EOF

# Create manifest with metadata
cat > "$TEMP_DIR/platform-types/manifest.json" << EOF
{
  "registry": "$REGISTRY",
  "namespace": "$NAMESPACE", 
  "version": "$VERSION",
  "platforms": [$(printf '"%s",' "${TEST_PLATFORMS[@]}" | sed 's/,$//')]
,
  "test": true,
  "timestamp": "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
}
EOF

echo "✅ Test artifacts created in: $TEMP_DIR"

# Test publishing for each platform
for platform in "${TEST_PLATFORMS[@]}"; do
    echo ""
    echo "🔄 Testing publish for platform: $platform"
    
    # Create platform-specific directory
    PLATFORM_DIR="$TEMP_DIR/platform-$platform"
    mkdir -p "$PLATFORM_DIR"
    
    # Copy base artifacts
    cp -r "$TEMP_DIR/platform-types"/* "$PLATFORM_DIR/"
    
    # Add platform-specific metadata
    echo "export const PLATFORM_ID = '$platform';" >> "$PLATFORM_DIR/types.ts"
    
    # Create tarball
    tar -czf "$TEMP_DIR/platform-$platform.tar.gz" -C "$PLATFORM_DIR" .
    
    echo "📦 Created package: platform-$platform.tar.gz ($(du -h "$TEMP_DIR/platform-$platform.tar.gz" | cut -f1))"
    
    # Attempt to push to ORAS
    echo "🚀 Publishing to: $REGISTRY/$NAMESPACE/platform-$platform:$VERSION"
    
    if oras push "$REGISTRY/$NAMESPACE/platform-$platform:$VERSION" \
        "$TEMP_DIR/platform-$platform.tar.gz:application/vnd.flight.platform.types" \
        --insecure \
        --disable-path-validation \
        --annotation "flight.project=flight" \
        --annotation "flight.category=shared-types" \
        --annotation "flight.platform=$platform" \
        --annotation "flight.test=true" \
        --annotation "org.opencontainers.image.created=$(date -u +%Y-%m-%dT%H:%M:%SZ)" \
        --annotation "org.opencontainers.image.description=Flight Platform Types for $platform (test)"; then
        
        echo "✅ Successfully published $platform platform types"
        
        # Test immediate pull back
        echo "🔍 Testing pull verification..."
        PULL_DIR="$TEMP_DIR/pull-test-$platform"
        mkdir -p "$PULL_DIR"
        
        if oras pull "$REGISTRY/$NAMESPACE/platform-$platform:$VERSION" \
            --output "$PULL_DIR" \
            --insecure; then
            echo "✅ Successfully pulled back $platform package"
            
            # Verify content
            if [ -f "$PULL_DIR/types.ts" ] && [ -f "$PULL_DIR/manifest.json" ]; then
                echo "✅ Package contents verified"
            else
                echo "❌ Package contents missing"
            fi
        else
            echo "❌ Failed to pull back $platform package"
        fi
        
    else
        echo "❌ Failed to publish $platform platform types"
        echo "   This could be due to authentication or permissions"
    fi
done

# Test universal platform matrix package
echo ""
echo "🌐 Testing universal platform matrix package..."

UNIVERSAL_DIR="$TEMP_DIR/platform-matrix"
mkdir -p "$UNIVERSAL_DIR"

# Copy all platform artifacts
cp -r "$TEMP_DIR/platform-types"/* "$UNIVERSAL_DIR/"

# Add universal metadata
cat >> "$UNIVERSAL_DIR/manifest.json" << EOF_APPEND
,
  "universal": true,
  "all_platforms": [$(printf '"%s",' "${TEST_PLATFORMS[@]}" | sed 's/,$//')]

EOF_APPEND

# Create universal tarball
tar -czf "$TEMP_DIR/platform-matrix.tar.gz" -C "$UNIVERSAL_DIR" .

echo "📦 Created universal package: platform-matrix.tar.gz ($(du -h "$TEMP_DIR/platform-matrix.tar.gz" | cut -f1))"

if oras push "$REGISTRY/$NAMESPACE/platform-matrix:$VERSION" \
    "$TEMP_DIR/platform-matrix.tar.gz:application/vnd.flight.platform.matrix" \
    --insecure \
    --disable-path-validation \
    --annotation "flight.project=flight" \
    --annotation "flight.category=shared-types" \
    --annotation "flight.type=platform-matrix" \
    --annotation "flight.test=true" \
    --annotation "org.opencontainers.image.created=$(date -u +%Y-%m-%dT%H:%M:%SZ)" \
    --annotation "org.opencontainers.image.description=Flight Platform Matrix (test)"; then
    
    echo "✅ Successfully published universal platform matrix"
else
    echo "❌ Failed to publish universal platform matrix"
fi

echo ""
echo "📊 Testing repository listing..."
if oras repo ls "$REGISTRY/$NAMESPACE" --insecure; then
    echo "✅ Repository listing successful"
else
    echo "❌ Repository listing failed"
fi

echo ""
echo "🏷️  Testing tag listing..."
for platform in "${TEST_PLATFORMS[@]}"; do
    echo "Tags for platform-$platform:"
    if oras repo tags "$REGISTRY/$NAMESPACE/platform-$platform" --insecure 2>/dev/null | head -5; then
        echo "✅ Tag listing successful for $platform"
    else
        echo "❌ Tag listing failed for $platform"
    fi
done

echo ""
echo "✅ ORAS Publishing Test Complete!"
echo ""
echo "📋 Summary:"
echo "  • Registry: $REGISTRY"
echo "  • Namespace: $NAMESPACE"
echo "  • Test Version: $VERSION"
echo "  • Platforms Tested: ${TEST_PLATFORMS[*]}"
echo ""
echo "🧹 Cleanup:"
echo "  Test packages can be removed with:"
echo "  oras repo rm $REGISTRY/$NAMESPACE/platform-{dreamcast,v6r-small,web}:$VERSION --insecure"
echo "  oras repo rm $REGISTRY/$NAMESPACE/platform-matrix:$VERSION --insecure"
