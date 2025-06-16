// =============================================================================
// Flight WASM Tests - Type Conversion System
// WebAssembly Core Specification 1.0 Compliance Testing
// =============================================================================

#include <catch2/catch_test_macros.hpp>
#include <flight/wasm/types/conversions.hpp>
#include <limits>
#include <chrono>

using namespace flight::wasm;
using namespace flight::wasm::conversions;

// =============================================================================
// Integer Conversion Tests
// =============================================================================

TEST_CASE("Integer Conversions", "[types][conversions][integer]") {
    SECTION("i32.wrap_i64") {
        auto i64_val = Value::from_i64(0x123456789ABCDEF0LL);
        auto result = TypeConverter::i32_wrap_i64(i64_val);
        
        REQUIRE(result.type() == ValueType::I32);
        REQUIRE(result.as_i32().value() == static_cast<int32_t>(0x9ABCDEF0));
    }
    
    SECTION("i32.wrap_i64 with negative values") {
        auto i64_val = Value::from_i64(-1LL);
        auto result = TypeConverter::i32_wrap_i64(i64_val);
        
        REQUIRE(result.type() == ValueType::I32);
        REQUIRE(result.as_i32().value() == -1);
    }
    
    SECTION("i64.extend_i32_s") {
        auto i32_val = Value::from_i32(-1);
        auto result = TypeConverter::i64_extend_i32_s(i32_val);
        
        REQUIRE(result.type() == ValueType::I64);
        REQUIRE(result.as_i64().value() == -1LL);  // Sign extended
    }
    
    SECTION("i64.extend_i32_s positive") {
        auto i32_val = Value::from_i32(0x7FFFFFFF);
        auto result = TypeConverter::i64_extend_i32_s(i32_val);
        
        REQUIRE(result.type() == ValueType::I64);
        REQUIRE(result.as_i64().value() == 0x000000007FFFFFFFLL);
    }
    
    SECTION("i64.extend_i32_u") {
        auto i32_val = Value::from_i32(-1);  // 0xFFFFFFFF
        auto result = TypeConverter::i64_extend_i32_u(i32_val);
        
        REQUIRE(result.type() == ValueType::I64);
        REQUIRE(result.as_i64().value() == 0x00000000FFFFFFFFLL);  // Zero extended
    }
    
    SECTION("i64.extend_i32_u positive") {
        auto i32_val = Value::from_i32(0x12345678);
        auto result = TypeConverter::i64_extend_i32_u(i32_val);
        
        REQUIRE(result.type() == ValueType::I64);
        REQUIRE(result.as_i64().value() == 0x0000000012345678LL);
    }
}

// =============================================================================
// Floating-Point Conversion Tests
// =============================================================================

TEST_CASE("Floating-Point Conversions", "[types][conversions][float]") {
    SECTION("f64.promote_f32") {
        auto f32_val = Value::from_f32(3.14159f);
        auto result = TypeConverter::f64_promote_f32(f32_val);
        
        REQUIRE(result.type() == ValueType::F64);
        // Should be exact conversion
        REQUIRE(result.as_f64().value() == static_cast<double>(3.14159f));
    }
    
    SECTION("f64.promote_f32 special values") {
        // Test NaN
        auto nan_f32 = Value::from_f32(std::numeric_limits<float>::quiet_NaN());
        auto result_nan = TypeConverter::f64_promote_f32(nan_f32);
        REQUIRE(std::isnan(result_nan.as_f64().value()));
        
        // Test positive infinity
        auto inf_f32 = Value::from_f32(std::numeric_limits<float>::infinity());
        auto result_inf = TypeConverter::f64_promote_f32(inf_f32);
        REQUIRE(std::isinf(result_inf.as_f64().value()));
        REQUIRE(result_inf.as_f64().value() > 0);
        
        // Test negative infinity
        auto neg_inf_f32 = Value::from_f32(-std::numeric_limits<float>::infinity());
        auto result_neg_inf = TypeConverter::f64_promote_f32(neg_inf_f32);
        REQUIRE(std::isinf(result_neg_inf.as_f64().value()));
        REQUIRE(result_neg_inf.as_f64().value() < 0);
    }
    
    SECTION("f32.demote_f64") {
        auto f64_val = Value::from_f64(3.141592653589793);
        auto result = TypeConverter::f32_demote_f64(f64_val);
        
        REQUIRE(result.type() == ValueType::F32);
        // May lose precision
        REQUIRE(std::abs(result.as_f32().value() - 3.14159f) < 1e-5f);
    }
    
    SECTION("f32.demote_f64 special values") {
        // Test NaN
        auto nan_f64 = Value::from_f64(std::numeric_limits<double>::quiet_NaN());
        auto result_nan = TypeConverter::f32_demote_f64(nan_f64);
        REQUIRE(std::isnan(result_nan.as_f32().value()));
        
        // Test infinity
        auto inf_f64 = Value::from_f64(std::numeric_limits<double>::infinity());
        auto result_inf = TypeConverter::f32_demote_f64(inf_f64);
        REQUIRE(std::isinf(result_inf.as_f32().value()));
        REQUIRE(result_inf.as_f32().value() > 0);
    }
    
    SECTION("f32.demote_f64 overflow") {
        // Very large f64 that overflows f32
        auto large_f64 = Value::from_f64(1e40);
        auto result = TypeConverter::f32_demote_f64(large_f64);
        REQUIRE(std::isinf(result.as_f32().value()));
    }
}

