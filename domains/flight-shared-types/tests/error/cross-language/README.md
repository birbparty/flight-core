# Cross-Language Error Handling Tests

## Overview

This directory contains comprehensive cross-language error handling tests that ensure **zero information loss** and consistent error behavior across TypeScript, Go, Rust, and C++17 bindings in the Flight-Core ecosystem.

## Test Coverage

### 🎯 **Production-Grade Error Validation**
- **Error Serialization**: Round-trip JSON serialization with complete field preservation
- **Cross-Language Compatibility**: Consistent error handling across all supported languages
- **Platform-Specific Errors**: Dreamcast (16MB) vs V6R (2GB+) constraint validation
- **Performance Testing**: Large error collection handling and serialization performance
- **Edge Cases**: Unicode characters, malformed JSON, null field handling

### 📋 **Test Files**

#### TypeScript Tests
- **`typescript-cross-language-serialization.test.ts`**
  - Complete FlightError serialization/deserialization
  - FlightResult<T> error propagation patterns
  - Platform-specific error handling (Dreamcast vs V6R)
  - Service/Platform/Network error code preservation
  - Error context enrichment chains
  - Analytics and health data serialization
  - Performance testing with 1000+ error collections
  - Unicode and special character handling

#### Go Tests
- **`go-error-serialization_test.go`**
  - Basic cross-language error serialization using standard library
  - Null/nil field handling across language boundaries
  - Platform-specific error constraints
  - Performance testing with collections
  - Unicode character preservation
  - JSON marshaling/unmarshaling validation

#### C++17 Tests
- **`cpp17-cross-language-error-handling_test.cpp`**
  - Modern C++17 features: `std::variant`, `std::optional`
  - FlightResult<T> using `std::variant` patterns
  - Optional field handling with `std::nullopt`
  - Platform-specific error handling (Dreamcast/V6R)
  - Error context enrichment and chaining
  - Performance testing with move semantics
  - Unicode and special character support
  - JSON serialization using nlohmann::json

### 🔧 **Key Features Tested**

#### Error Structure Validation
```typescript
interface FlightError {
  readonly id: string;
  readonly severity: ErrorSeverity;
  readonly category: ErrorCategory;
  readonly message: string;
  readonly details?: string;
  readonly context: ErrorContext;
  readonly timestamp: number;
  readonly cause?: string;
}
```

#### Cross-Language Result Types
- **TypeScript**: `FlightResult<T> = { tag: "ok"; val: T } | { tag: "err"; val: FlightError }`
- **Go**: `FlightResult[T] struct { Value *T; Error *FlightError }`
- **C++17**: `FlightResult<T> = std::variant<T, FlightError>`
- **Rust**: `Result<T, FlightError>` (standard Rust Result)

#### Platform-Specific Error Scenarios

##### Dreamcast (16MB Memory Constraint)
```json
{
  "context": {
    "platform": "dreamcast",
    "metadata": [
      ["total_memory_bytes", "16777216"],
      ["available_memory_bytes", "4194304"],
      ["hardware_arch", "sh4"]
    ]
  }
}
```

##### V6R Cloud (2GB+ Memory)
```json
{
  "context": {
    "platform": "v6r-large", 
    "metadata": [
      ["vm_memory_limit_bytes", "2147483648"],
      ["scaling_available", "true"],
      ["auto_scale_threshold", "90.0"]
    ]
  }
}
```

#### Error Code Mapping
- **Service Error Codes**: 12 standardized codes (auth-failed, rate-limit-exceeded, etc.)
- **Platform Error Codes**: 7 platform-specific codes (insufficient-memory, feature-not-supported, etc.)
- **Network Error Codes**: 10 network-related codes (connection-timeout, tls-error, etc.)

### 🚀 **Performance Requirements**

All tests validate:
- **Serialization**: < 1 second for 1000+ errors
- **Deserialization**: < 500ms for 1000+ errors
- **Memory Usage**: Linear scaling with error context size
- **Zero Information Loss**: All fields preserved across language boundaries

