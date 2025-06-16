# HAL Compliance Validator

This directory contains the HAL validation tools that provide comprehensive interface compliance checking, driver validation, and error tracking capabilities.

## Quick Start

### 1. Build the Validation Tools

```bash
# From the project root directory
mkdir build && cd build
cmake .. -DBUILD_TOOLS=ON
make hal_validator
```

### 2. Run Validation (Before Installation)

The validator executable will be built in the build directory:

```bash
# From the build directory
./tools/compliance_validator/hal_validator --help
./tools/compliance_validator/hal_validator --list-validators
./tools/compliance_validator/hal_validator --list-platforms

# Validate specific interface
./tools/compliance_validator/hal_validator --interface IMemoryInterface --platform dreamcast --verbose

# Run all validations
./tools/compliance_validator/hal_validator --all --platform psp

# Generate JSON report
./tools/compliance_validator/hal_validator --all --output json --output-file report.json
```

### 3. Install Tools (Optional)

```bash
# From the build directory
make install

# Now you can use the tools globally
hal_validator --help
hal_validate.sh validate-memory --platform dreamcast
```

## Available Commands

### Direct Tool Usage

| Command | Description |
|---------|-------------|
| `hal_validator --help` | Show help and all options |
| `hal_validator --list-validators` | List available validators |
| `hal_validator --list-platforms` | List supported platforms |
| `hal_validator --interface NAME --platform PLATFORM` | Validate specific interface |
| `hal_validator --all --platform PLATFORM` | Run all validations |
| `hal_validator --all --output json` | Generate JSON report |

### Platform-Specific Examples

```bash
# Dreamcast validation (16MB memory constraints)
./tools/compliance_validator/hal_validator --interface IMemoryInterface --platform dreamcast

# PSP validation (WiFi networking)
./tools/compliance_validator/hal_validator --interface INetworkInterface --platform psp

# Web validation (sandbox restrictions)
./tools/compliance_validator/hal_validator --interface IGraphicsInterface --platform web

# Desktop validation (high performance)
./tools/compliance_validator/hal_validator --all --platform macos --verbose
```

## Supported Platforms

- `dreamcast` - Sega Dreamcast (16MB RAM, single-threaded)
- `psp` - Sony PlayStation Portable (32-64MB RAM, WiFi)
- `web` - Web/Emscripten (sandbox restrictions, WebGL)
- `macos` - Apple macOS (high performance, full features)
- `windows` - Microsoft Windows (high performance, full features)
- `linux` - Linux (high performance, full features)

## Validation Categories

- **Interface** - Method implementation and contract compliance
- **Error Handling** - Error propagation and recovery
- **Resource Usage** - Memory and resource management
- **Threading** - Thread safety (where applicable)
- **Platform Support** - Platform-specific requirements
- **Performance** - Performance characteristics
- **Configuration** - Configuration parameter handling
- **Memory** - Memory allocation patterns

## Output Formats

### Console Output (Default)
Human-readable format with colored output and categorized results.

### JSON Output
Machine-readable format for CI/CD integration:
```bash
./tools/compliance_validator/hal_validator --all --output json --output-file validation_report.json
```

## Files in This Directory

- `hal_validator.cpp` - Main validation tool implementation
- `hal_validate.sh.in` - Convenience script template (generated during build)
- `CMakeLists.txt` - Build configuration
- `README.md` - This file

## CI/CD Integration

The validator returns standard exit codes:
- `0` - All validations passed
- `1` - Validation failures detected  
- `2` - Tool configuration error
- `3` - System error

Example CI usage:
```bash
# Build and run validation
mkdir build && cd build
cmake .. -DBUILD_TOOLS=ON
make hal_validator
./tools/compliance_validator/hal_validator --all --output json --output-file ci_report.json

# Check exit code
if [ $? -eq 0 ]; then
    echo "All validations passed"
else
    echo "Validation failures detected"
    exit 1
fi
```

## Troubleshooting

### "command not found: hal_validate.sh"
The convenience script is only available after installation:
```bash
make install  # Install tools first
hal_validate.sh validate-memory --platform dreamcast
```

Or use the direct tool:
```bash
./tools/compliance_validator/hal_validator --interface IMemoryInterface --platform dreamcast
```

### "HAL validator not found"
Make sure you built with tools enabled:
```bash
cmake .. -DBUILD_TOOLS=ON
make hal_validator
```

### "Interface instance is null"
This is expected behavior when validating without real interface implementations. The validator tests interface contracts without requiring actual hardware.

## Documentation

For complete documentation, see:
- [HAL Validation Tools Guide](../../docs/hal_validation_tools_guide.md)
- [Interface Compliance Testing Framework](../../docs/interface_compliance_testing_framework.md)
- [Example Usage](../../examples/validation_usage/validation_example.cpp)