// =============================================================================
// Truncation Operation Tests (Can Trap)
// =============================================================================

TEST_CASE("Float Truncation", "[types][conversions][truncation]") {
    SECTION("i32.trunc_f32_s valid range") {
        auto f32_val = Value::from_f32(42.7f);
        auto result = TypeConverter::i32_trunc_f32_s(f32_val);
        
        REQUIRE(result.success());
        REQUIRE(result.value().as_i32().value() == 42);  // Truncated toward zero
    }
    
    SECTION("i32.trunc_f32_s negative") {
        auto f32_val = Value::from_f32(-42.7f);
        auto result = TypeConverter::i32_trunc_f32_s(f32_val);
        
        REQUIRE(result.success());
        REQUIRE(result.value().as_i32().value() == -42);  // Truncated toward zero
    }
    
    SECTION("i32.trunc_f32_s edge cases") {
        // Test exact boundary values
        auto max_val = Value::from_f32(2147483647.0f);
        auto result_max = TypeConverter::i32_trunc_f32_s(max_val);
        REQUIRE(result_max.success());
        REQUIRE(result_max.value().as_i32().value() == 2147483647);
        
        auto min_val = Value::from_f32(-2147483648.0f);
        auto result_min = TypeConverter::i32_trunc_f32_s(min_val);
        REQUIRE(result_min.success());
        REQUIRE(result_min.value().as_i32().value() == -2147483648);
    }
    
    SECTION("i32.trunc_f32_s overflow") {
        auto f32_val = Value::from_f32(3e9f);  // Larger than i32 max
        auto result = TypeConverter::i32_trunc_f32_s(f32_val);
        
        REQUIRE_FALSE(result.success());
        REQUIRE(result.error().code() == error_codes::IntegerOverflow);
    }
    
    SECTION("i32.trunc_f32_s NaN") {
        auto f32_val = Value::from_f32(std::numeric_limits<float>::quiet_NaN());
        auto result = TypeConverter::i32_trunc_f32_s(f32_val);
        
        REQUIRE_FALSE(result.success());
        REQUIRE(result.error().code() == error_codes::InvalidConversion);
    }
    
    SECTION("i32.trunc_f32_s infinity") {
        auto f32_val = Value::from_f32(std::numeric_limits<float>::infinity());
        auto result = TypeConverter::i32_trunc_f32_s(f32_val);
        
        REQUIRE_FALSE(result.success());
        REQUIRE(result.error().code() == error_codes::InvalidConversion);
        
        auto neg_inf_val = Value::from_f32(-std::numeric_limits<float>::infinity());
        auto result_neg = TypeConverter::i32_trunc_f32_s(neg_inf_val);
        
        REQUIRE_FALSE(result_neg.success());
        REQUIRE(result_neg.error().code() == error_codes::InvalidConversion);
    }
    
    SECTION("i32.trunc_f32_u valid range") {
        auto f32_val = Value::from_f32(42.7f);
        auto result = TypeConverter::i32_trunc_f32_u(f32_val);
        
        REQUIRE(result.success());
        REQUIRE(result.value().as_i32().value() == 42);
    }
    
    SECTION("i32.trunc_f32_u negative") {
        auto f32_val = Value::from_f32(-1.0f);
        auto result = TypeConverter::i32_trunc_f32_u(f32_val);
        
        REQUIRE_FALSE(result.success());
        REQUIRE(result.error().code() == error_codes::IntegerOverflow);
    }
    
    SECTION("i32.trunc_f32_u max unsigned") {
        auto f32_val = Value::from_f32(4294967295.0f);
        auto result = TypeConverter::i32_trunc_f32_u(f32_val);
        
        REQUIRE(result.success());
        REQUIRE(result.value().as_i32().value() == -1);  // 0xFFFFFFFF as signed
    }
    
    SECTION("i32.trunc_f64_s") {
        auto f64_val = Value::from_f64(42.7);
        auto result = TypeConverter::i32_trunc_f64_s(f64_val);
        
        REQUIRE(result.success());
        REQUIRE(result.value().as_i32().value() == 42);
    }
    
    SECTION("i32.trunc_f64_u") {
        auto f64_val = Value::from_f64(42.7);
        auto result = TypeConverter::i32_trunc_f64_u(f64_val);
        
        REQUIRE(result.success());
        REQUIRE(result.value().as_i32().value() == 42);
    }
}

