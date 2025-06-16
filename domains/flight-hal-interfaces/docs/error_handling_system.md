# Flight HAL Error Handling System

## Overview

The Flight HAL implements a comprehensive, zero-allocation error handling system designed for embedded platforms including the Dreamcast. The system provides Rust-inspired Result<T,E> types with union-based storage, comprehensive error categorization, and efficient propagation patterns.

## Key Features

### ✅ Zero Allocation Guarantee
- Union-based `HALResult<T,E>` storage
- Static string literals for error messages
- No dynamic memory allocation in error paths
- Fixed-size, stack-allocated error types

### ✅ Rich Error Information
- Structured error categories (Hardware, Driver, Configuration, Resource, Platform, Network, Validation, Internal)
- Specific error codes within categories
- Context strings for additional debugging information
- Source location tracking capabilities

### ✅ Monadic Error Handling
- Functional-style operations (`map`, `and_then`, `map_error`)
- Pattern matching with `HAL_MATCH`
- Early return macros for clean error propagation
- Chaining operations with automatic error handling

### ✅ Performance Optimized
- Move semantics throughout
- Minimal overhead in success paths
- Compile-time optimizations
- Branch prediction hints for error paths

## Core Components

### 1. HALError (`hal_error.hpp`)

```cpp
class HALError {
    HALErrorCategory category_;
    uint32_t code_;
    const char* message_;    // Static strings only
    const char* context_;    // Additional context
};

// Predefined error factories
namespace errors {
    constexpr HALError device_not_found(uint32_t code, const char* context = nullptr);
    constexpr HALError out_of_memory(uint32_t code, const char* context = nullptr);
    // ... more error types
}
```

### 2. HALResult (`hal_result.hpp`)

```cpp
template<typename T, typename E = HALError>
class HALResult {
    union Storage {
        T value;
        E error;
    } storage_;
    bool has_value_;
    
public:
    // Factory methods
    static HALResult success(T&& value);
    static HALResult error(E&& error);
    
    // Status checking
    bool is_ok() const noexcept;
    bool is_err() const noexcept;
    
    // Value access
    T& value() &;
    T&& value() &&;
    E& error() &;
    E&& error() &&;
    
    // Monadic operations
    template<typename F> auto map(F&& func) -> HALResult<...>;
    template<typename F> auto and_then(F&& func) -> ...;
    template<typename F> auto map_error(F&& func) -> HALResult<...>;
};

// Type aliases
using HALVoidResult = HALResult<void, HALError>;
```

### 3. Error Propagation Macros (`hal_error_macros.hpp`)

```cpp
// Early return for void operations
HAL_TRY_VOID(some_void_operation());

// Condition checking with automatic error return
HAL_ENSURE_HAL(ptr != nullptr, HALErrorCategory::Validation, 1, "Null pointer");

// Return error immediately
HAL_RETURN_HAL_ERROR(HALErrorCategory::Hardware, 42, "Device failed");

// Conditional execution
HAL_IF_OK(some_operation()) {
    // Handle success case
}

HAL_IF_ERR(some_operation()) {
    // Handle error case: __hal_result.error()
}

// Pattern matching
HAL_MATCH(some_operation())
    .on_success([](auto&& value) { /* handle success */ })
    .on_error([](auto&& error) { /* handle error */ });
```

### 4. Logging Integration (`hal_logging.hpp`)

```cpp
// Automatic error logging
HAL_LOG_ERROR(error);
HAL_LOG_WARNING(error);

// RAII error tracking
auto tracked = HAL_TRACK_ERRORS(some_operation());
// Automatically logs unhandled errors

// Configurable log levels
HALLogger::set_log_level(LogLevel::Warning);
```

## Usage Patterns

### Basic Error Handling

```cpp
HALResult<uint32_t> read_register(uint32_t address) {
    HAL_ENSURE_HAL(address < 0x1000, HALErrorCategory::Configuration, 1, "Invalid address");
    
    if (hardware_failure()) {
        return HALResult<uint32_t>::error(
            errors::device_busy(2, "Hardware timeout"));
    }
    
    return HALResult<uint32_t>::success(read_hw_register(address));
}
```

### Error Propagation

```cpp
HALVoidResult initialize_system() {
    HAL_TRY_VOID(initialize_hardware());
    HAL_TRY_VOID(load_configuration());
    HAL_TRY_VOID(start_services());
    
    return HALVoidResult::success();
}
```

### Functional Style

```cpp
auto result = get_device_id()
    .map([](auto id) { return format_id(id); })
    .and_then([](auto formatted) { return validate_id(formatted); })
    .map_error([](auto error) { return add_context(error, "device_init"); });
```

### Pattern Matching

