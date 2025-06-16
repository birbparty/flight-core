#ifndef FLIGHT_WASM_TYPES_CONVERSIONS_HPP
#define FLIGHT_WASM_TYPES_CONVERSIONS_HPP

/**
 * @file conversions.hpp
 * @brief WebAssembly Type Conversion System
 * 
 * This header implements the complete WebAssembly type conversion system with
 * exact specification compliance. It provides all conversion instructions
 * including integer conversions, floating-point conversions, truncation
 * operations, and reinterpretation operations with platform-specific
 * optimizations for embedded systems.
 */

#include <flight/wasm/types/value.hpp>
#include <flight/wasm/types/values.hpp>
#include <flight/wasm/utilities/error.hpp>
#include <flight/wasm/utilities/platform.hpp>
#include <cmath>
#include <limits>
#include <cstring>

namespace flight::wasm::conversions {

    // Forward declarations for error codes
    namespace error_codes {
        constexpr ErrorCode InvalidConversion = ErrorCode::TypeMismatch;
        constexpr ErrorCode IntegerOverflow = static_cast<ErrorCode>(0x2010);
        constexpr ErrorCode ConversionTrap = static_cast<ErrorCode>(0x2011);
    }

    // =========================================================================
    // IEEE 754 Compliance Framework
    // =========================================================================

    namespace ieee754 {
        /**
         * @brief IEEE 754 rounding modes for conversions
         */
        enum class RoundingMode {
            ToNearest,      // Round to nearest, ties to even (default)
            ToZero,         // Round toward zero (truncate)
            ToPositive,     // Round toward positive infinity
            ToNegative      // Round toward negative infinity
        };

        /**
         * @brief IEEE 754 special value handling and conversion limits
         */
        struct FloatingPointInfo {
            // IEEE 754 special values
            static constexpr float f32_positive_infinity = std::numeric_limits<float>::infinity();
            static constexpr float f32_negative_infinity = -std::numeric_limits<float>::infinity();
            static constexpr float f32_nan = std::numeric_limits<float>::quiet_NaN();
            
            static constexpr double f64_positive_infinity = std::numeric_limits<double>::infinity();
            static constexpr double f64_negative_infinity = -std::numeric_limits<double>::infinity();
            static constexpr double f64_nan = std::numeric_limits<double>::quiet_NaN();
            
            // Conversion range limits for WebAssembly spec compliance
            static constexpr float max_i32_as_f32 = 2147483520.0f;  // Largest i32 exactly representable in f32
            static constexpr float min_i32_as_f32 = -2147483648.0f; // Smallest i32 exactly representable in f32
            static constexpr double max_i32_as_f64 = 2147483647.0;  // i32 max exactly representable in f64
            static constexpr double min_i32_as_f64 = -2147483648.0; // i32 min exactly representable in f64
            
            static constexpr float max_i64_as_f32 = 9223371487098961920.0f;  // Largest i64 exactly representable in f32
            static constexpr float min_i64_as_f32 = -9223372036854775808.0f; // Smallest i64 exactly representable in f32
            static constexpr double max_i64_as_f64 = 9223372036854774784.0;  // Largest i64 exactly representable in f64
            static constexpr double min_i64_as_f64 = -9223372036854775808.0; // Smallest i64 exactly representable in f64
            
            // Unsigned conversion limits
            static constexpr float max_u32_as_f32 = 4294967040.0f;  // Largest u32 exactly representable in f32
            static constexpr double max_u32_as_f64 = 4294967295.0;  // u32 max exactly representable in f64
            static constexpr float max_u64_as_f32 = 18446742974197923840.0f; // Largest u64 exactly representable in f32
            static constexpr double max_u64_as_f64 = 18446744073709549568.0; // Largest u64 exactly representable in f64
            
            // Special value checking
            static bool is_nan(float value) noexcept { return std::isnan(value); }
            static bool is_nan(double value) noexcept { return std::isnan(value); }
            static bool is_infinite(float value) noexcept { return std::isinf(value); }
            static bool is_infinite(double value) noexcept { return std::isinf(value); }
            static bool is_finite(float value) noexcept { return std::isfinite(value); }
            static bool is_finite(double value) noexcept { return std::isfinite(value); }
        };
    }

    // =========================================================================
    // Platform-Specific Optimization Framework
    // =========================================================================