TEST_CASE("i64 Truncation", "[types][conversions][truncation][i64]") {
    SECTION("i64.trunc_f32_s") {
        auto f32_val = Value::from_f32(42.7f);
        auto result = TypeConverter::i64_trunc_f32_s(f32_val);
        
        REQUIRE(result.success());
        REQUIRE(result.value().as_i64().value() == 42LL);
    }
    
    SECTION("i64.trunc_f32_s negative") {
        auto f32_val = Value::from_f32(-12345.8f);
        auto result = TypeConverter::i64_trunc_f32_s(f32_val);
        
        REQUIRE(result.success());
        REQUIRE(result.value().as_i64().value() == -12345LL);
    }
    
    SECTION("i64.trunc_f32_u") {
        auto f32_val = Value::from_f32(12345.8f);
        auto result = TypeConverter::i64_trunc_f32_u(f32_val);
        
        REQUIRE(result.success());
        REQUIRE(result.value().as_i64().value() == 12345LL);
    }
    
    SECTION("i64.trunc_f64_s") {
        auto f64_val = Value::from_f64(1234567890123.456);
        auto result = TypeConverter::i64_trunc_f64_s(f64_val);
        
        REQUIRE(result.success());
        REQUIRE(result.value().as_i64().value() == 1234567890123LL);
    }
    
    SECTION("i64.trunc_f64_u") {
        auto f64_val = Value::from_f64(1234567890123.456);
        auto result = TypeConverter::i64_trunc_f64_u(f64_val);
        
        REQUIRE(result.success());
        REQUIRE(result.value().as_i64().value() == 1234567890123LL);
    }
    
    SECTION("i64 truncation overflow") {
        auto large_f64 = Value::from_f64(1e20);
        auto result = TypeConverter::i64_trunc_f64_s(large_f64);
        
        REQUIRE_FALSE(result.success());
        REQUIRE(result.error().code() == error_codes::IntegerOverflow);
    }
    
    SECTION("i64 truncation NaN/infinity") {
        auto nan_f64 = Value::from_f64(std::numeric_limits<double>::quiet_NaN());
        auto result_nan = TypeConverter::i64_trunc_f64_s(nan_f64);
        REQUIRE_FALSE(result_nan.success());
        
        auto inf_f64 = Value::from_f64(std::numeric_limits<double>::infinity());
        auto result_inf = TypeConverter::i64_trunc_f64_s(inf_f64);
        REQUIRE_FALSE(result_inf.success());
    }
}

// =============================================================================
// Integer to Floating-Point Conversion Tests
// =============================================================================

TEST_CASE("Integer to Float Conversions", "[types][conversions][int2float]") {
    SECTION("f32.convert_i32_s") {
        auto i32_val = Value::from_i32(123456);
        auto result = TypeConverter::f32_convert_i32_s(i32_val);
        
        REQUIRE(result.type() == ValueType::F32);
        REQUIRE(result.as_f32().value() == 123456.0f);
    }
    
    SECTION("f32.convert_i32_s negative") {
        auto i32_val = Value::from_i32(-123456);
        auto result = TypeConverter::f32_convert_i32_s(i32_val);
        
        REQUIRE(result.type() == ValueType::F32);
        REQUIRE(result.as_f32().value() == -123456.0f);
    }
    
    SECTION("f32.convert_i32_u") {
        auto i32_val = Value::from_i32(-1);  // 0xFFFFFFFF as unsigned
        auto result = TypeConverter::f32_convert_i32_u(i32_val);
        
        REQUIRE(result.type() == ValueType::F32);
        REQUIRE(result.as_f32().value() == 4294967295.0f);
    }
    
    SECTION("f32.convert_i64_s") {
        auto i64_val = Value::from_i64(1234567890123LL);
        auto result = TypeConverter::f32_convert_i64_s(i64_val);
        
        REQUIRE(result.type() == ValueType::F32);
        // May lose precision for large values
        REQUIRE(std::abs(result.as_f32().value() - 1234567890123.0f) < 1e6f);
    }
    
    SECTION("f32.convert_i64_u") {
        auto i64_val = Value::from_i64(-1LL);  // 0xFFFFFFFFFFFFFFFF as unsigned
        auto result = TypeConverter::f32_convert_i64_u(i64_val);
        
        REQUIRE(result.type() == ValueType::F32);
        // Very large unsigned value
        REQUIRE(result.as_f32().value() > 1e19f);
    }
    
    SECTION("f64.convert_i32_s") {
        auto i32_val = Value::from_i32(123456789);
        auto result = TypeConverter::f64_convert_i32_s(i32_val);
        
        REQUIRE(result.type() == ValueType::F64);
        REQUIRE(result.as_f64().value() == 123456789.0);
    }
    
    SECTION("f64.convert_i32_u") {
        auto i32_val = Value::from_i32(-1);  // 0xFFFFFFFF as unsigned
        auto result = TypeConverter::f64_convert_i32_u(i32_val);
        
        REQUIRE(result.type() == ValueType::F64);
        REQUIRE(result.as_f64().value() == 4294967295.0);
    }
    
    SECTION("f64.convert_i64_s") {
        auto i64_val = Value::from_i64(1234567890123456LL);
        auto result = TypeConverter::f64_convert_i64_s(i64_val);
        
        REQUIRE(result.type() == ValueType::F64);
        // May lose precision for very large values
        REQUIRE(std::abs(result.as_f64().value() - 1234567890123456.0) < 1000.0);
    }
    
    SECTION("f64.convert_i64_u") {
        auto i64_val = Value::from_i64(9223372036854775807LL);  // i64 max
        auto result = TypeConverter::f64_convert_i64_u(i64_val);
        
        REQUIRE(result.type() == ValueType::F64);
        REQUIRE(result.as_f64().value() > 0.0);
    }
}

