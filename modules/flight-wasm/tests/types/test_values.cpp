// =============================================================================
// Flight WASM Tests - Value Type System & Value Class
// WebAssembly Core Specification 1.0 Compliance Testing
// =============================================================================

#include <catch2/catch_test_macros.hpp>
#include <flight/wasm/types/values.hpp>
#include <flight/wasm/types/value.hpp>
#include <limits>
#include <chrono>

using namespace flight::wasm;

// =============================================================================
// Value Type Definition Tests
// =============================================================================

TEST_CASE("ValueType Definitions", "[types][values][spec]") {
    SECTION("Basic value type enumeration") {
        // WebAssembly Core Specification Section 2.3
        REQUIRE(static_cast<uint8_t>(ValueType::I32) == 0x7F);
        REQUIRE(static_cast<uint8_t>(ValueType::I64) == 0x7E);
        REQUIRE(static_cast<uint8_t>(ValueType::F32) == 0x7D);
        REQUIRE(static_cast<uint8_t>(ValueType::F64) == 0x7C);
        REQUIRE(static_cast<uint8_t>(ValueType::V128) == 0x7B);
        REQUIRE(static_cast<uint8_t>(ValueType::FuncRef) == 0x70);
        REQUIRE(static_cast<uint8_t>(ValueType::ExternRef) == 0x6F);
        REQUIRE(static_cast<uint8_t>(ValueType::EmptyBlockType) == 0x40);
    }
    
    SECTION("Value type size constraints") {
        // Ensure compact representation
        REQUIRE(sizeof(ValueType) == 1);
        
        // Verify all defined types fit in single byte
        REQUIRE(static_cast<uint8_t>(ValueType::I32) < 128);
        REQUIRE(static_cast<uint8_t>(ValueType::I64) < 128);
        REQUIRE(static_cast<uint8_t>(ValueType::F32) < 128);
        REQUIRE(static_cast<uint8_t>(ValueType::F64) < 128);
        REQUIRE(static_cast<uint8_t>(ValueType::V128) < 128);
    }
    
    SECTION("Value type validation") {
        // Test is_valid_value_type function
        REQUIRE(is_valid_value_type(ValueType::I32));
        REQUIRE(is_valid_value_type(ValueType::I64));
        REQUIRE(is_valid_value_type(ValueType::F32));
        REQUIRE(is_valid_value_type(ValueType::F64));
        REQUIRE(is_valid_value_type(ValueType::V128));
        REQUIRE(is_valid_value_type(ValueType::FuncRef));
        REQUIRE(is_valid_value_type(ValueType::ExternRef));
        
        // Invalid types
        REQUIRE_FALSE(is_valid_value_type(static_cast<ValueType>(0x00)));
        REQUIRE_FALSE(is_valid_value_type(static_cast<ValueType>(0xFF)));
        REQUIRE_FALSE(is_valid_value_type(ValueType::EmptyBlockType)); // Special encoding
    }
    
    SECTION("Value type classification") {
        // Numeric types
        REQUIRE(is_number_type(ValueType::I32));
        REQUIRE(is_number_type(ValueType::I64));
        REQUIRE(is_number_type(ValueType::F32));
        REQUIRE(is_number_type(ValueType::F64));
        REQUIRE_FALSE(is_number_type(ValueType::V128));
        REQUIRE_FALSE(is_number_type(ValueType::FuncRef));
        REQUIRE_FALSE(is_number_type(ValueType::ExternRef));
        
        // Vector types
        REQUIRE(is_vector_type(ValueType::V128));
        REQUIRE_FALSE(is_vector_type(ValueType::I32));
        REQUIRE_FALSE(is_vector_type(ValueType::F64));
        REQUIRE_FALSE(is_vector_type(ValueType::FuncRef));
        
        // Reference types
        REQUIRE(is_reference_type(ValueType::FuncRef));
        REQUIRE(is_reference_type(ValueType::ExternRef));
        REQUIRE_FALSE(is_reference_type(ValueType::I32));
        REQUIRE_FALSE(is_reference_type(ValueType::F64));
        REQUIRE_FALSE(is_reference_type(ValueType::V128));
    }
}

// =============================================================================
// Value Type Utility Functions
// =============================================================================

TEST_CASE("ValueType Utilities", "[types][values][utilities]") {
    SECTION("Value type sizes") {
        REQUIRE(value_type_size(ValueType::I32) == 4);
        REQUIRE(value_type_size(ValueType::I64) == 8);
        REQUIRE(value_type_size(ValueType::F32) == 4);
        REQUIRE(value_type_size(ValueType::F64) == 8);
        REQUIRE(value_type_size(ValueType::V128) == 16);
        REQUIRE(value_type_size(ValueType::FuncRef) == sizeof(void*));
        REQUIRE(value_type_size(ValueType::ExternRef) == sizeof(void*));
    }
    
    SECTION("Value type alignment") {
        REQUIRE(value_type_alignment(ValueType::I32) == 4);
        REQUIRE(value_type_alignment(ValueType::I64) == 8);
        REQUIRE(value_type_alignment(ValueType::F32) == 4);
        REQUIRE(value_type_alignment(ValueType::F64) == 8);
        REQUIRE(value_type_alignment(ValueType::V128) == 16);
        REQUIRE(value_type_alignment(ValueType::FuncRef) == alignof(void*));
        REQUIRE(value_type_alignment(ValueType::ExternRef) == alignof(void*));
    }
    
    SECTION("Value type names") {
        REQUIRE(std::string_view(value_type_name(ValueType::I32)) == "i32");
        REQUIRE(std::string_view(value_type_name(ValueType::I64)) == "i64");
        REQUIRE(std::string_view(value_type_name(ValueType::F32)) == "f32");
        REQUIRE(std::string_view(value_type_name(ValueType::F64)) == "f64");
        REQUIRE(std::string_view(value_type_name(ValueType::V128)) == "v128");
        REQUIRE(std::string_view(value_type_name(ValueType::FuncRef)) == "funcref");
        REQUIRE(std::string_view(value_type_name(ValueType::ExternRef)) == "externref");
        REQUIRE(std::string_view(value_type_name(static_cast<ValueType>(0xFF))) == "unknown");
    }
}