```cpp
HAL_MATCH(connect_to_device())
    .on_success([](auto&& device) {
        HAL_LOG_MESSAGE(LogLevel::Info, "Device connected");
        return configure_device(device);
    })
    .on_error([](auto&& error) {
        HAL_LOG_ERROR(error);
        return fallback_configuration();
    });
```

## Error Categories

| Category | Description | Example Errors |
|----------|-------------|----------------|
| **Hardware** | Physical device failures | Device not found, initialization failed, hardware timeout |
| **Driver** | Driver compatibility issues | Driver incompatible, feature not supported, driver not loaded |
| **Configuration** | Invalid parameters/settings | Invalid parameter, configuration missing, parameter out of range |
| **Resource** | Resource exhaustion | Out of memory, resource exhausted, resource locked |
| **Platform** | Platform-specific constraints | Platform not supported, platform constraint violation |
| **Network** | Connectivity issues | Connection failed, network timeout |
| **Validation** | Input validation failures | Validation failed, invalid state |
| **Internal** | Internal HAL errors | Internal HAL error, feature not implemented |

## Performance Characteristics

### Memory Usage
- `HALResult<uint32_t>`: ~40 bytes (union + bool + padding)
- `HALError`: ~32 bytes (category + code + 2 pointers)
- No dynamic allocation or heap usage
- All storage is stack-based

### Runtime Performance
- Success path: Near-zero overhead
- Error path: Single branch + move operation
- Move semantics eliminate unnecessary copies
- Compile-time optimization for release builds

## Platform Compatibility

### Supported Platforms
- ✅ Dreamcast (SH4, limited memory)
- ✅ Modern consoles (PS5, Xbox Series X)
- ✅ PC (Windows, Linux, macOS)
- ✅ Embedded systems (ARM Cortex-M)

### Compiler Requirements
- C++17 or later
- Support for `constexpr`, `std::variant` concepts
- Union-based storage support
- Template metaprogramming

## Migration Guide

### From Traditional Error Codes

**Before:**
```cpp
int read_data(uint32_t address, uint32_t* out_data) {
    if (!out_data) return ERR_INVALID_PARAM;
    if (address >= 0x1000) return ERR_OUT_OF_RANGE;
    
    *out_data = read_hw(address);
    return SUCCESS;
}
```

**After:**
```cpp
HALResult<uint32_t> read_data(uint32_t address) {
    HAL_ENSURE_HAL(address < 0x1000, HALErrorCategory::Configuration, 1, "Address out of range");
    
    return HALResult<uint32_t>::success(read_hw(address));
}
```

### From Exceptions

**Before:**
```cpp
uint32_t read_data(uint32_t address) {
    if (address >= 0x1000) {
        throw std::invalid_argument("Address out of range");
    }
    return read_hw(address);
}
```

**After:**
```cpp
HALResult<uint32_t> read_data(uint32_t address) {
    HAL_ENSURE_HAL(address < 0x1000, HALErrorCategory::Configuration, 1, "Address out of range");
    
    return HALResult<uint32_t>::success(read_hw(address));
}
```

## Best Practices

### 1. Use Appropriate Error Categories
```cpp
// Hardware issues
return errors::device_not_found(1, "GPU device");

// Configuration problems  
return errors::invalid_parameter(2, "buffer_size");

// Resource exhaustion
return errors::out_of_memory(3, "texture_cache");
```

### 2. Provide Context Information
```cpp
return errors::initialization_failed(1, "display_driver_v2.1");
```

### 3. Use Monadic Chains for Complex Operations
```cpp
return load_config()
    .and_then([](auto config) { return validate_config(config); })
    .and_then([](auto config) { return apply_config(config); })
    .map_error([](auto error) { return add_context(error, "system_init"); });
```

### 4. Leverage Pattern Matching for Readable Code
```cpp
HAL_MATCH(initialize_device())
    .on_success([](auto&& device) {
        // Success path
    })
    .on_error([](auto&& error) {
        // Error handling
    });
```

### 5. Use RAII Error Tracking for Development
```cpp
{
    auto tracked = HAL_TRACK_ERRORS(risky_operation());
    // Error automatically logged if unhandled
}
```

## Testing and Validation

The error handling system includes comprehensive testing:

- **Unit Tests**: Individual component validation
- **Integration Tests**: Cross-component error propagation
- **Performance Tests**: Zero-allocation verification
- **Platform Tests**: Embedded system compatibility
- **Stress Tests**: Error handling under load

## Future Enhancements

### Planned Features
- Stack trace capture for debug builds
- Error telemetry integration
- Custom error serialization
- Error recovery strategies
- Async error handling patterns

### Extensibility Points
- Custom error categories
- Platform-specific error types  
- Integration with external logging systems
- Error transformation pipelines

---

The Flight HAL error handling system provides a robust, efficient foundation for reliable operation across all target platforms while maintaining the performance characteristics required for real-time applications.
