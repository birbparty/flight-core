#!/bin/bash
# WIT validation tool for Flight Shared Types

set -euo pipefail

WIT_DIR="/Users/punk1290/git/flight/domains/flight-shared-types/wit"
TEMP_DIR="/tmp/flight-wit-validation"

echo "🔍 Validating WIT files..."

for wit_file in "$WIT_DIR"/*.wit; do
    if [ -f "$wit_file" ]; then
        echo "Validating $(basename "$wit_file")..."
        
        # Create temp directory
        mkdir -p "$TEMP_DIR"
        
        # Test WIT compilation
        if wit-bindgen rust "$wit_file" --out-dir "$TEMP_DIR" 2>/dev/null; then
            echo "✅ $(basename "$wit_file") - Valid"
        else
            echo "❌ $(basename "$wit_file") - Invalid"
            exit 1
        fi
        
        # Cleanup
        rm -rf "$TEMP_DIR"
    fi
done

echo "✅ All WIT files validated"