// =============================================================================
// V128 Vector Type Tests
// =============================================================================

TEST_CASE("V128 Vector Type", "[types][values][simd]") {
    SECTION("V128 basic properties") {
        REQUIRE(sizeof(V128) == 16);
        REQUIRE(alignof(V128) == 16);
        
        V128 v;
        REQUIRE(v.bytes.size() == 16);
        
        // Test default initialization (should be zero)
        for (size_t i = 0; i < 16; ++i) {
            REQUIRE(v.bytes[i] == 0);
        }
    }
    
    SECTION("V128 construction") {
        std::array<uint8_t, 16> test_bytes = {
            0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
            0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
        };
        
        V128 v(test_bytes);
        
        for (size_t i = 0; i < 16; ++i) {
            REQUIRE(v.bytes[i] == test_bytes[i]);
        }
    }
    
    SECTION("V128 different interpretations") {
        V128 v;
        
        // Set some data
        v.u32[0] = 0x12345678;
        v.u32[1] = 0x9ABCDEF0;
        v.u32[2] = 0x11111111;
        v.u32[3] = 0x22222222;
        
        // Verify different interpretations access the same memory
        REQUIRE(v.u16[0] == 0x5678);  // Little-endian assumption
        REQUIRE(v.u16[1] == 0x1234);
        REQUIRE(v.u8[0] == 0x78);
        REQUIRE(v.u8[1] == 0x56);
    }
}

// =============================================================================
// Advanced Type Traits System
// =============================================================================

TEST_CASE("Value Type Traits", "[types][values][traits]") {
    SECTION("Basic type traits") {
        using traits_i32 = value_type_traits<ValueType::I32>;
        
        REQUIRE(traits_i32::type == ValueType::I32);
        REQUIRE(traits_i32::size == 4);
        REQUIRE(traits_i32::alignment == 4);
        REQUIRE(std::string_view(traits_i32::name) == "i32");
        REQUIRE(traits_i32::is_number);
        REQUIRE_FALSE(traits_i32::is_vector);
        REQUIRE_FALSE(traits_i32::is_reference);
        REQUIRE(traits_i32::is_valid);
    }
    
    SECTION("Platform-specific I32 traits") {
        using traits = value_type_traits<ValueType::I32>;
        
        REQUIRE(std::is_same_v<traits::storage_type, int32_t>);
        REQUIRE(std::is_same_v<traits::fast_type, int32_t>);
        REQUIRE(traits::is_signed);
        REQUIRE(traits::has_fast_ops); // Should be true on all platforms
        REQUIRE(traits::default_value() == 0);
        REQUIRE(traits::min_value() == INT32_MIN);
        REQUIRE(traits::max_value() == INT32_MAX);
        REQUIRE(traits::in_range(100));
        REQUIRE(traits::in_range(INT32_MIN));
        REQUIRE(traits::in_range(INT32_MAX));
        REQUIRE_FALSE(traits::in_range(static_cast<int64_t>(INT32_MAX) + 1));
    }
    
    SECTION("Platform-specific I64 traits") {
        using traits = value_type_traits<ValueType::I64>;
        
        REQUIRE(std::is_same_v<traits::storage_type, int64_t>);
        REQUIRE(std::is_same_v<traits::fast_type, int64_t>);
        REQUIRE(traits::is_signed);
        REQUIRE(traits::default_value() == 0);
        REQUIRE(traits::min_value() == INT64_MIN);
        REQUIRE(traits::max_value() == INT64_MAX);
        
        // Platform-specific optimization check
        #if defined(FLIGHT_WASM_PLATFORM_DREAMCAST) || defined(FLIGHT_WASM_PLATFORM_PSP)
            REQUIRE_FALSE(traits::has_fast_ops);
            REQUIRE(traits::needs_emulation);
        #else
            REQUIRE(traits::has_fast_ops);
            REQUIRE_FALSE(traits::needs_emulation);
        #endif
    }
    
    SECTION("Platform-specific F32 traits") {
        using traits = value_type_traits<ValueType::F32>;
        
        REQUIRE(std::is_same_v<traits::storage_type, float>);
        REQUIRE(std::is_same_v<traits::fast_type, float>);
        REQUIRE(traits::is_floating_point);
        REQUIRE(traits::has_fast_ops);  // Most platforms have FPU
        REQUIRE(traits::default_value() == 0.0f);
        REQUIRE(traits::mantissa_bits == 23);
        REQUIRE(traits::exponent_bits == 8);
    }
    
    SECTION("Platform-specific F64 traits") {
        using traits = value_type_traits<ValueType::F64>;
        
        REQUIRE(std::is_same_v<traits::storage_type, double>);
        REQUIRE(std::is_same_v<traits::fast_type, double>);
        REQUIRE(traits::is_floating_point);
        REQUIRE(traits::default_value() == 0.0);
        REQUIRE(traits::mantissa_bits == 52);
        REQUIRE(traits::exponent_bits == 11);
        
        // Platform-specific double precision support
        #if defined(FLIGHT_WASM_PLATFORM_DREAMCAST) || defined(FLIGHT_WASM_PLATFORM_PSP)
            REQUIRE_FALSE(traits::has_fast_ops);
        #else
            REQUIRE(traits::has_fast_ops);
        #endif
    }
    
    SECTION("Platform-specific V128 traits") {
        using traits = value_type_traits<ValueType::V128>;
        
        REQUIRE(std::is_same_v<traits::storage_type, V128>);
        REQUIRE(std::is_same_v<traits::fast_type, V128>);
        REQUIRE_FALSE(traits::is_number);
        REQUIRE(traits::is_vector);
        REQUIRE_FALSE(traits::is_reference);
        
        // SIMD support varies by platform
        #ifdef FLIGHT_WASM_NEON_AVAILABLE
            REQUIRE(traits::has_simd_support);
            REQUIRE(traits::has_fast_ops);
        #else
            REQUIRE_FALSE(traits::has_simd_support);
            REQUIRE_FALSE(traits::has_fast_ops);
        #endif
    }
    
    SECTION("Reference type traits") {
        using funcref_traits = value_type_traits<ValueType::FuncRef>;
        using externref_traits = value_type_traits<ValueType::ExternRef>;
        
        // FuncRef traits
        REQUIRE(std::is_same_v<funcref_traits::storage_type, void*>);
        REQUIRE(funcref_traits::is_reference);
        REQUIRE(funcref_traits::is_nullable);
        REQUIRE(funcref_traits::has_fast_ops);
        REQUIRE(funcref_traits::default_value() == nullptr);
        REQUIRE(funcref_traits::null_value() == nullptr);
        
        // ExternRef traits
        REQUIRE(std::is_same_v<externref_traits::storage_type, void*>);
        REQUIRE(externref_traits::is_reference);
        REQUIRE(externref_traits::is_nullable);
        REQUIRE(externref_traits::has_fast_ops);
        REQUIRE(externref_traits::default_value() == nullptr);
        REQUIRE(externref_traits::null_value() == nullptr);
    }
}