// =============================================================================
// Reinterpretation Operation Tests
// =============================================================================

TEST_CASE("Reinterpretation Operations", "[types][conversions][reinterpret]") {
    SECTION("i32.reinterpret_f32") {
        auto f32_val = Value::from_f32(1.0f);
        auto result = TypeConverter::i32_reinterpret_f32(f32_val);
        
        REQUIRE(result.type() == ValueType::I32);
        // IEEE 754 representation of 1.0f is 0x3F800000
        REQUIRE(result.as_i32().value() == 0x3F800000);
    }
    
    SECTION("f32.reinterpret_i32") {
        auto i32_val = Value::from_i32(0x3F800000);
        auto result = TypeConverter::f32_reinterpret_i32(i32_val);
        
        REQUIRE(result.type() == ValueType::F32);
        REQUIRE(result.as_f32().value() == 1.0f);
    }
    
    SECTION("i64.reinterpret_f64") {
        auto f64_val = Value::from_f64(1.0);
        auto result = TypeConverter::i64_reinterpret_f64(f64_val);
        
        REQUIRE(result.type() == ValueType::I64);
        // IEEE 754 representation of 1.0 is 0x3FF0000000000000
        REQUIRE(result.as_i64().value() == 0x3FF0000000000000LL);
    }
    
    SECTION("f64.reinterpret_i64") {
        auto i64_val = Value::from_i64(0x3FF0000000000000LL);
        auto result = TypeConverter::f64_reinterpret_i64(i64_val);
        
        REQUIRE(result.type() == ValueType::F64);
        REQUIRE(result.as_f64().value() == 1.0);
    }
    
    SECTION("Reinterpretation round trip f32") {
        auto original = Value::from_f32(3.14159f);
        auto as_int = TypeConverter::i32_reinterpret_f32(original);
        auto back_to_float = TypeConverter::f32_reinterpret_i32(as_int);
        
        REQUIRE(back_to_float.as_f32().value() == original.as_f32().value());
    }
    
    SECTION("Reinterpretation round trip f64") {
        auto original = Value::from_f64(3.141592653589793);
        auto as_int = TypeConverter::i64_reinterpret_f64(original);
        auto back_to_float = TypeConverter::f64_reinterpret_i64(as_int);
        
        REQUIRE(back_to_float.as_f64().value() == original.as_f64().value());
    }
    
    SECTION("Reinterpretation special values") {
        // Test NaN reinterpretation
        auto nan_f32 = Value::from_f32(std::numeric_limits<float>::quiet_NaN());
        auto nan_as_i32 = TypeConverter::i32_reinterpret_f32(nan_f32);
        auto back_to_nan = TypeConverter::f32_reinterpret_i32(nan_as_i32);
        REQUIRE(std::isnan(back_to_nan.as_f32().value()));
        
        // Test infinity reinterpretation
        auto inf_f32 = Value::from_f32(std::numeric_limits<float>::infinity());
        auto inf_as_i32 = TypeConverter::i32_reinterpret_f32(inf_f32);
        auto back_to_inf = TypeConverter::f32_reinterpret_i32(inf_as_i32);
        REQUIRE(std::isinf(back_to_inf.as_f32().value()));
        REQUIRE(back_to_inf.as_f32().value() > 0);
    }
}

// =============================================================================
// Generic Conversion Interface Tests
// =============================================================================

