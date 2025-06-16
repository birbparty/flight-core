/**
 * @file conversions.cpp
 * @brief WebAssembly Type Conversion System Implementation
 * 
 * Implementation of complex conversion operations including truncation
 * operations with trap behavior and integer-to-float conversions.
 */

#include <flight/wasm/types/conversions.hpp>
#include <cmath>

namespace flight::wasm::conversions {

    // =========================================================================
    // Generic Conversion Interface Implementation
    // =========================================================================

    Result<Value> TypeConverter::convert(const Value& value, ValueType target_type) noexcept {
        // Early return if already the target type
        if (value.type() == target_type) {
            return Result<Value>{value};
        }
        
        // Dispatch to specific conversion functions
        switch (value.type()) {
            case ValueType::I32:
                switch (target_type) {
                    case ValueType::I64:
                        return Result<Value>{i64_extend_i32_s(value)};  // Default to signed extension
                    case ValueType::F32:
                        return Result<Value>{f32_convert_i32_s(value)};
                    case ValueType::F64:
                        return Result<Value>{f64_convert_i32_s(value)};
                    default:
                        break;
                }
                break;
                
            case ValueType::I64:
                switch (target_type) {
                    case ValueType::I32:
                        return Result<Value>{i32_wrap_i64(value)};
                    case ValueType::F32:
                        return Result<Value>{f32_convert_i64_s(value)};
                    case ValueType::F64:
                        return Result<Value>{f64_convert_i64_s(value)};
                    default:
                        break;
                }
                break;
                
            case ValueType::F32:
                switch (target_type) {
                    case ValueType::I32:
                        return i32_trunc_f32_s(value);  // May trap
                    case ValueType::I64:
                        return i64_trunc_f32_s(value);  // May trap
                    case ValueType::F64:
                        return Result<Value>{f64_promote_f32(value)};
                    default:
                        break;
                }
                break;
                
            case ValueType::F64:
                switch (target_type) {
                    case ValueType::I32:
                        return i32_trunc_f64_s(value);  // May trap
                    case ValueType::I64:
                        return i64_trunc_f64_s(value);  // May trap
                    case ValueType::F32:
                        return Result<Value>{f32_demote_f64(value)};
                    default:
                        break;
                }
                break;
                
            default:
                break;
        }
        
        return Result<Value>{Error{error_codes::InvalidConversion, 
            "No conversion available between the specified types"}};
    }

    bool TypeConverter::is_conversion_valid(ValueType from, ValueType to) noexcept {
        // Check if a conversion exists
        if (from == to) return true;
        
        // All number types can be converted to each other
        return is_number_type(from) && is_number_type(to);
    }

    bool TypeConverter::is_conversion_lossy(ValueType from, ValueType to) noexcept {
        // Check if conversion may lose precision or range
        switch (from) {
            case ValueType::I64:
                return (to == ValueType::I32) || (to == ValueType::F32);
            case ValueType::F64:
                return (to != ValueType::F64);
            case ValueType::I32:
                return (to == ValueType::F32);  // f32 cannot represent all i32 values exactly
            default:
                return false;
        }
    }

    // =========================================================================
    // Truncation Operations Implementation (Can Trap)
    // =========================================================================

    Result<Value> TypeConverter::i32_trunc_f32_s(const Value& value) noexcept {
        auto f32_val = value.as_f32().value();
        
        #ifdef FLIGHT_WASM_PLATFORM_VITA
            return platform_conversions::vita::optimized_i32_trunc_f32_s(value);
        #else
            // Check for NaN
            if (ieee754::FloatingPointInfo::is_nan(f32_val)) {
                return Result<Value>{Error{error_codes::InvalidConversion, 
                    "Cannot convert NaN to integer"}};
            }
            
            // Check for infinity
            if (ieee754::FloatingPointInfo::is_infinite(f32_val)) {
                return Result<Value>{Error{error_codes::InvalidConversion, 
                    "Cannot convert infinity to integer"}};
            }
            
            // Check range for signed i32
            if (f32_val < -2147483648.0f || f32_val > 2147483647.0f) {
                return Result<Value>{Error{error_codes::IntegerOverflow, 
                    "Float value out of i32 range"}};
            }
            
            // Truncate toward zero
            int32_t result = static_cast<int32_t>(std::trunc(f32_val));
            return Result<Value>{Value::from_i32(result)};
        #endif
    }