TEST_CASE("Type Mapping and Conversion", "[types][values][mapping]") {
    SECTION("C++ to ValueType mapping") {
        REQUIRE(cpp_to_value_type_v<int32_t> == ValueType::I32);
        REQUIRE(cpp_to_value_type_v<uint32_t> == ValueType::I32);
        REQUIRE(cpp_to_value_type_v<int64_t> == ValueType::I64);
        REQUIRE(cpp_to_value_type_v<uint64_t> == ValueType::I64);
        REQUIRE(cpp_to_value_type_v<float> == ValueType::F32);
        REQUIRE(cpp_to_value_type_v<double> == ValueType::F64);
        REQUIRE(cpp_to_value_type_v<V128> == ValueType::V128);
    }
    
    SECTION("WASM value type conversion detection") {
        REQUIRE(is_convertible_to_wasm_value_v<int32_t>);
        REQUIRE(is_convertible_to_wasm_value_v<uint32_t>);
        REQUIRE(is_convertible_to_wasm_value_v<int64_t>);
        REQUIRE(is_convertible_to_wasm_value_v<uint64_t>);
        REQUIRE(is_convertible_to_wasm_value_v<float>);
        REQUIRE(is_convertible_to_wasm_value_v<double>);
        REQUIRE(is_convertible_to_wasm_value_v<V128>);
        
        // Non-convertible types
        REQUIRE_FALSE(is_convertible_to_wasm_value_v<std::string>);
        REQUIRE_FALSE(is_convertible_to_wasm_value_v<void*>);
        REQUIRE_FALSE(is_convertible_to_wasm_value_v<char>);
    }
    
    SECTION("WASM value type trait detection") {
        REQUIRE(is_wasm_value_type_v<int32_t>);
        REQUIRE(is_wasm_value_type_v<int64_t>);
        REQUIRE(is_wasm_value_type_v<float>);
        REQUIRE(is_wasm_value_type_v<double>);
        REQUIRE(is_wasm_value_type_v<V128>);
        
        REQUIRE_FALSE(is_wasm_value_type_v<uint32_t>);  // Not directly mapped
        REQUIRE_FALSE(is_wasm_value_type_v<std::string>);
        REQUIRE_FALSE(is_wasm_value_type_v<void*>);
    }
}

// =============================================================================
// Zero-Overhead Type Operations
// =============================================================================

TEST_CASE("Zero-Overhead Type Operations", "[types][values][performance]") {
    SECTION("Compile-time type comparison") {
        // Compile-time comparisons
        static_assert(types_equal<ValueType::I32, ValueType::I32>());
        static_assert(!types_equal<ValueType::I32, ValueType::I64>());
        static_assert(types_equal<ValueType::F32, ValueType::F32>());
        static_assert(!types_equal<ValueType::F32, ValueType::F64>());
    }
    
    SECTION("Runtime type comparison") {
        // Runtime comparisons (should compile to single instruction)
        REQUIRE(types_equal(ValueType::I32, ValueType::I32));
        REQUIRE_FALSE(types_equal(ValueType::I32, ValueType::I64));
        REQUIRE(types_equal(ValueType::F32, ValueType::F32));
        REQUIRE_FALSE(types_equal(ValueType::F32, ValueType::F64));
        REQUIRE(types_equal(ValueType::V128, ValueType::V128));
        REQUIRE_FALSE(types_equal(ValueType::FuncRef, ValueType::ExternRef));
    }
    
    SECTION("Type convertibility checking") {
        // Compile-time convertibility
        static_assert(is_convertible<ValueType::I32, ValueType::I32>());
        static_assert(is_convertible<ValueType::I32, ValueType::I64>());
        static_assert(is_convertible<ValueType::F32, ValueType::F64>());
        static_assert(!is_convertible<ValueType::I32, ValueType::FuncRef>());
        static_assert(!is_convertible<ValueType::V128, ValueType::I32>());
        
        // Runtime convertibility
        REQUIRE(is_convertible(ValueType::I32, ValueType::I32));
        REQUIRE(is_convertible(ValueType::I32, ValueType::I64));
        REQUIRE(is_convertible(ValueType::F32, ValueType::F64));
        REQUIRE_FALSE(is_convertible(ValueType::I32, ValueType::FuncRef));
        REQUIRE_FALSE(is_convertible(ValueType::V128, ValueType::I32));
    }
}

// =============================================================================
// Binary Format Integration
// =============================================================================