TEST_CASE("Generic Conversion Interface", "[types][conversions][generic]") {
    SECTION("Valid conversions") {
        auto i32_val = Value::from_i32(42);
        
        auto to_i64 = TypeConverter::convert(i32_val, ValueType::I64);
        REQUIRE(to_i64.success());
        REQUIRE(to_i64.value().as_i64().value() == 42);
        
        auto to_f32 = TypeConverter::convert(i32_val, ValueType::F32);
        REQUIRE(to_f32.success());
        REQUIRE(to_f32.value().as_f32().value() == 42.0f);
        
        auto to_f64 = TypeConverter::convert(i32_val, ValueType::F64);
        REQUIRE(to_f64.success());
        REQUIRE(to_f64.value().as_f64().value() == 42.0);
    }
    
    SECTION("Invalid conversions") {
        auto i32_val = Value::from_i32(42);
        auto to_funcref = TypeConverter::convert(i32_val, ValueType::FuncRef);
        
        REQUIRE_FALSE(to_funcref.success());
        REQUIRE(to_funcref.error().code() == error_codes::InvalidConversion);
    }
    
    SECTION("Same type conversion") {
        auto i32_val = Value::from_i32(42);
        auto result = TypeConverter::convert(i32_val, ValueType::I32);
        
        REQUIRE(result.success());
        REQUIRE(result.value().as_i32().value() == 42);
    }
    
    SECTION("Trapping conversions through generic interface") {
        auto f32_nan = Value::from_f32(std::numeric_limits<float>::quiet_NaN());
        auto result = TypeConverter::convert(f32_nan, ValueType::I32);
        
        REQUIRE_FALSE(result.success());
        REQUIRE(result.error().code() == error_codes::InvalidConversion);
    }
}

// =============================================================================
// Conversion Utility Function Tests
// =============================================================================

TEST_CASE("Conversion Utility Functions", "[types][conversions][utility]") {
    SECTION("is_conversion_valid") {
        REQUIRE(TypeConverter::is_conversion_valid(ValueType::I32, ValueType::I64));
        REQUIRE(TypeConverter::is_conversion_valid(ValueType::I64, ValueType::I32));
        REQUIRE(TypeConverter::is_conversion_valid(ValueType::F32, ValueType::F64));
        REQUIRE(TypeConverter::is_conversion_valid(ValueType::F64, ValueType::F32));
        REQUIRE(TypeConverter::is_conversion_valid(ValueType::I32, ValueType::F32));
        REQUIRE(TypeConverter::is_conversion_valid(ValueType::F32, ValueType::I32));
        
        // Same type is always valid
        REQUIRE(TypeConverter::is_conversion_valid(ValueType::I32, ValueType::I32));
        REQUIRE(TypeConverter::is_conversion_valid(ValueType::F64, ValueType::F64));
        
        // Invalid conversions
        REQUIRE_FALSE(TypeConverter::is_conversion_valid(ValueType::I32, ValueType::FuncRef));
        REQUIRE_FALSE(TypeConverter::is_conversion_valid(ValueType::FuncRef, ValueType::I32));
        REQUIRE_FALSE(TypeConverter::is_conversion_valid(ValueType::V128, ValueType::I32));
    }
    
    SECTION("is_conversion_lossy") {
        // Lossy conversions
        REQUIRE(TypeConverter::is_conversion_lossy(ValueType::I64, ValueType::I32));
        REQUIRE(TypeConverter::is_conversion_lossy(ValueType::I64, ValueType::F32));
        REQUIRE(TypeConverter::is_conversion_lossy(ValueType::F64, ValueType::F32));
        REQUIRE(TypeConverter::is_conversion_lossy(ValueType::F64, ValueType::I32));
        REQUIRE(TypeConverter::is_conversion_lossy(ValueType::F64, ValueType::I64));
        REQUIRE(TypeConverter::is_conversion_lossy(ValueType::I32, ValueType::F32));
        
        // Non-lossy conversions
        REQUIRE_FALSE(TypeConverter::is_conversion_lossy(ValueType::I32, ValueType::I64));
        REQUIRE_FALSE(TypeConverter::is_conversion_lossy(ValueType::I32, ValueType::F64));
        REQUIRE_FALSE(TypeConverter::is_conversion_lossy(ValueType::F32, ValueType::F64));
        REQUIRE_FALSE(TypeConverter::is_conversion_lossy(ValueType::I32, ValueType::I32));
    }
}

// =============================================================================
// IEEE 754 Compliance Tests
// =============================================================================