    namespace platform_conversions {
        #ifdef FLIGHT_WASM_PLATFORM_DREAMCAST
            namespace dreamcast {
                // SH-4 has limited 64-bit support - optimize i64 conversions
                inline Value optimized_i64_extend_i32_s(const Value& value) noexcept {
                    auto i32_val = value.as_i32().value();
                    // Use efficient sign extension
                    int64_t result = static_cast<int64_t>(i32_val);
                    return Value::from_i64(result);
                }
                
                // Optimize f64 operations for limited FPU
                inline Value optimized_f64_promote_f32(const Value& value) noexcept {
                    auto f32_val = value.as_f32().value();
                    // SH-4 FPU can handle this efficiently
                    double result = static_cast<double>(f32_val);
                    return Value::from_f64(result);
                }
                
                // Use integer operations for f64 when beneficial
                inline Value optimized_f32_demote_f64(const Value& value) noexcept {
                    auto f64_val = value.as_f64().value();
                    
                    // Check for special values first
                    if (ieee754::FloatingPointInfo::is_nan(f64_val)) {
                        return Value::from_f32(ieee754::FloatingPointInfo::f32_nan);
                    }
                    if (ieee754::FloatingPointInfo::is_infinite(f64_val)) {
                        return Value::from_f32(f64_val > 0 ? 
                            ieee754::FloatingPointInfo::f32_positive_infinity :
                            ieee754::FloatingPointInfo::f32_negative_infinity);
                    }
                    
                    // Standard conversion with rounding
                    float result = static_cast<float>(f64_val);
                    return Value::from_f32(result);
                }
            }
        #endif
        
        #ifdef FLIGHT_WASM_PLATFORM_PSP
            namespace psp {
                // MIPS has efficient bit manipulation for reinterpretation
                inline Value optimized_i32_reinterpret_f32(const Value& value) noexcept {
                    auto f32_val = value.as_f32().value();
                    
                    int32_t result;
                    std::memcpy(&result, &f32_val, sizeof(int32_t));
                    return Value::from_i32(result);
                }
                
                inline Value optimized_f32_reinterpret_i32(const Value& value) noexcept {
                    auto i32_val = value.as_i32().value();
                    
                    float result;
                    std::memcpy(&result, &i32_val, sizeof(float));
                    return Value::from_f32(result);
                }
            }
        #endif
        
        #ifdef FLIGHT_WASM_PLATFORM_VITA
            namespace vita {
                // ARM has efficient conversion instructions
                inline Value optimized_f32_convert_i32_s(const Value& value) noexcept {
                    auto i32_val = value.as_i32().value();
                    
                    // ARM VFP handles this efficiently
                    float result = static_cast<float>(i32_val);
                    return Value::from_f32(result);
                }
                
                // Use ARM rounding instructions when available
                inline Result<Value> optimized_i32_trunc_f32_s(const Value& value) noexcept {
                    auto f32_val = value.as_f32().value();
                    
                    // Check for NaN and infinity
                    if (ieee754::FloatingPointInfo::is_nan(f32_val) || 
                        ieee754::FloatingPointInfo::is_infinite(f32_val)) {
                        return Result<Value>{Error{error_codes::InvalidConversion, 
                            "Cannot convert NaN or infinity to integer"}};
                    }
                    
                    // Check range
                    if (f32_val < ieee754::FloatingPointInfo::min_i32_as_f32 || 
                        f32_val > ieee754::FloatingPointInfo::max_i32_as_f32) {
                        return Result<Value>{Error{error_codes::IntegerOverflow, 
                            "Float value out of i32 range"}};
                    }
                    
                    // ARM efficient truncation
                    int32_t result = static_cast<int32_t>(f32_val);
                    return Result<Value>{Value::from_i32(result)};
                }
            }
        #endif
    }

    // =========================================================================
    // Main Type Converter Interface
    // =========================================================================

    /**
     * @brief Main WebAssembly type conversion interface
     * 
     * This class provides all WebAssembly type conversion operations with
     * exact specification compliance, optimal performance, and platform-specific
     * optimizations.
     */
    class TypeConverter {
    public:
        // =====================================================================
        // Generic Conversion Interface
        // =====================================================================

