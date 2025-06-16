# Flight HAL Code Generator

The Flight HAL Code Generator is a powerful tool for automatically generating HAL driver implementations, tests, configuration files, and documentation from interface definitions. This tool accelerates HAL development by automating boilerplate code creation while ensuring consistency and compliance with HAL patterns.

## Features

- **Interface Stub Generation**: Complete driver implementation skeletons
- **Test Skeleton Creation**: Compliance and unit test frameworks
- **Configuration Automation**: CMakeLists.txt and build configuration files
- **Documentation Generation**: Comprehensive driver documentation
- **Platform Customization**: Support for multiple target platforms
- **Template-Based**: Easily customizable generation templates

## Quick Start

### Building the Generator

```bash
# From the Flight HAL project root
cd tools/interface_generator
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Basic Usage

```bash
# Generate a Windows memory driver
./hal_generator -t driver -i memory -p windows -c WindowsMemoryDriver -o ./output

# Generate test skeleton for graphics interface
./hal_generator -t test -i graphics -o ./tests

# Generate configuration files
./hal_generator -t config -i audio -p linux -c LinuxAudioDriver -o ./build
```

## Command Line Interface

### Options

| Option | Description | Default |
|--------|-------------|---------|
| `-t, --type TYPE` | Generation type (driver\|test\|config\|doc) | driver |
| `-i, --interface IFACE` | Interface name | Required |
| `-p, --platform PLAT` | Target platform | generic |
| `-c, --class CLASS` | Generated class name | Auto-generated |
| `-n, --namespace NS` | Target namespace | Auto-generated |
| `-o, --output DIR` | Output directory | Required |
| `--templates DIR` | Templates directory | ./templates |
| `--interfaces DIR` | Interfaces directory | ../../include/flight/hal/interfaces |
| `--auto-register` | Include auto-registration | true |
| `--examples` | Include example code | false |
| `--documentation` | Include documentation | true |
| `-h, --help` | Show help message | |
| `-v, --version` | Show version information | |

### Supported Interfaces

- `memory` - Memory management interface
- `graphics` - Graphics rendering interface
- `audio` - Audio processing interface
- `input` - Input device interface
- `file` - File system interface
- `network` - Network communication interface
- `time` - Time and timing interface
- `thread` - Threading and synchronization interface
- `performance` - Performance monitoring interface

### Supported Platforms

- `windows` - Microsoft Windows
- `linux` - Linux distributions
- `macos` - Apple macOS
- `embedded` - Embedded systems
- `dreamcast` - Sega Dreamcast
- `generic` - Platform-agnostic

## Generation Types

### Driver Implementation (`driver`)

Generates a complete driver implementation including:
- Header file with class declaration
- Implementation file with method stubs
- Platform-specific initialization
- Auto-registration support
- Error handling integration
- Thread-safe implementation patterns

Example:
```bash
hal_generator -t driver -i memory -p windows -c WindowsMemoryDriver -o ./drivers/windows
```

### Test Skeleton (`test`)

Generates comprehensive test frameworks including:
- Google Test-based test suites
- Compliance test patterns
- Mock implementations
- Performance tests
- Thread safety tests
- Integration test skeletons

Example:
```bash
hal_generator -t test -i graphics -o ./tests/graphics
```

### Configuration Files (`config`)

Generates build and configuration files including:
- CMakeLists.txt with platform detection
- Package configuration files
- Installation scripts
- Dependency management
- Compiler configuration

Example:
```bash
hal_generator -t config -i audio -p linux -c LinuxAudioDriver -o ./build/linux
```

### Documentation (`doc`)

Generates comprehensive documentation including:
- Driver usage guides
- API reference documentation
- Platform-specific notes
- Installation instructions
- Troubleshooting guides
- Example code snippets

Example:
```bash
hal_generator -t doc -i input -p macos -c MacOSInputDriver -o ./docs
```

## Templates

The code generator uses a template-based system for maximum flexibility. Templates are located in the `templates/` directory:

```
templates/
├── driver_templates/
│   ├── driver_header.hpp.template
│   └── driver_impl.cpp.template
├── test_templates/
│   └── test_skeleton.cpp.template
├── config_templates/
│   └── CMakeLists.txt.template
└── doc_templates/
    └── interface_doc.md.template