### 🌍 **Unicode and Special Character Support**

Tests validate handling of:
- **Emoji**: 🚀🔥💻🎮🎯🌍
- **Multi-language text**: 日本語, 中文, English, Français
- **Special characters**: `\n\t\r\"\\`
- **Session/User IDs**: `unicode-session-🎯`, `user-émile-测试`

### 🔗 **Error Chaining and Context Enrichment**

Tests validate error propagation through language chains:
```
TypeScript → Go → Rust → C++17
```

With metadata accumulation:
```json
{
  "metadata": [
    ["language_layer", "typescript"],
    ["ts_processing_time_ms", "120"],
    ["language_layer", "go"],
    ["go_processing_time_ms", "75"],
    ["language_layer", "rust"], 
    ["rust_processing_time_ms", "25"],
    ["language_layer", "c++17"],
    ["cpp_processing_time_ms", "10"],
    ["total_chain_time_ms", "230"]
  ]
}
```

## Running Tests

### Prerequisites

#### TypeScript
```bash
# Install dependencies (if using jest/testing framework)
npm install --save-dev @types/jest jest ts-jest

# Run TypeScript tests
npm test tests/error/cross-language/typescript-cross-language-serialization.test.ts
```

#### Go
```bash
# Run Go tests (uses standard library only)
go test ./tests/error/cross-language/go-error-serialization_test.go
```

#### C++17
```bash
# Install dependencies
# - Google Test (gtest)
# - nlohmann/json

# Build and run with CMake
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON
make
./cpp17_cross_language_error_test
```

#### Rust
```bash
# Add to validation/error/Cargo.toml dependencies
cargo test --package flight_error_validation
```

## Integration with Flight-Core

### Error Type Consistency

All tests ensure errors can flow seamlessly between:
- **Flight-Core** (C++17 native runtime)
- **V6R Services** (TypeScript/Go cloud services)  
- **Component Model** (WASM with Rust validation)
- **Language Bindings** (Generated from WIT definitions)

### Platform Compatibility

Tests validate error handling across:
- **Dreamcast**: 16MB memory, SH4 architecture
- **PSP**: 32MB memory, MIPS architecture  
- **Vita**: 512MB memory, ARM architecture
- **V6R Small**: 512MB VM memory
- **V6R Medium**: 1GB VM memory
- **V6R Large**: 2GB+ VM memory

### Service Integration

Error codes map to real service scenarios:
- **Authentication failures** → `auth-failed`
- **Rate limiting** → `rate-limit-exceeded`
- **Resource exhaustion** → `resource-limit-exceeded`
- **Network timeouts** → `connection-timeout`
- **Platform constraints** → `insufficient-platform-memory`

## Architecture Benefits

### Zero Information Loss
- All error fields preserved across language boundaries
- Context metadata maintains debugging information
- Error chains preserve causality relationships

### Performance Optimized
- JSON serialization tuned for production workloads
- Memory usage scales linearly with error context
- Cross-language overhead minimized

### Production Ready
- Comprehensive edge case handling
- Unicode and internationalization support
- Platform-specific constraint validation
- Analytics and health monitoring integration

## Future Enhancements

### Planned Additions
- **Rust Integration Tests**: Native Result<T, FlightError> patterns
- **WASM Component Tests**: Error handling across WASM boundaries
- **Performance Benchmarks**: Cross-language error handling latency
- **Analytics Integration**: Real-time error monitoring tests
- **Recovery Strategy Tests**: Automated error recovery validation

### Monitoring Integration
- Error analytics with trending and prediction
- System health assessment based on error patterns
- Recovery suggestion automation
- Cross-language performance monitoring

---

## Summary

These tests ensure **production-grade error handling** across the entire Flight ecosystem, from resource-constrained retro platforms (Dreamcast) to modern cloud environments (V6R), with **zero information loss** and consistent behavior across all supported programming languages.