        /**
         * @brief Generic conversion interface with automatic dispatch
         */
        static Result<Value> convert(const Value& value, ValueType target_type) noexcept;

        /**
         * @brief Check if conversion is valid between types
         */
        static bool is_conversion_valid(ValueType from, ValueType to) noexcept;

        /**
         * @brief Check if conversion may lose precision or range
         */
        static bool is_conversion_lossy(ValueType from, ValueType to) noexcept;

        // =====================================================================
        // Integer Conversion Operations (WebAssembly Section 4.3.2.1)
        // =====================================================================

        /**
         * @brief i32.wrap_i64: wrap i64 to i32 (truncate high bits)
         */
        static Value i32_wrap_i64(const Value& value) noexcept;

        /**
         * @brief i64.extend_i32_s: sign-extend i32 to i64
         */
        static Value i64_extend_i32_s(const Value& value) noexcept;

        /**
         * @brief i64.extend_i32_u: zero-extend i32 to i64
         */
        static Value i64_extend_i32_u(const Value& value) noexcept;

        // =====================================================================
        // Truncation Operations (Can Trap)
        // =====================================================================

        /**
         * @brief i32.trunc_f32_s: truncate f32 to signed i32
         */
        static Result<Value> i32_trunc_f32_s(const Value& value) noexcept;

        /**
         * @brief i32.trunc_f32_u: truncate f32 to unsigned i32
         */
        static Result<Value> i32_trunc_f32_u(const Value& value) noexcept;

        /**
         * @brief i32.trunc_f64_s: truncate f64 to signed i32
         */
        static Result<Value> i32_trunc_f64_s(const Value& value) noexcept;

        /**
         * @brief i32.trunc_f64_u: truncate f64 to unsigned i32
         */
        static Result<Value> i32_trunc_f64_u(const Value& value) noexcept;

        /**
         * @brief i64.trunc_f32_s: truncate f32 to signed i64
         */
        static Result<Value> i64_trunc_f32_s(const Value& value) noexcept;

        /**
         * @brief i64.trunc_f32_u: truncate f32 to unsigned i64
         */
        static Result<Value> i64_trunc_f32_u(const Value& value) noexcept;

        /**
         * @brief i64.trunc_f64_s: truncate f64 to signed i64
         */
        static Result<Value> i64_trunc_f64_s(const Value& value) noexcept;

        /**
         * @brief i64.trunc_f64_u: truncate f64 to unsigned i64
         */
        static Result<Value> i64_trunc_f64_u(const Value& value) noexcept;

        // =====================================================================
        // Floating-Point Conversion Operations (WebAssembly Section 4.3.2.2)
        // =====================================================================

        /**
         * @brief f32.demote_f64: demote f64 to f32 with rounding
         */
        static Value f32_demote_f64(const Value& value) noexcept;

        /**
         * @brief f64.promote_f32: promote f32 to f64 (exact conversion)
         */
        static Value f64_promote_f32(const Value& value) noexcept;

        // =====================================================================
        // Integer to Floating-Point Conversions
        // =====================================================================

        /**
         * @brief f32.convert_i32_s: convert signed i32 to f32
         */
        static Value f32_convert_i32_s(const Value& value) noexcept;

        /**
         * @brief f32.convert_i32_u: convert unsigned i32 to f32
         */
        static Value f32_convert_i32_u(const Value& value) noexcept;

        /**
         * @brief f32.convert_i64_s: convert signed i64 to f32
         */
        static Value f32_convert_i64_s(const Value& value) noexcept;

        /**
         * @brief f32.convert_i64_u: convert unsigned i64 to f32
         */
        static Value f32_convert_i64_u(const Value& value) noexcept;

        /**
         * @brief f64.convert_i32_s: convert signed i32 to f64
         */
        static Value f64_convert_i32_s(const Value& value) noexcept;

        /**
         * @brief f64.convert_i32_u: convert unsigned i32 to f64
         */
        static Value f64_convert_i32_u(const Value& value) noexcept;

        /**
         * @brief f64.convert_i64_s: convert signed i64 to f64
         */
        static Value f64_convert_i64_s(const Value& value) noexcept;

        /**
         * @brief f64.convert_i64_u: convert unsigned i64 to f64
         */
        static Value f64_convert_i64_u(const Value& value) noexcept;