TEST_CASE("Binary Format Integration", "[types][values][binary]") {
    SECTION("Type encoding") {
        REQUIRE(encode_value_type(ValueType::I32) == 0x7F);
        REQUIRE(encode_value_type(ValueType::I64) == 0x7E);
        REQUIRE(encode_value_type(ValueType::F32) == 0x7D);
        REQUIRE(encode_value_type(ValueType::F64) == 0x7C);
        REQUIRE(encode_value_type(ValueType::V128) == 0x7B);
        REQUIRE(encode_value_type(ValueType::FuncRef) == 0x70);
        REQUIRE(encode_value_type(ValueType::ExternRef) == 0x6F);
    }
    
    SECTION("Type decoding") {
        // Valid type decoding
        auto result_i32 = decode_value_type(0x7F);
        REQUIRE(result_i32.success());
        REQUIRE(result_i32.value() == ValueType::I32);
        
        auto result_i64 = decode_value_type(0x7E);
        REQUIRE(result_i64.success());
        REQUIRE(result_i64.value() == ValueType::I64);
        
        auto result_f32 = decode_value_type(0x7D);
        REQUIRE(result_f32.success());
        REQUIRE(result_f32.value() == ValueType::F32);
        
        auto result_f64 = decode_value_type(0x7C);
        REQUIRE(result_f64.success());
        REQUIRE(result_f64.value() == ValueType::F64);
        
        auto result_v128 = decode_value_type(0x7B);
        REQUIRE(result_v128.success());
        REQUIRE(result_v128.value() == ValueType::V128);
        
        auto result_funcref = decode_value_type(0x70);
        REQUIRE(result_funcref.success());
        REQUIRE(result_funcref.value() == ValueType::FuncRef);
        
        auto result_externref = decode_value_type(0x6F);
        REQUIRE(result_externref.success());
        REQUIRE(result_externref.value() == ValueType::ExternRef);
    }
    
    SECTION("Invalid type decoding") {
        auto result_invalid = decode_value_type(0x00);
        REQUIRE(result_invalid.failed());
        REQUIRE(result_invalid.error().code() == ErrorCode::TypeMismatch);
        
        auto result_invalid2 = decode_value_type(0xFF);
        REQUIRE(result_invalid2.failed());
        REQUIRE(result_invalid2.error().code() == ErrorCode::TypeMismatch);
    }
    
    SECTION("Encoding/decoding round trip") {
        ValueType types[] = {
            ValueType::I32, ValueType::I64, ValueType::F32, ValueType::F64,
            ValueType::V128, ValueType::FuncRef, ValueType::ExternRef
        };
        
        for (auto type : types) {
            uint8_t encoded = encode_value_type(type);
            auto decoded = decode_value_type(encoded);
            REQUIRE(decoded.success());
            REQUIRE(decoded.value() == type);
        }
    }
}

// =============================================================================
// Type Validation Framework
// =============================================================================

TEST_CASE("Type Validation Framework", "[types][values][validation]") {
    SECTION("Type encoding validation") {
        // Valid encodings
        auto result = validation::validate_type_encoding(0x7F);
        REQUIRE(result.success());
        REQUIRE(result.value() == ValueType::I32);
        
        // Invalid encodings
        auto invalid_result = validation::validate_type_encoding(0x00);
        REQUIRE(invalid_result.failed());
    }
    
    SECTION("Type matching validation") {
        // Compile-time validation
        static_assert(validation::validate_type_match<ValueType::I32, ValueType::I32>());
        static_assert(!validation::validate_type_match<ValueType::I32, ValueType::I64>());
        
        // Runtime validation
        REQUIRE(validation::validate_type_match(ValueType::I32, ValueType::I32));
        REQUIRE_FALSE(validation::validate_type_match(ValueType::I32, ValueType::I64));
    }
    
    SECTION("Specific type validation") {
        REQUIRE(validation::validate_numeric_type(ValueType::I32));
        REQUIRE(validation::validate_numeric_type(ValueType::F64));
        REQUIRE_FALSE(validation::validate_numeric_type(ValueType::V128));
        REQUIRE_FALSE(validation::validate_numeric_type(ValueType::FuncRef));
        
        REQUIRE(validation::validate_vector_type(ValueType::V128));
        REQUIRE_FALSE(validation::validate_vector_type(ValueType::I32));
        REQUIRE_FALSE(validation::validate_vector_type(ValueType::FuncRef));
        
        REQUIRE(validation::validate_reference_type(ValueType::FuncRef));
        REQUIRE(validation::validate_reference_type(ValueType::ExternRef));
        REQUIRE_FALSE(validation::validate_reference_type(ValueType::I32));
        REQUIRE_FALSE(validation::validate_reference_type(ValueType::V128));
    }
    
    SECTION("Type sequence validation") {
        ValueType valid_types[] = {ValueType::I32, ValueType::F64, ValueType::V128};
        REQUIRE(validation::validate_type_sequence(valid_types, 3));
        
        ValueType invalid_types[] = {ValueType::I32, static_cast<ValueType>(0xFF), ValueType::F64};
        REQUIRE_FALSE(validation::validate_type_sequence(invalid_types, 3));
        
        // Empty sequence should be valid
        REQUIRE(validation::validate_type_sequence(nullptr, 0));
    }
    
    SECTION("Function type validation") {
        ValueType params[] = {ValueType::I32, ValueType::F32};
        ValueType results[] = {ValueType::I64};
        
        REQUIRE(validation::validate_function_types(params, 2, results, 1));
        
        ValueType invalid_params[] = {ValueType::I32, static_cast<ValueType>(0xFF)};
        REQUIRE_FALSE(validation::validate_function_types(invalid_params, 2, results, 1));
        
        ValueType invalid_results[] = {static_cast<ValueType>(0x00)};
        REQUIRE_FALSE(validation::validate_function_types(params, 2, invalid_results, 1));
    }
}

// =============================================================================
// Compact Type Representation
// =============================================================================

TEST_CASE("Compact Value Type", "[types][values][compact]") {
    SECTION("Basic compact type functionality") {
        CompactValueType compact_i32(ValueType::I32);
        
        REQUIRE(compact_i32.type() == ValueType::I32);
        REQUIRE(compact_i32.encoding() == 0x7F);
        REQUIRE(compact_i32.is_number());
        REQUIRE_FALSE(compact_i32.is_vector());
        REQUIRE_FALSE(compact_i32.is_reference());
    }
    
    SECTION("Compact type comparison") {
        CompactValueType compact1(ValueType::I32);
        CompactValueType compact2(ValueType::I32);
        CompactValueType compact3(ValueType::I64);
        
        REQUIRE(compact1 == compact2);
        REQUIRE_FALSE(compact1 == compact3);
        REQUIRE(compact1 != compact3);
        REQUIRE_FALSE(compact1 != compact2);
    }
    
    SECTION("Memory efficiency") {
        REQUIRE(sizeof(CompactValueType) == 1);
        
        // Test all value types
        CompactValueType types[] = {
            CompactValueType(ValueType::I32),
            CompactValueType(ValueType::I64),
            CompactValueType(ValueType::F32),
            CompactValueType(ValueType::F64),
            CompactValueType(ValueType::V128),
            CompactValueType(ValueType::FuncRef),
            CompactValueType(ValueType::ExternRef)
        };
        
        REQUIRE(sizeof(types) == 7); // 7 single-byte types
    }
}