```

### Template Variables

Templates support variable substitution using `{{VARIABLE}}` syntax:

- `{{CLASS_NAME}}` - Generated class name
- `{{INTERFACE_NAME}}` - Interface class name
- `{{PLATFORM}}` - Target platform name
- `{{PLATFORM_UPPER}}` - Uppercase platform name
- `{{NAMESPACE}}` - Target namespace
- `{{METHODS}}` - Generated method signatures
- `{{DATE}}` - Current date and time
- `{{INCLUDE_GUARD}}` - Include guard macro

### Template Functions

Templates also support function calls using `{{function(args)}}` syntax:

- `{{upper(string)}}` - Convert to uppercase
- `{{lower(string)}}` - Convert to lowercase
- `{{camel(string)}}` - Convert to camelCase
- `{{snake(string)}}` - Convert to snake_case
- `{{include_guard(filename)}}` - Generate include guard

## Examples

### Generating a Complete Driver

```bash
# Generate a comprehensive Linux graphics driver
hal_generator -t driver -i graphics -p linux -c LinuxGraphicsDriver \
  --namespace flight::hal::drivers::linux \
  --examples --documentation \
  -o ./drivers/linux/graphics

# This creates:
# - LinuxGraphicsDriver.hpp
# - LinuxGraphicsDriver.cpp
# - Complete implementation stubs
# - Platform-specific optimizations
```

### Generating Test Suites

```bash
# Generate comprehensive tests for memory interface
hal_generator -t test -i memory -o ./tests/memory

# This creates:
# - memory_test.cpp
# - Mock implementations
# - Compliance tests
# - Performance tests
# - Thread safety tests
```

### Generating Project Configuration

```bash
# Generate complete build configuration
hal_generator -t config -i audio -p windows -c WindowsAudioDriver \
  -o ./build/windows/audio

# This creates:
# - CMakeLists.txt
# - Package configuration files
# - Installation scripts
# - Platform-specific settings
```

## Integration with Flight HAL

### Auto-Registration

Generated drivers automatically register with the HAL system:

```cpp
// Auto-generated registration code
#ifdef FLIGHT_PLATFORM_WINDOWS
REGISTER_HAL_DRIVER(flight::hal::IMemoryInterface, 
                    flight::hal::drivers::windows::WindowsMemoryDriver);
#endif
```

### Error Handling

Generated code follows HAL error handling patterns:

```cpp
HALResult<void> WindowsMemoryDriver::initialize() {
    if (initialized_) {
        return HALError::AlreadyInitialized;
    }
    
    // Platform-specific initialization
    
    initialized_ = true;
    return HALResult<void>::success();
}
```

### Thread Safety

Generated drivers include thread-safe implementations:

```cpp
class WindowsMemoryDriver : public IMemoryInterface {
private:
    mutable std::mutex state_mutex_;
    // Thread-safe member access
};
```

## Customization

### Custom Templates

Create custom templates for specific needs:

1. Copy existing templates to a new directory
2. Modify templates as needed
3. Use `--templates` option to specify custom template directory

### Platform Extensions

Add support for new platforms:

1. Update `PlatformAdapter::get_platform_variables()`
2. Add platform-specific includes and capabilities
3. Create platform-specific templates if needed

### Interface Extensions

Add support for new interfaces:

1. Place interface header in interfaces directory
2. Ensure proper interface naming convention (`I*Interface`)
3. Generator automatically detects new interfaces

## Best Practices

### Driver Development

1. **Start with Generation**: Always start with generated stubs
2. **Incremental Implementation**: Implement methods incrementally
3. **Test Early**: Use generated tests to verify functionality
4. **Platform Optimization**: Leverage platform-specific features
5. **Error Handling**: Follow HAL error handling patterns

### Template Maintenance

1. **Version Control**: Keep templates in version control
2. **Testing**: Test template changes thoroughly
3. **Documentation**: Document custom template variables
4. **Consistency**: Maintain consistency across templates

## Troubleshooting

### Common Issues

1. **Template Not Found**
   - Check template directory path
   - Verify template file naming

2. **Interface Not Detected**
   - Ensure interface header is in correct directory
   - Verify interface naming convention

3. **Generation Failures**
   - Check output directory permissions
   - Verify required parameters are provided

### Debug Mode

Enable verbose output for debugging:

```bash
hal_generator -t driver -i memory -p linux -c LinuxMemoryDriver \
  --output ./debug --templates ./templates --interfaces ./interfaces
```

## Contributing

To contribute to the code generator:

1. Fork the repository
2. Create feature branch
3. Implement changes
4. Add tests for new functionality
5. Update documentation
6. Submit pull request

### Code Style

- Follow C++20 standards
- Use RAII for resource management
- Comprehensive error handling
- Extensive documentation

## License

This tool is part of the Flight HAL project and is licensed under the same terms.

---

*The Flight HAL Code Generator - Accelerating HAL development through intelligent automation.*