        // =====================================================================
        // Reinterpretation Operations (WebAssembly Section 4.3.2.3)
        // =====================================================================

        /**
         * @brief i32.reinterpret_f32: bitwise reinterpret f32 as i32
         */
        static Value i32_reinterpret_f32(const Value& value) noexcept;

        /**
         * @brief i64.reinterpret_f64: bitwise reinterpret f64 as i64
         */
        static Value i64_reinterpret_f64(const Value& value) noexcept;

        /**
         * @brief f32.reinterpret_i32: bitwise reinterpret i32 as f32
         */
        static Value f32_reinterpret_i32(const Value& value) noexcept;

        /**
         * @brief f64.reinterpret_i64: bitwise reinterpret i64 as f64
         */
        static Value f64_reinterpret_i64(const Value& value) noexcept;
    };

    // =========================================================================
    // Zero-Overhead Template Conversion Dispatch
    // =========================================================================

    namespace optimized {
        /**
         * @brief Compile-time conversion dispatch for zero-overhead
         */
        template<ValueType From, ValueType To>
        struct TypedConversion {
            static constexpr bool is_valid = false;
            static Value convert(const Value& value) noexcept = delete;
        };

        // Specializations for valid conversions
        template<>
        struct TypedConversion<ValueType::I32, ValueType::I64> {
            static constexpr bool is_valid = true;
            static Value convert(const Value& value) noexcept {
                return TypeConverter::i64_extend_i32_s(value);
            }
        };

        template<>
        struct TypedConversion<ValueType::I64, ValueType::I32> {
            static constexpr bool is_valid = true;
            static Value convert(const Value& value) noexcept {
                return TypeConverter::i32_wrap_i64(value);
            }
        };

        template<>
        struct TypedConversion<ValueType::F32, ValueType::F64> {
            static constexpr bool is_valid = true;
            static Value convert(const Value& value) noexcept {
                return TypeConverter::f64_promote_f32(value);
            }
        };

        template<>
        struct TypedConversion<ValueType::F64, ValueType::F32> {
            static constexpr bool is_valid = true;
            static Value convert(const Value& value) noexcept {
                return TypeConverter::f32_demote_f64(value);
            }
        };

        /**
         * @brief Runtime conversion with compile-time optimization
         */
        template<ValueType To>
        constexpr Result<Value> convert_to(const Value& value) noexcept {
            switch (value.type()) {
                case ValueType::I32:
                    if constexpr (TypedConversion<ValueType::I32, To>::is_valid) {
                        return Result<Value>{TypedConversion<ValueType::I32, To>::convert(value)};
                    }
                    break;
                case ValueType::I64:
                    if constexpr (TypedConversion<ValueType::I64, To>::is_valid) {
                        return Result<Value>{TypedConversion<ValueType::I64, To>::convert(value)};
                    }
                    break;
                case ValueType::F32:
                    if constexpr (TypedConversion<ValueType::F32, To>::is_valid) {
                        return Result<Value>{TypedConversion<ValueType::F32, To>::convert(value)};
                    }
                    break;
                case ValueType::F64:
                    if constexpr (TypedConversion<ValueType::F64, To>::is_valid) {
                        return Result<Value>{TypedConversion<ValueType::F64, To>::convert(value)};
                    }
                    break;
                default:
                    break;
            }
            
            return Result<Value>{Error{error_codes::InvalidConversion, 
                "No conversion available from source to target type"}};
        }
    }

    // =========================================================================
    // Performance Optimization Utilities
    // =========================================================================

    namespace performance {
        /**
         * @brief Performance targets for different platforms
         */
        template<platform::Platform P>
        struct ConversionPerformanceTargets;

        template<>
        struct ConversionPerformanceTargets<platform::Platform::Dreamcast> {
            static constexpr auto max_integer_conversion_cycles = 5;
            static constexpr auto max_float_conversion_cycles = 10;
            static constexpr auto max_reinterpret_cycles = 2;
            static constexpr auto max_truncation_cycles = 15;
        };

        template<>
        struct ConversionPerformanceTargets<platform::Platform::PSP> {
            static constexpr auto max_integer_conversion_cycles = 3;
            static constexpr auto max_float_conversion_cycles = 5;
            static constexpr auto max_reinterpret_cycles = 1;
            static constexpr auto max_truncation_cycles = 10;
        };