// =============================================================================
// WebAssembly Specification Compliance Tests
// =============================================================================

TEST_CASE("WebAssembly Spec Compliance - Value Types", "[types][values][spec][compliance]") {
    SECTION("Section 2.3.1 - Number Types") {
        // Verify exact specification values
        REQUIRE(static_cast<uint8_t>(ValueType::I32) == 0x7F);
        REQUIRE(static_cast<uint8_t>(ValueType::I64) == 0x7E);
        REQUIRE(static_cast<uint8_t>(ValueType::F32) == 0x7D);
        REQUIRE(static_cast<uint8_t>(ValueType::F64) == 0x7C);
    }
    
    SECTION("Section 2.3.2 - Vector Types") {
        // V128 vector type
        REQUIRE(static_cast<uint8_t>(ValueType::V128) == 0x7B);
        REQUIRE(is_vector_type(ValueType::V128));
    }
    
    SECTION("Section 2.3.3 - Reference Types") {
        // funcref and externref
        REQUIRE(static_cast<uint8_t>(ValueType::FuncRef) == 0x70);
        REQUIRE(static_cast<uint8_t>(ValueType::ExternRef) == 0x6F);
        REQUIRE(is_reference_type(ValueType::FuncRef));
        REQUIRE(is_reference_type(ValueType::ExternRef));
    }
    
    SECTION("IEEE 754 Floating Point Compliance") {
        // Verify IEEE 754 compliance at compile time
        static_assert(std::numeric_limits<float>::is_iec559, "f32 must be IEEE 754");
        static_assert(std::numeric_limits<double>::is_iec559, "f64 must be IEEE 754");
        static_assert(sizeof(float) == 4, "f32 must be 4 bytes");
        static_assert(sizeof(double) == 8, "f64 must be 8 bytes");
    }
}

// =============================================================================
// Platform-Specific Tests
// =============================================================================

TEST_CASE("Platform-Specific Value Behavior", "[types][values][platform]") {
    SECTION("Memory layout consistency") {
        // Ensure consistent memory layout across platforms
        V128 v1, v2;
        
        v1.u32[0] = 0x12345678;
        v2.bytes[0] = 0x78;
        v2.bytes[1] = 0x56;
        v2.bytes[2] = 0x34;
        v2.bytes[3] = 0x12;
        
        // On little-endian systems, these should match
        // On big-endian systems, the behavior is defined by the platform
        REQUIRE(v1.u32[0] == v2.u32[0]);
    }
    
    SECTION("Alignment requirements") {
        // V128 must be properly aligned
        V128 v;
        auto addr = reinterpret_cast<uintptr_t>(&v);
        REQUIRE((addr % 16) == 0);
    }
}

// =============================================================================
// Value Class Tests
// =============================================================================

TEST_CASE("Value Construction", "[types][value]") {
    SECTION("Default constructor") {
        Value value;
        REQUIRE(value.type() == ValueType::I32);
        REQUIRE(value.as_i32().success());
        REQUIRE(value.as_i32().value() == 0);
        REQUIRE(value.is_number());
        REQUIRE_FALSE(value.is_vector());
        REQUIRE_FALSE(value.is_reference());
    }
    
    SECTION("i32 construction") {
        auto value = Value::from_i32(42);
        REQUIRE(value.type() == ValueType::I32);
        REQUIRE(value.as_i32().success());
        REQUIRE(value.as_i32().value() == 42);
        REQUIRE(value.is_number());
        REQUIRE_FALSE(value.is_vector());
        REQUIRE_FALSE(value.is_reference());
    }
    
    SECTION("i64 construction") {
        auto value = Value::from_i64(-12345678901234LL);
        REQUIRE(value.type() == ValueType::I64);
        REQUIRE(value.as_i64().success());
        REQUIRE(value.as_i64().value() == -12345678901234LL);
        REQUIRE(value.is_number());
    }
    
    SECTION("f32 construction") {
        auto value = Value::from_f32(3.14159f);
        REQUIRE(value.type() == ValueType::F32);
        REQUIRE(value.as_f32().success());
        REQUIRE(value.as_f32().value() == 3.14159f);
        REQUIRE(value.is_number());
    }
    
    SECTION("f64 construction") {
        auto value = Value::from_f64(3.141592653589793);
        REQUIRE(value.type() == ValueType::F64);
        REQUIRE(value.as_f64().success());
        REQUIRE(value.as_f64().value() == 3.141592653589793);
        REQUIRE(value.is_number());
    }
    
    SECTION("v128 construction") {
        V128 vec_data{};
        vec_data.bytes[0] = 0xFF;
        vec_data.bytes[15] = 0xAA;
        
        auto value = Value::from_v128(vec_data);
        REQUIRE(value.type() == ValueType::V128);
        REQUIRE(value.as_v128().success());
        REQUIRE(value.as_v128().value().bytes[0] == 0xFF);
        REQUIRE(value.as_v128().value().bytes[15] == 0xAA);
        REQUIRE(value.is_vector());
    }
    
    SECTION("funcref construction") {
        void* dummy_ptr = reinterpret_cast<void*>(0x12345678);
        auto value = Value::from_funcref(dummy_ptr);
        
        REQUIRE(value.type() == ValueType::FuncRef);
        REQUIRE(value.as_funcref().success());
        REQUIRE(value.as_funcref().value() == dummy_ptr);
        REQUIRE(value.is_reference());
    }
    
    SECTION("externref construction") {
        void* dummy_ptr = reinterpret_cast<void*>(0x87654321);
        auto value = Value::from_externref(dummy_ptr);
        
        REQUIRE(value.type() == ValueType::ExternRef);
        REQUIRE(value.as_externref().success());
        REQUIRE(value.as_externref().value() == dummy_ptr);
        REQUIRE(value.is_reference());
    }
}