TEST_CASE("IEEE 754 Compliance", "[types][conversions][ieee754]") {
    SECTION("Special value constants") {
        REQUIRE(std::isnan(ieee754::FloatingPointInfo::f32_nan));
        REQUIRE(std::isinf(ieee754::FloatingPointInfo::f32_positive_infinity));
        REQUIRE(std::isinf(ieee754::FloatingPointInfo::f32_negative_infinity));
        REQUIRE(ieee754::FloatingPointInfo::f32_positive_infinity > 0);
        REQUIRE(ieee754::FloatingPointInfo::f32_negative_infinity < 0);
        
        REQUIRE(std::isnan(ieee754::FloatingPointInfo::f64_nan));
        REQUIRE(std::isinf(ieee754::FloatingPointInfo::f64_positive_infinity));
        REQUIRE(std::isinf(ieee754::FloatingPointInfo::f64_negative_infinity));
        REQUIRE(ieee754::FloatingPointInfo::f64_positive_infinity > 0);
        REQUIRE(ieee754::FloatingPointInfo::f64_negative_infinity < 0);
    }
    
    SECTION("Special value detection") {
        REQUIRE(ieee754::FloatingPointInfo::is_nan(std::numeric_limits<float>::quiet_NaN()));
        REQUIRE_FALSE(ieee754::FloatingPointInfo::is_nan(1.0f));
        REQUIRE_FALSE(ieee754::FloatingPointInfo::is_nan(std::numeric_limits<float>::infinity()));
        
        REQUIRE(ieee754::FloatingPointInfo::is_infinite(std::numeric_limits<float>::infinity()));
        REQUIRE(ieee754::FloatingPointInfo::is_infinite(-std::numeric_limits<float>::infinity()));
        REQUIRE_FALSE(ieee754::FloatingPointInfo::is_infinite(1.0f));
        REQUIRE_FALSE(ieee754::FloatingPointInfo::is_infinite(std::numeric_limits<float>::quiet_NaN()));
        
        REQUIRE(ieee754::FloatingPointInfo::is_finite(1.0f));
        REQUIRE(ieee754::FloatingPointInfo::is_finite(-1.0f));
        REQUIRE_FALSE(ieee754::FloatingPointInfo::is_finite(std::numeric_limits<float>::infinity()));
        REQUIRE_FALSE(ieee754::FloatingPointInfo::is_finite(std::numeric_limits<float>::quiet_NaN()));
    }
    
    SECTION("Conversion range limits") {
        // These constants should be within the valid ranges
        REQUIRE(ieee754::FloatingPointInfo::max_i32_as_f32 > 0);
        REQUIRE(ieee754::FloatingPointInfo::min_i32_as_f32 < 0);
        REQUIRE(ieee754::FloatingPointInfo::max_i32_as_f64 > 0);
        REQUIRE(ieee754::FloatingPointInfo::min_i32_as_f64 < 0);
        
        // Test that the limits are reasonable
        REQUIRE(ieee754::FloatingPointInfo::max_i32_as_f32 > 2e9f);
        REQUIRE(ieee754::FloatingPointInfo::min_i32_as_f32 < -2e9f);
        REQUIRE(ieee754::FloatingPointInfo::max_i32_as_f64 > 2e9);
        REQUIRE(ieee754::FloatingPointInfo::min_i32_as_f64 < -2e9);
    }
}

// =============================================================================
// Zero-Overhead Template Operations Tests
// =============================================================================

TEST_CASE("Zero-Overhead Template Operations", "[types][conversions][templates]") {
    SECTION("Typed conversion operations") {
        using ops_i32_to_i64 = conversions::optimized::TypedConversion<ValueType::I32, ValueType::I64>;
        using ops_i64_to_i32 = conversions::optimized::TypedConversion<ValueType::I64, ValueType::I32>;
        
        static_assert(ops_i32_to_i64::is_valid);
        static_assert(ops_i64_to_i32::is_valid);
        
        auto i32_value = Value::from_i32(42);
        auto i64_result = ops_i32_to_i64::convert(i32_value);
        REQUIRE(i64_result.type() == ValueType::I64);
        REQUIRE(i64_result.as_i64().value() == 42);
        
        auto i64_value = Value::from_i64(0x123456789ABCDEF0LL);
        auto i32_result = ops_i64_to_i32::convert(i64_value);
        REQUIRE(i32_result.type() == ValueType::I32);
        REQUIRE(i32_result.as_i32().value() == static_cast<int32_t>(0x9ABCDEF0));
    }
    
    SECTION("Template conversion dispatch") {
        auto i32_value = Value::from_i32(42);
        
        auto to_i64 = conversions::optimized::convert_to<ValueType::I64>(i32_value);
        REQUIRE(to_i64.success());
        REQUIRE(to_i64.value().as_i64().value() == 42);
        
        auto f32_value = Value::from_f32(3.14f);
        auto to_f64 = conversions::optimized::convert_to<ValueType::F64>(f32_value);
        REQUIRE(to_f64.success());
        REQUIRE(to_f64.value().as_f64().value() == static_cast<double>(3.14f));
    }
}

// =============================================================================
// Performance Tests
// =============================================================================