    Result<Value> TypeConverter::i32_trunc_f32_u(const Value& value) noexcept {
        auto f32_val = value.as_f32().value();
        
        // Check for NaN
        if (ieee754::FloatingPointInfo::is_nan(f32_val)) {
            return Result<Value>{Error{error_codes::InvalidConversion, 
                "Cannot convert NaN to integer"}};
        }
        
        // Check for infinity
        if (ieee754::FloatingPointInfo::is_infinite(f32_val)) {
            return Result<Value>{Error{error_codes::InvalidConversion, 
                "Cannot convert infinity to integer"}};
        }
        
        // Check range for unsigned i32 (stored as signed)
        if (f32_val < 0.0f || f32_val > 4294967295.0f) {
            return Result<Value>{Error{error_codes::IntegerOverflow, 
                "Float value out of u32 range"}};
        }
        
        // Truncate toward zero and reinterpret as signed
        uint32_t result = static_cast<uint32_t>(std::trunc(f32_val));
        return Result<Value>{Value::from_i32(static_cast<int32_t>(result))};
    }

    Result<Value> TypeConverter::i32_trunc_f64_s(const Value& value) noexcept {
        auto f64_val = value.as_f64().value();
        
        // Check for NaN
        if (ieee754::FloatingPointInfo::is_nan(f64_val)) {
            return Result<Value>{Error{error_codes::InvalidConversion, 
                "Cannot convert NaN to integer"}};
        }
        
        // Check for infinity
        if (ieee754::FloatingPointInfo::is_infinite(f64_val)) {
            return Result<Value>{Error{error_codes::InvalidConversion, 
                "Cannot convert infinity to integer"}};
        }
        
        // Check range for signed i32
        if (f64_val < -2147483648.0 || f64_val > 2147483647.0) {
            return Result<Value>{Error{error_codes::IntegerOverflow, 
                "Double value out of i32 range"}};
        }
        
        // Truncate toward zero
        int32_t result = static_cast<int32_t>(std::trunc(f64_val));
        return Result<Value>{Value::from_i32(result)};
    }

    Result<Value> TypeConverter::i32_trunc_f64_u(const Value& value) noexcept {
        auto f64_val = value.as_f64().value();
        
        // Check for NaN
        if (ieee754::FloatingPointInfo::is_nan(f64_val)) {
            return Result<Value>{Error{error_codes::InvalidConversion, 
                "Cannot convert NaN to integer"}};
        }
        
        // Check for infinity
        if (ieee754::FloatingPointInfo::is_infinite(f64_val)) {
            return Result<Value>{Error{error_codes::InvalidConversion, 
                "Cannot convert infinity to integer"}};
        }
        
        // Check range for unsigned i32 (stored as signed)
        if (f64_val < 0.0 || f64_val > 4294967295.0) {
            return Result<Value>{Error{error_codes::IntegerOverflow, 
                "Double value out of u32 range"}};
        }
        
        // Truncate toward zero and reinterpret as signed
        uint32_t result = static_cast<uint32_t>(std::trunc(f64_val));
        return Result<Value>{Value::from_i32(static_cast<int32_t>(result))};
    }

    Result<Value> TypeConverter::i64_trunc_f32_s(const Value& value) noexcept {
        auto f32_val = value.as_f32().value();
        
        if (ieee754::FloatingPointInfo::is_nan(f32_val) || ieee754::FloatingPointInfo::is_infinite(f32_val)) {
            return Result<Value>{Error{error_codes::InvalidConversion, 
                "Cannot convert NaN or infinity to integer"}};
        }
        
        // Check range for signed i64
        if (f32_val < ieee754::FloatingPointInfo::min_i64_as_f32 || 
            f32_val > ieee754::FloatingPointInfo::max_i64_as_f32) {
            return Result<Value>{Error{error_codes::IntegerOverflow, 
                "Float value out of i64 range"}};
        }
        
        int64_t result = static_cast<int64_t>(std::trunc(f32_val));
        return Result<Value>{Value::from_i64(result)};
    }

    Result<Value> TypeConverter::i64_trunc_f32_u(const Value& value) noexcept {
        auto f32_val = value.as_f32().value();
        
        if (ieee754::FloatingPointInfo::is_nan(f32_val) || ieee754::FloatingPointInfo::is_infinite(f32_val)) {
            return Result<Value>{Error{error_codes::InvalidConversion, 
                "Cannot convert NaN or infinity to integer"}};
        }
        
        // Check range for unsigned i64 (stored as signed)
        if (f32_val < 0.0f || f32_val > ieee754::FloatingPointInfo::max_u64_as_f32) {
            return Result<Value>{Error{error_codes::IntegerOverflow, 
                "Float value out of u64 range"}};
        }
        
        uint64_t result = static_cast<uint64_t>(std::trunc(f32_val));
        return Result<Value>{Value::from_i64(static_cast<int64_t>(result))};
    }