TEST_CASE("Value Type Safety", "[types][value][safety]") {
    SECTION("Type mismatch detection") {
        auto i32_value = Value::from_i32(42);
        
        // Should fail when extracting as wrong type
        REQUIRE_FALSE(i32_value.as_i64().success());
        REQUIRE_FALSE(i32_value.as_f32().success());
        REQUIRE_FALSE(i32_value.as_f64().success());
        REQUIRE_FALSE(i32_value.as_v128().success());
        REQUIRE_FALSE(i32_value.as_funcref().success());
        REQUIRE_FALSE(i32_value.as_externref().success());
        
        // Error codes should be correct
        REQUIRE(i32_value.as_i64().error().code() == ErrorCode::TypeMismatch);
        REQUIRE(i32_value.as_f32().error().code() == ErrorCode::TypeMismatch);
    }
    
    SECTION("Default values") {
        auto i32_default = Value::default_for_type(ValueType::I32);
        REQUIRE(i32_default.as_i32().value() == 0);
        
        auto i64_default = Value::default_for_type(ValueType::I64);
        REQUIRE(i64_default.as_i64().value() == 0);
        
        auto f32_default = Value::default_for_type(ValueType::F32);
        REQUIRE(f32_default.as_f32().value() == 0.0f);
        
        auto f64_default = Value::default_for_type(ValueType::F64);
        REQUIRE(f64_default.as_f64().value() == 0.0);
        
        auto funcref_default = Value::default_for_type(ValueType::FuncRef);
        REQUIRE(funcref_default.as_funcref().value() == nullptr);
        
        auto externref_default = Value::default_for_type(ValueType::ExternRef);
        REQUIRE(externref_default.as_externref().value() == nullptr);
        
        auto v128_default = Value::default_for_type(ValueType::V128);
        REQUIRE(v128_default.as_v128().success());
        // V128 default should be all zeros
        for (size_t i = 0; i < 16; ++i) {
            REQUIRE(v128_default.as_v128().value().bytes[i] == 0);
        }
    }
}

TEST_CASE("Value Copy and Move", "[types][value][copying]") {
    SECTION("Copy constructor") {
        auto original = Value::from_i32(42);
        Value copy(original);
        
        REQUIRE(copy.type() == ValueType::I32);
        REQUIRE(copy.as_i32().value() == 42);
        REQUIRE(copy == original);
    }
    
    SECTION("Move constructor") {
        auto original = Value::from_f64(3.14159);
        Value moved(std::move(original));
        
        REQUIRE(moved.type() == ValueType::F64);
        REQUIRE(moved.as_f64().value() == 3.14159);
        
        // Original should be reset to default state
        REQUIRE(original.type() == ValueType::I32);
        REQUIRE(original.as_i32().value() == 0);
    }
    
    SECTION("Copy assignment") {
        auto original = Value::from_i64(-12345);
        Value assigned = Value::from_f32(1.0f);
        
        assigned = original;
        
        REQUIRE(assigned.type() == ValueType::I64);
        REQUIRE(assigned.as_i64().value() == -12345);
        REQUIRE(assigned == original);
    }
    
    SECTION("Move assignment") {
        auto original = Value::from_v128(V128{});
        Value assigned = Value::from_i32(100);
        
        assigned = std::move(original);
        
        REQUIRE(assigned.type() == ValueType::V128);
        REQUIRE(assigned.as_v128().success());
        
        // Original should be reset to default state
        REQUIRE(original.type() == ValueType::I32);
        REQUIRE(original.as_i32().value() == 0);
    }
}

TEST_CASE("Value Equality", "[types][value][comparison]") {
    SECTION("Same type equality") {
        auto a = Value::from_i32(42);
        auto b = Value::from_i32(42);
        auto c = Value::from_i32(24);
        
        REQUIRE(a == b);
        REQUIRE(a != c);
        REQUIRE(b != c);
    }
    
    SECTION("Different type inequality") {
        auto i32_val = Value::from_i32(42);
        auto i64_val = Value::from_i64(42);
        
        REQUIRE(i32_val != i64_val);  // Different types
    }
    
    SECTION("Floating-point equality") {
        auto a = Value::from_f32(3.14f);
        auto b = Value::from_f32(3.14f);
        auto c = Value::from_f32(3.15f);
        
        REQUIRE(a == b);
        REQUIRE(a != c);
        
        // Test special float values
        auto nan1 = Value::from_f32(std::numeric_limits<float>::quiet_NaN());
        auto nan2 = Value::from_f32(std::numeric_limits<float>::quiet_NaN());
        REQUIRE_FALSE(nan1 == nan2); // NaN != NaN
        
        auto inf1 = Value::from_f32(std::numeric_limits<float>::infinity());
        auto inf2 = Value::from_f32(std::numeric_limits<float>::infinity());
        REQUIRE(inf1 == inf2);
    }
    
    SECTION("v128 equality") {
        V128 vec1{}, vec2{}, vec3{};
        vec1.bytes[0] = 0xAA;
        vec2.bytes[0] = 0xAA;
        vec3.bytes[0] = 0xBB;
        
        auto a = Value::from_v128(vec1);
        auto b = Value::from_v128(vec2);
        auto c = Value::from_v128(vec3);
        
        REQUIRE(a == b);
        REQUIRE(a != c);
    }
    
    SECTION("Reference equality") {
        void* ptr1 = reinterpret_cast<void*>(0x1000);
        void* ptr2 = reinterpret_cast<void*>(0x2000);
        
        auto func1 = Value::from_funcref(ptr1);
        auto func2 = Value::from_funcref(ptr1);
        auto func3 = Value::from_funcref(ptr2);
        
        REQUIRE(func1 == func2);
        REQUIRE(func1 != func3);
        
        auto extern1 = Value::from_externref(ptr1);
        auto extern2 = Value::from_externref(ptr1);
        
        REQUIRE(extern1 == extern2);
        REQUIRE(func1 != extern1); // Different types
    }
}