TEST_CASE("Conversion Performance", "[types][conversions][performance]") {
    SECTION("Simple conversion performance") {
        constexpr int iterations = 10000;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            auto i32_val = Value::from_i32(i);
            volatile auto i64_val = TypeConverter::i64_extend_i32_s(i32_val);
            (void)i64_val;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        auto ns_per_op = duration.count() / static_cast<double>(iterations);
        
        INFO("Integer conversion: " << ns_per_op << "ns per operation");
        
        // Should be very fast (allow generous margin for test overhead)
        REQUIRE(ns_per_op < 1000.0);
    }
    
    SECTION("Reinterpretation performance") {
        constexpr int iterations = 10000;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            auto f32_val = Value::from_f32(static_cast<float>(i));
            volatile auto i32_val = TypeConverter::i32_reinterpret_f32(f32_val);
            (void)i32_val;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        auto ns_per_op = duration.count() / static_cast<double>(iterations);
        
        INFO("Reinterpretation: " << ns_per_op << "ns per operation");
        
        // Reinterpretation should be extremely fast
        REQUIRE(ns_per_op < 100.0);
    }
    
    SECTION("Truncation performance") {
        constexpr int iterations = 1000;  // Fewer iterations since truncation is more complex
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            auto f32_val = Value::from_f32(static_cast<float>(i) + 0.5f);
            volatile auto result = TypeConverter::i32_trunc_f32_s(f32_val);
            (void)result;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        auto ns_per_op = duration.count() / static_cast<double>(iterations);
        
        INFO("Truncation: " << ns_per_op << "ns per operation");
        
        // Truncation is more complex but should still be reasonable
        REQUIRE(ns_per_op < 5000.0);
    }
}

// =============================================================================
// WebAssembly Specification Compliance Tests
// =============================================================================

TEST_CASE("WebAssembly Spec Compliance - Conversions", "[types][conversions][spec][compliance]") {
    SECTION("Integer wrap behavior") {
        // Test WebAssembly i32.wrap_i64 exact behavior
        auto max_u32_as_i64 = Value::from_i64(0x00000000FFFFFFFFLL);
        auto wrapped = TypeConverter::i32_wrap_i64(max_u32_as_i64);
        REQUIRE(wrapped.as_i32().value() == -1);  // 0xFFFFFFFF as signed i32
        
        auto test_val = Value::from_i64(0x123456789ABCDEF0LL);
        auto wrapped2 = TypeConverter::i32_wrap_i64(test_val);
        REQUIRE(wrapped2.as_i32().value() == static_cast<int32_t>(0x9ABCDEF0));
    }
    
    SECTION("Sign extension behavior") {
        // Test WebAssembly i64.extend_i32_s exact behavior
        auto negative_i32 = Value::from_i32(-1);
        auto extended = TypeConverter::i64_extend_i32_s(negative_i32);
        REQUIRE(extended.as_i64().value() == -1LL);
        
        auto positive_i32 = Value::from_i32(0x7FFFFFFF);
        auto extended2 = TypeConverter::i64_extend_i32_s(positive_i32);
        REQUIRE(extended2.as_i64().value() == 0x000000007FFFFFFFLL);
    }
    
    SECTION("Zero extension behavior") {
        // Test WebAssembly i64.extend_i32_u exact behavior
        auto negative_i32 = Value::from_i32(-1);  // 0xFFFFFFFF
        auto extended = TypeConverter::i64_extend_i32_u(negative_i32);
        REQUIRE(extended.as_i64().value() == 0x00000000FFFFFFFFLL);
        
        auto positive_i32 = Value::from_i32(0x12345678);
        auto extended2 = TypeConverter::i64_extend_i32_u(positive_i32);
        REQUIRE(extended2.as_i64().value() == 0x0000000012345678LL);
    }
    
    SECTION("IEEE 754 promotion/demotion") {
        // f64.promote_f32 should be exact
        auto f32_val = Value::from_f32(3.14159f);
        auto promoted = TypeConverter::f64_promote_f32(f32_val);
        REQUIRE(promoted.as_f64().value() == static_cast<double>(3.14159f));
        
        // f32.demote_f64 may lose precision
        auto f64_val = Value::from_f64(3.141592653589793);
        auto demoted = TypeConverter::f32_demote_f64(f64_val);
        REQUIRE(demoted.type() == ValueType::F32);
        // Should be approximately equal within f32 precision
        REQUIRE(std::abs(demoted.as_f32().value() - 3.14159f) < 1e-5f);
    }
    
    SECTION("Truncation trap behavior") {
        // NaN should trap
        auto nan_f32 = Value::from_f32(std::numeric_limits<float>::quiet_NaN());
        auto result_nan = TypeConverter::i32_trunc_f32_s(nan_f32);
        REQUIRE_FALSE(result_nan.success());
        
        // Infinity should trap
        auto inf_f32 = Value::from_f32(std::numeric_limits<float>::infinity());
        auto result_inf = TypeConverter::i32_trunc_f32_s(inf_f32);
        REQUIRE_FALSE(result_inf.success());
        
        // Out of range should trap
        auto large_f32 = Value::from_f32(3e9f);
        auto result_large = TypeConverter::i32_trunc_f32_s(large_f32);
        REQUIRE_FALSE(result_large.success());
    }
    
    SECTION("Reinterpretation bit preservation") {
        // Test that reinterpretation preserves exact bit patterns
        
        // Test with known IEEE 754 bit patterns
        auto i32_bits = Value::from_i32(0x3F800000);  // IEEE 754 for 1.0f
        auto as_float = TypeConverter::f32_reinterpret_i32(i32_bits);
        REQUIRE(as_float.as_f32().value() == 1.0f);
        
        auto back_to_int = TypeConverter::i32_reinterpret_f32(as_float);
        REQUIRE(back_to_int.as_i32().value() == 0x3F800000);
        
        // Test with f64
        auto i64_bits = Value::from_i64(0x3FF0000000000000LL);  // IEEE 754 for 1.0
        auto as_double = TypeConverter::f64_reinterpret_i64(i64_bits);
        REQUIRE(as_double.as_f64().value() == 1.0);
        
        auto back_to_i64 = TypeConverter::i64_reinterpret_f64(as_double);
        REQUIRE(back_to_i64.as_i64().value() == 0x3FF0000000000000LL);
    }
}