        template<>
        struct ConversionPerformanceTargets<platform::Platform::PSVita> {
            static constexpr auto max_integer_conversion_cycles = 2;
            static constexpr auto max_float_conversion_cycles = 3;
            static constexpr auto max_reinterpret_cycles = 1;
            static constexpr auto max_truncation_cycles = 5;
        };

        template<>
        struct ConversionPerformanceTargets<platform::Platform::MacOS> {
            static constexpr auto max_integer_conversion_cycles = 1;
            static constexpr auto max_float_conversion_cycles = 2;
            static constexpr auto max_reinterpret_cycles = 1;
            static constexpr auto max_truncation_cycles = 3;
        };
    }

    // =========================================================================
    // Inline Implementation of Core Conversion Operations
    // =========================================================================

    // Integer conversions
    inline Value TypeConverter::i32_wrap_i64(const Value& value) noexcept {
        auto i64_val = value.as_i64().value();
        // Simply truncate to lower 32 bits
        int32_t result = static_cast<int32_t>(i64_val);
        return Value::from_i32(result);
    }

    inline Value TypeConverter::i64_extend_i32_s(const Value& value) noexcept {
        #ifdef FLIGHT_WASM_PLATFORM_DREAMCAST
            return platform_conversions::dreamcast::optimized_i64_extend_i32_s(value);
        #else
            auto i32_val = value.as_i32().value();
            // Sign extension
            int64_t result = static_cast<int64_t>(i32_val);
            return Value::from_i64(result);
        #endif
    }

    inline Value TypeConverter::i64_extend_i32_u(const Value& value) noexcept {
        auto i32_val = value.as_i32().value();
        // Zero extension (cast to unsigned first)
        uint64_t result = static_cast<uint32_t>(i32_val);
        return Value::from_i64(static_cast<int64_t>(result));
    }

    // Floating-point conversions
    inline Value TypeConverter::f32_demote_f64(const Value& value) noexcept {
        #ifdef FLIGHT_WASM_PLATFORM_DREAMCAST
            return platform_conversions::dreamcast::optimized_f32_demote_f64(value);
        #else
            auto f64_val = value.as_f64().value();
            // IEEE 754 compliant demotion with rounding
            float result = static_cast<float>(f64_val);
            return Value::from_f32(result);
        #endif
    }

    inline Value TypeConverter::f64_promote_f32(const Value& value) noexcept {
        #ifdef FLIGHT_WASM_PLATFORM_DREAMCAST
            return platform_conversions::dreamcast::optimized_f64_promote_f32(value);
        #else
            auto f32_val = value.as_f32().value();
            // Exact promotion (no precision loss)
            double result = static_cast<double>(f32_val);
            return Value::from_f64(result);
        #endif
    }

    // Reinterpretation operations
    inline Value TypeConverter::i32_reinterpret_f32(const Value& value) noexcept {
        #ifdef FLIGHT_WASM_PLATFORM_PSP
            return platform_conversions::psp::optimized_i32_reinterpret_f32(value);
        #else
            auto f32_val = value.as_f32().value();
            int32_t result;
            std::memcpy(&result, &f32_val, sizeof(int32_t));
            return Value::from_i32(result);
        #endif
    }

    inline Value TypeConverter::i64_reinterpret_f64(const Value& value) noexcept {
        auto f64_val = value.as_f64().value();
        int64_t result;
        std::memcpy(&result, &f64_val, sizeof(int64_t));
        return Value::from_i64(result);
    }

    inline Value TypeConverter::f32_reinterpret_i32(const Value& value) noexcept {
        #ifdef FLIGHT_WASM_PLATFORM_PSP
            return platform_conversions::psp::optimized_f32_reinterpret_i32(value);
        #else
            auto i32_val = value.as_i32().value();
            float result;
            std::memcpy(&result, &i32_val, sizeof(float));
            return Value::from_f32(result);
        #endif
    }

    inline Value TypeConverter::f64_reinterpret_i64(const Value& value) noexcept {
        auto i64_val = value.as_i64().value();
        double result;
        std::memcpy(&result, &i64_val, sizeof(double));
        return Value::from_f64(result);
    }

} // namespace flight::wasm::conversions

#endif // FLIGHT_WASM_TYPES_CONVERSIONS_HPP