TEST_CASE("Value Arithmetic", "[types][value][arithmetic]") {
    SECTION("i32 addition") {
        auto a = Value::from_i32(10);
        auto b = Value::from_i32(32);
        
        auto result = arithmetic::add_i32(a, b);
        REQUIRE(result.success());
        REQUIRE(result.value().as_i32().value() == 42);
    }
    
    SECTION("i32 overflow wrapping") {
        auto a = Value::from_i32(INT32_MAX);
        auto b = Value::from_i32(1);
        
        auto result = arithmetic::add_i32(a, b);
        REQUIRE(result.success());
        REQUIRE(result.value().as_i32().value() == INT32_MIN);  // Wraps
    }
    
    SECTION("i64 addition") {
        auto a = Value::from_i64(1000000000000LL);
        auto b = Value::from_i64(2000000000000LL);
        
        auto result = arithmetic::add_i64(a, b);
        REQUIRE(result.success());
        REQUIRE(result.value().as_i64().value() == 3000000000000LL);
    }
    
    SECTION("i64 overflow wrapping") {
        auto a = Value::from_i64(INT64_MAX);
        auto b = Value::from_i64(1);
        
        auto result = arithmetic::add_i64(a, b);
        REQUIRE(result.success());
        REQUIRE(result.value().as_i64().value() == INT64_MIN);  // Wraps
    }
    
    SECTION("f32 addition") {
        auto a = Value::from_f32(1.5f);
        auto b = Value::from_f32(2.5f);
        
        auto result = arithmetic::add_f32(a, b);
        REQUIRE(result.success());
        REQUIRE(result.value().as_f32().value() == 4.0f);
    }
    
    SECTION("f64 addition") {
        auto a = Value::from_f64(1.23456789);
        auto b = Value::from_f64(9.87654321);
        
        auto result = arithmetic::add_f64(a, b);
        REQUIRE(result.success());
        REQUIRE(result.value().as_f64().value() == (1.23456789 + 9.87654321));
    }
    
    SECTION("Type mismatch in arithmetic") {
        auto a = Value::from_i32(10);
        auto b = Value::from_f32(3.14f);
        
        auto result = arithmetic::add_i32(a, b);
        REQUIRE_FALSE(result.success());
        REQUIRE(result.error().code() == ErrorCode::TypeMismatch);
    }
}

TEST_CASE("Value String Representation", "[types][value][string]") {
    SECTION("Integer string representation") {
        auto i32_val = Value::from_i32(42);
        REQUIRE(i32_val.to_string() == "42");
        
        auto i32_neg = Value::from_i32(-12345);
        REQUIRE(i32_neg.to_string() == "-12345");
        
        auto i64_val = Value::from_i64(1234567890123456LL);
        REQUIRE(i64_val.to_string() == "1234567890123456");
    }
    
    SECTION("Float string representation") {
        auto f32_val = Value::from_f32(3.14f);
        auto f32_str = f32_val.to_string();
        REQUIRE(f32_str.find("3.14") != std::string::npos);
        
        auto f64_val = Value::from_f64(2.718281828);
        auto f64_str = f64_val.to_string();
        REQUIRE(f64_str.find("2.718281828") != std::string::npos);
    }
    
    SECTION("V128 string representation") {
        V128 vec{};
        vec.bytes[0] = 0xAA;
        vec.bytes[1] = 0xBB;
        vec.bytes[15] = 0xFF;
        
        auto v128_val = Value::from_v128(vec);
        auto str = v128_val.to_string();
        
        REQUIRE(str.find("v128(") != std::string::npos);
        REQUIRE(str.find("aa") != std::string::npos);
        REQUIRE(str.find("bb") != std::string::npos);
        REQUIRE(str.find("ff") != std::string::npos);
    }
    
    SECTION("Reference string representation") {
        void* ptr = reinterpret_cast<void*>(0x12345678);
        
        auto funcref_val = Value::from_funcref(ptr);
        auto funcref_str = funcref_val.to_string();
        REQUIRE(funcref_str.find("funcref(0x") != std::string::npos);
        
        auto externref_val = Value::from_externref(ptr);
        auto externref_str = externref_val.to_string();
        REQUIRE(externref_str.find("externref(0x") != std::string::npos);
    }
}

TEST_CASE("Value Serialization", "[types][value][binary]") {
    SECTION("i32 serialization round trip") {
        auto original = Value::from_i32(-12345);
        
        std::vector<uint8_t> serialized;
        binary::serialize_value(original, serialized);
        REQUIRE(serialized.size() == 4);
        
        size_t offset = 0;
        auto deserialized = binary::deserialize_value(
            ValueType::I32, serialized.data(), offset, serialized.size());
        
        REQUIRE(deserialized.success());
        REQUIRE(deserialized.value() == original);
        REQUIRE(offset == 4);
    }
    
    SECTION("i64 serialization round trip") {
        auto original = Value::from_i64(-1234567890123456LL);
        
        std::vector<uint8_t> serialized;
        binary::serialize_value(original, serialized);
        REQUIRE(serialized.size() == 8);
        
        size_t offset = 0;
        auto deserialized = binary::deserialize_value(
            ValueType::I64, serialized.data(), offset, serialized.size());
        
        REQUIRE(deserialized.success());
        REQUIRE(deserialized.value() == original);
        REQUIRE(offset == 8);
    }
    
    SECTION("f32 serialization round trip") {
        auto original = Value::from_f32(3.14159f);
        
        std::vector<uint8_t> serialized;
        binary::serialize_value(original, serialized);
        REQUIRE(serialized.size() == 4);
        
        size_t offset = 0;
        auto deserialized = binary::deserialize_value(
            ValueType::F32, serialized.data(), offset, serialized.size());
        
        REQUIRE(deserialized.success());
        REQUIRE(deserialized.value() == original);
        REQUIRE(offset == 4);
    }
    
    SECTION("f64 serialization round trip") {
        auto original = Value::from_f64(3.141592653589793);
        
        std::vector<uint8_t> serialized;
        binary::serialize_value(original, serialized);
        REQUIRE(serialized.size() == 8);
        
        size_t offset = 0;
        auto deserialized = binary::deserialize_value(
            ValueType::F64, serialized.data(), offset, serialized.size());
        
        REQUIRE(deserialized.success());
        REQUIRE(deserialized.value() == original);
        REQUIRE(offset == 8);
    }
    
    SECTION("v128 serialization round trip") {
        V128 vec{};
        for (size_t i = 0; i < 16; ++i) {
            vec.bytes[i] = static_cast<uint8_t>(i + 1);
        }
        auto original = Value::from_v128(vec);
        
        std::vector<uint8_t> serialized;
        binary::serialize_value(original, serialized);
        REQUIRE(serialized.size() == 16);
        
        size_t offset = 0;
        auto deserialized = binary::deserialize_value(
            ValueType::V128, serialized.data(), offset, serialized.size());
        
        REQUIRE(deserialized.success());
        REQUIRE(deserialized.value() == original);
        REQUIRE(offset == 16);
    }
    
    SECTION("Insufficient data error handling") {
        std::vector<uint8_t> insufficient_data = {0x01, 0x02}; // Only 2 bytes
        
        size_t offset = 0;
        auto result = binary::deserialize_value(
            ValueType::I32, insufficient_data.data(), offset, insufficient_data.size());
        
        REQUIRE_FALSE(result.success());
        REQUIRE(result.error().code() == ErrorCode::UnexpectedEndOfFile);
    }
}

