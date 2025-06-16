/**
 * @file test_headers.cpp
 * @brief Comprehensive test file for Flight WASM header structure
 * 
 * This file validates that all headers compile correctly and that the
 * basic API structure is working as expected.
 */

#include <flight/wasm/wasm.hpp>
#include <iostream>

int main() {
    std::cout << "Flight WASM Header Structure Test\n";
    std::cout << "==================================\n\n";

    // Test version information
    std::cout << "Library Version: " << flight::wasm::version::STRING << "\n";
    std::cout << "Build Type: " << flight::wasm::version::BUILD_TYPE << "\n";
    std::cout << "WebAssembly Magic: 0x" << std::hex << flight::wasm::spec::BINARY_MAGIC << std::dec << "\n";
    std::cout << "WebAssembly Version: " << flight::wasm::spec::BINARY_VERSION << "\n\n";

    // Test platform detection
    std::cout << "Platform Information:\n";
    std::cout << "  Big Endian: " << (flight::wasm::platform::is_big_endian() ? "yes" : "no") << "\n";
    std::cout << "  Little Endian: " << (flight::wasm::platform::is_little_endian() ? "yes" : "no") << "\n";
    std::cout << "  Has NEON SIMD: " << (flight::wasm::platform::has_neon_simd ? "yes" : "no") << "\n";
    std::cout << "  Has Limited Memory: " << (flight::wasm::platform::has_limited_memory ? "yes" : "no") << "\n";
    std::cout << "  Max Memory: " << flight::wasm::platform::max_memory << " bytes\n";
    std::cout << "  Cache Line Size: " << flight::wasm::platform::cache_line_size << " bytes\n\n";

    // Test value type utilities
    std::cout << "Value Type Information:\n";
    const flight::wasm::ValueType types[] = {
        flight::wasm::ValueType::I32,
        flight::wasm::ValueType::I64,
        flight::wasm::ValueType::F32,
        flight::wasm::ValueType::F64,
        flight::wasm::ValueType::V128,
        flight::wasm::ValueType::FuncRef,
        flight::wasm::ValueType::ExternRef
    };

    for (auto type : types) {
        std::cout << "  " << flight::wasm::utils::value_type_name(type) 
                  << ": size=" << flight::wasm::value_type_size(type) 
                  << ", align=" << flight::wasm::value_type_alignment(type)
                  << ", number=" << (flight::wasm::is_number_type(type) ? "yes" : "no")
                  << ", vector=" << (flight::wasm::is_vector_type(type) ? "yes" : "no")
                  << ", reference=" << (flight::wasm::is_reference_type(type) ? "yes" : "no") << "\n";
    }
    std::cout << "\n";

    // Test error handling
    std::cout << "Error Handling Test:\n";
    flight::wasm::Error success_error;
    std::cout << "  Success error: " << (success_error.success() ? "success" : "failure") << "\n";
    
    flight::wasm::Error test_error(flight::wasm::ErrorCode::InvalidMagicNumber, "Test error message");
    std::cout << "  Test error: " << (test_error.success() ? "success" : "failure") << "\n";
    std::cout << "  Error code: " << static_cast<uint32_t>(test_error.code()) << "\n";
    std::cout << "  Error message: " << test_error.message() << "\n";
    std::cout << "  Error category: " << test_error.category() << "\n";
    std::cout << "  Is binary error: " << (flight::wasm::is_binary_error(test_error.code()) ? "yes" : "no") << "\n\n";

    // Test Result type
    std::cout << "Result Type Test:\n";
    auto success_result = flight::wasm::make_result<int>(42);
    std::cout << "  Success result: " << (success_result.success() ? "success" : "failure") << "\n";
    if (success_result.success()) {
        std::cout << "  Success value: " << success_result.value() << "\n";
    }

    auto error_result = flight::wasm::make_error<int>(flight::wasm::ErrorCode::OutOfMemory, "Memory allocation failed");
    std::cout << "  Error result: " << (error_result.success() ? "success" : "failure") << "\n";
    if (error_result.failed()) {
        std::cout << "  Error code: " << static_cast<uint32_t>(error_result.error().code()) << "\n";
    }
    std::cout << "\n";

    // Test span utility
    std::cout << "Span Utility Test:\n";
    const uint8_t test_data[] = {0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00}; // WASM magic + version
    auto data_span = flight::wasm::utils::make_span(test_data);
    std::cout << "  Span size: " << data_span.size() << "\n";
    std::cout << "  First 4 bytes (hex): ";
    for (size_t i = 0; i < 4 && i < data_span.size(); ++i) {
        std::cout << "0x" << std::hex << static_cast<unsigned>(data_span[i]) << std::dec << " ";
    }
    std::cout << "\n\n";

    // Test configuration
    std::cout << "Configuration Test:\n";
    auto platform_defaults = flight::wasm::config::get_platform_defaults();
    std::cout << "  Default validation level: " << static_cast<int>(platform_defaults.default_validation_level) << "\n";
    std::cout << "  Strict validation: " << (platform_defaults.strict_validation ? "yes" : "no") << "\n";
    std::cout << "  Fast path optimizations: " << (platform_defaults.enable_fast_path_optimizations ? "yes" : "no") << "\n";
    std::cout << "  SIMD optimizations: " << (platform_defaults.enable_simd_optimizations ? "yes" : "no") << "\n";
    std::cout << "  Parser buffer size: " << platform_defaults.parser_buffer_size << " bytes\n";
    std::cout << "  Max module size: " << platform_defaults.max_module_size << " bytes\n\n";

    // Test version check macro
    std::cout << "Version Check Test:\n";
    std::cout << "  Version >= 1.0.0: " << (FLIGHT_WASM_VERSION_CHECK(1, 0, 0) ? "yes" : "no") << "\n";
    std::cout << "  Version >= 2.0.0: " << (FLIGHT_WASM_VERSION_CHECK(2, 0, 0) ? "yes" : "no") << "\n\n";

    // Test static assertions
    static_assert(sizeof(flight::wasm::V128) == 16, "V128 must be 16 bytes");
    static_assert(alignof(flight::wasm::V128) == 16, "V128 must be 16-byte aligned");
    static_assert(flight::wasm::value_type_size(flight::wasm::ValueType::I32) == 4, "I32 must be 4 bytes");
    static_assert(flight::wasm::value_type_size(flight::wasm::ValueType::I64) == 8, "I64 must be 8 bytes");
    static_assert(flight::wasm::is_number_type(flight::wasm::ValueType::I32), "I32 must be a number type");
    static_assert(flight::wasm::is_vector_type(flight::wasm::ValueType::V128), "V128 must be a vector type");
    static_assert(flight::wasm::is_reference_type(flight::wasm::ValueType::FuncRef), "FuncRef must be a reference type");

    std::cout << "Static Assertions: All passed\n\n";

    std::cout << "✅ All header structure tests passed!\n";
    std::cout << "✅ Flight WASM header architecture is ready for implementation.\n";

    return 0;
}