// =============================================================================
// Platform-Specific Tests
// =============================================================================

TEST_CASE("Platform-Specific Conversion Behavior", "[types][conversions][platform]") {
    SECTION("All platforms should produce identical results") {
        // Test values that should produce identical results across platforms
        auto test_i32 = Value::from_i32(12345);
        auto test_i64 = Value::from_i64(-9876543210LL);
        auto test_f32 = Value::from_f32(3.14159f);
        auto test_f64 = Value::from_f64(2.718281828);
        
        // These conversions should be identical across platforms
        auto i32_to_i64 = TypeConverter::i64_extend_i32_s(test_i32);
        REQUIRE(i32_to_i64.as_i64().value() == 12345LL);
        
        auto i64_to_i32 = TypeConverter::i32_wrap_i64(test_i64);
        REQUIRE(i64_to_i32.type() == ValueType::I32);
        
        auto f32_to_f64 = TypeConverter::f64_promote_f32(test_f32);
        REQUIRE(f32_to_f64.as_f64().value() == static_cast<double>(3.14159f));
        
        auto f64_to_f32 = TypeConverter::f32_demote_f64(test_f64);
        REQUIRE(f64_to_f32.type() == ValueType::F32);
    }
    
    SECTION("Platform optimizations should be transparent") {
        // Test that platform-specific optimizations don't change behavior
        auto test_val = Value::from_f32(42.7f);
        auto result = TypeConverter::i32_trunc_f32_s(test_val);
        
        REQUIRE(result.success());
        REQUIRE(result.value().as_i32().value() == 42);
        
        // This should work the same regardless of platform optimization
    }
}

// =============================================================================
// Edge Case and Boundary Tests
// =============================================================================

TEST_CASE("Conversion Edge Cases", "[types][conversions][edge]") {
    SECTION("Boundary value conversions") {
        // Test i32 min/max
        auto i32_max = Value::from_i32(INT32_MAX);
        auto i32_min = Value::from_i32(INT32_MIN);
        
        auto max_to_i64 = TypeConverter::i64_extend_i32_s(i32_max);
        REQUIRE(max_to_i64.as_i64().value() == INT32_MAX);
        
        auto min_to_i64 = TypeConverter::i64_extend_i32_s(i32_min);
        REQUIRE(min_to_i64.as_i64().value() == INT32_MIN);
        
        // Test i64 to i32 wrapping
        auto i64_max = Value::from_i64(INT64_MAX);
        auto wrapped_max = TypeConverter::i32_wrap_i64(i64_max);
        REQUIRE(wrapped_max.as_i32().value() == -1);  // Wraps around
        
        auto i64_min = Value::from_i64(INT64_MIN);
        auto wrapped_min = TypeConverter::i32_wrap_i64(i64_min);
        REQUIRE(wrapped_min.as_i32().value() == 0);   // Wraps around
    }
    
    SECTION("Zero and negative zero") {
        auto pos_zero = Value::from_f32(0.0f);
        auto neg_zero = Value::from_f32(-0.0f);
        
        auto pos_zero_to_i32 = TypeConverter::i32_trunc_f32_s(pos_zero);
        auto neg_zero_to_i32 = TypeConverter::i32_trunc_f32_s(neg_zero);
        
        REQUIRE(pos_zero_to_i32.success());
        REQUIRE(neg_zero_to_i32.success());
        REQUIRE(pos_zero_to_i32.value().as_i32().value() == 0);
        REQUIRE(neg_zero_to_i32.value().as_i32().value() == 0);
    }
    
    SECTION("Very small floating-point values") {
        auto tiny_f32 = Value::from_f32(1e-10f);
        auto tiny_f64 = Value::from_f64(1e-100);
        
        auto tiny_f32_to_i32 = TypeConverter::i32_trunc_f32_s(tiny_f32);
        auto tiny_f64_to_i32 = TypeConverter::i32_trunc_f64_s(tiny_f64);
        
        REQUIRE(tiny_f32_to_i32.success());
        REQUIRE(tiny_f64_to_i32.success());
        REQUIRE(tiny_f32_to_i32.value().as_i32().value() == 0);  // Truncates to 0
        REQUIRE(tiny_f64_to_i32.value().as_i32().value() == 0);  // Truncates to 0
    }
}