TEST_CASE("Value Performance", "[types][value][performance]") {
    SECTION("Construction performance") {
        constexpr int iterations = 100000;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            volatile auto value = Value::from_i32(i);
            (void)value;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        auto ns_per_op = duration.count() / static_cast<double>(iterations);
        
        // Should be very fast (< 10ns per operation on most platforms)
        INFO("Construction: " << ns_per_op << "ns per operation");
        REQUIRE(ns_per_op < 100.0); // Allow generous margin for test overhead
    }
    
    SECTION("Extraction performance") {
        auto value = Value::from_i32(42);
        constexpr int iterations = 100000;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            volatile auto result = value.as_i32();
            (void)result;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        auto ns_per_op = duration.count() / static_cast<double>(iterations);
        
        INFO("Extraction: " << ns_per_op << "ns per operation");
        REQUIRE(ns_per_op < 50.0); // Should be very fast
    }
    
    SECTION("Memory usage") {
        // Verify Value size meets embedded platform constraints
        REQUIRE(sizeof(Value) <= 32);
        
        // Test memory layout
        Value values[10];
        uintptr_t first_addr = reinterpret_cast<uintptr_t>(&values[0]);
        uintptr_t second_addr = reinterpret_cast<uintptr_t>(&values[1]);
        
        // Values should be tightly packed
        REQUIRE((second_addr - first_addr) == sizeof(Value));
    }
}

TEST_CASE("Zero-Overhead Template Operations", "[types][value][templates]") {
    SECTION("Typed value operations") {
        using ops = optimized::typed_value_operations<ValueType::I32>;
        
        auto value = ops::construct(42);
        REQUIRE(value.type() == ValueType::I32);
        REQUIRE(value.as_i32().value() == 42);
        
        auto extracted = ops::extract(value);
        REQUIRE(extracted.success());
        REQUIRE(extracted.value() == 42);
    }
    
    SECTION("Value visitor pattern") {
        auto i32_value = Value::from_i32(42);
        auto f32_value = Value::from_f32(3.14f);
        
        bool called = false;
        auto visitor = [&called](auto value) -> int {
            called = true;
            if constexpr (std::is_same_v<decltype(value), int32_t>) {
                return static_cast<int>(value);
            } else if constexpr (std::is_same_v<decltype(value), float>) {
                return static_cast<int>(value);
            } else {
                return 0;
            }
        };
        
        auto result1 = optimized::visit_value(i32_value, visitor);
        REQUIRE(called);
        REQUIRE(result1 == 42);
        
        called = false;
        auto result2 = optimized::visit_value(f32_value, visitor);
        REQUIRE(called);
        REQUIRE(result2 == 3);  // 3.14f converted to int
    }
}

TEST_CASE("WebAssembly Spec Compliance - Values", "[types][value][spec][compliance]") {
    SECTION("Value representation compliance") {
        // Values must maintain bit-exact representation
        auto i32_max = Value::from_i32(INT32_MAX);
        auto i32_min = Value::from_i32(INT32_MIN);
        
        REQUIRE(i32_max.as_i32().value() == INT32_MAX);
        REQUIRE(i32_min.as_i32().value() == INT32_MIN);
        
        // Test WebAssembly wrap-around semantics
        auto overflow_result = arithmetic::add_i32(i32_max, Value::from_i32(1));
        REQUIRE(overflow_result.success());
        REQUIRE(overflow_result.value().as_i32().value() == INT32_MIN);
    }
    
    SECTION("IEEE 754 compliance") {
        // Test special float values
        auto nan_f32 = Value::from_f32(std::numeric_limits<float>::quiet_NaN());
        auto inf_f32 = Value::from_f32(std::numeric_limits<float>::infinity());
        auto neg_inf_f32 = Value::from_f32(-std::numeric_limits<float>::infinity());
        
        REQUIRE(std::isnan(nan_f32.as_f32().value()));
        REQUIRE(std::isinf(inf_f32.as_f32().value()));
        REQUIRE(std::isinf(neg_inf_f32.as_f32().value()));
        REQUIRE(neg_inf_f32.as_f32().value() < 0);
        
        // NaN behavior (NaN != NaN)
        REQUIRE_FALSE(nan_f32 == nan_f32);
        
        // Infinity behavior
        REQUIRE(inf_f32 == inf_f32);
        REQUIRE(neg_inf_f32 == neg_inf_f32);
        REQUIRE_FALSE(inf_f32 == neg_inf_f32);
    }
    
    SECTION("Reference type null values") {
        auto null_funcref = Value::default_for_type(ValueType::FuncRef);
        auto null_externref = Value::default_for_type(ValueType::ExternRef);
        
        REQUIRE(null_funcref.as_funcref().value() == nullptr);
        REQUIRE(null_externref.as_externref().value() == nullptr);
        
        REQUIRE(null_funcref == Value::from_funcref(nullptr));
        REQUIRE(null_externref == Value::from_externref(nullptr));
    }
}