    Result<Value> TypeConverter::i64_trunc_f64_s(const Value& value) noexcept {
        auto f64_val = value.as_f64().value();
        
        if (ieee754::FloatingPointInfo::is_nan(f64_val) || ieee754::FloatingPointInfo::is_infinite(f64_val)) {
            return Result<Value>{Error{error_codes::InvalidConversion, 
                "Cannot convert NaN or infinity to integer"}};
        }
        
        // Check range for signed i64
        if (f64_val < ieee754::FloatingPointInfo::min_i64_as_f64 || 
            f64_val > ieee754::FloatingPointInfo::max_i64_as_f64) {
            return Result<Value>{Error{error_codes::IntegerOverflow, 
                "Double value out of i64 range"}};
        }
        
        int64_t result = static_cast<int64_t>(std::trunc(f64_val));
        return Result<Value>{Value::from_i64(result)};
    }

    Result<Value> TypeConverter::i64_trunc_f64_u(const Value& value) noexcept {
        auto f64_val = value.as_f64().value();
        
        if (ieee754::FloatingPointInfo::is_nan(f64_val) || ieee754::FloatingPointInfo::is_infinite(f64_val)) {
            return Result<Value>{Error{error_codes::InvalidConversion, 
                "Cannot convert NaN or infinity to integer"}};
        }
        
        // Check range for unsigned i64 (stored as signed)
        if (f64_val < 0.0 || f64_val > ieee754::FloatingPointInfo::max_u64_as_f64) {
            return Result<Value>{Error{error_codes::IntegerOverflow, 
                "Double value out of u64 range"}};
        }
        
        uint64_t result = static_cast<uint64_t>(std::trunc(f64_val));
        return Result<Value>{Value::from_i64(static_cast<int64_t>(result))};
    }

    // =========================================================================
    // Integer to Floating-Point Conversions Implementation
    // =========================================================================

    Value TypeConverter::f32_convert_i32_s(const Value& value) noexcept {
        #ifdef FLIGHT_WASM_PLATFORM_VITA
            return platform_conversions::vita::optimized_f32_convert_i32_s(value);
        #else
            auto i32_val = value.as_i32().value();
            float result = static_cast<float>(i32_val);
            return Value::from_f32(result);
        #endif
    }

    Value TypeConverter::f32_convert_i32_u(const Value& value) noexcept {
        auto i32_val = value.as_i32().value();
        
        // Treat as unsigned
        uint32_t unsigned_val = static_cast<uint32_t>(i32_val);
        float result = static_cast<float>(unsigned_val);
        return Value::from_f32(result);
    }

    Value TypeConverter::f32_convert_i64_s(const Value& value) noexcept {
        auto i64_val = value.as_i64().value();
        
        // Note: May lose precision for large i64 values
        float result = static_cast<float>(i64_val);
        return Value::from_f32(result);
    }

    Value TypeConverter::f32_convert_i64_u(const Value& value) noexcept {
        auto i64_val = value.as_i64().value();
        
        // Treat as unsigned
        uint64_t unsigned_val = static_cast<uint64_t>(i64_val);
        float result = static_cast<float>(unsigned_val);
        return Value::from_f32(result);
    }

    Value TypeConverter::f64_convert_i32_s(const Value& value) noexcept {
        auto i32_val = value.as_i32().value();
        double result = static_cast<double>(i32_val);
        return Value::from_f64(result);
    }

    Value TypeConverter::f64_convert_i32_u(const Value& value) noexcept {
        auto i32_val = value.as_i32().value();
        
        // Treat as unsigned
        uint32_t unsigned_val = static_cast<uint32_t>(i32_val);
        double result = static_cast<double>(unsigned_val);
        return Value::from_f64(result);
    }

    Value TypeConverter::f64_convert_i64_s(const Value& value) noexcept {
        auto i64_val = value.as_i64().value();
        
        // Note: May lose precision for large i64 values
        double result = static_cast<double>(i64_val);
        return Value::from_f64(result);
    }

    Value TypeConverter::f64_convert_i64_u(const Value& value) noexcept {
        auto i64_val = value.as_i64().value();
        
        // Treat as unsigned
        uint64_t unsigned_val = static_cast<uint64_t>(i64_val);
        double result = static_cast<double>(unsigned_val);
        return Value::from_f64(result);
    }

} // namespace flight::wasm::conversions
