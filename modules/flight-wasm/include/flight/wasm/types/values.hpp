#ifndef FLIGHT_WASM_TYPES_VALUES_HPP
#define FLIGHT_WASM_TYPES_VALUES_HPP

/**
 * @file values.hpp
 * @brief WebAssembly value types and value representation
 * 
 * This header defines the core WebAssembly value types (i32, i64, f32, f64, v128,
 * funcref, externref) and provides efficient value representation for the Flight
 * WASM toolkit with comprehensive platform optimizations and specification compliance.
 */

#include <cstdint>
#include <array>
#include <type_traits>
#include <limits>
#include <cstring>
#include <flight/wasm/utilities/platform.hpp>
#include <flight/wasm/utilities/error.hpp>

namespace flight::wasm {

    /**
     * @brief WebAssembly value types as defined by the specification
     * 
     * These correspond directly to the WebAssembly Core Specification Section 2.3:
     * Binary encoding values match the specification exactly for binary format compatibility.
     */
    enum class ValueType : uint8_t {
        // Number types (Section 2.3.1)
        I32 = 0x7F,      // 32-bit integer
        I64 = 0x7E,      // 64-bit integer  
        F32 = 0x7D,      // 32-bit IEEE 754 floating point
        F64 = 0x7C,      // 64-bit IEEE 754 floating point
        
        // Vector types (Section 2.3.2)
        V128 = 0x7B,     // 128-bit SIMD vector
        
        // Reference types (Section 2.3.3)
        FuncRef = 0x70,  // Function reference
        ExternRef = 0x6F, // External reference
        
        // Special encoding values
        EmptyBlockType = 0x40  // Empty block type (void)
    };

    /**
     * @brief Validate that a value type is valid according to WebAssembly specification
     */
    constexpr bool is_valid_value_type(ValueType type) noexcept {
        switch (type) {
            case ValueType::I32:
            case ValueType::I64:
            case ValueType::F32:
            case ValueType::F64:
            case ValueType::V128:
            case ValueType::FuncRef:
            case ValueType::ExternRef:
                return true;
            default:
                return false;
        }
    }

    /**
     * @brief Check if a value type is a number type (Section 2.3.1)
     */
    constexpr bool is_number_type(ValueType type) noexcept {
        return type == ValueType::I32 || type == ValueType::I64 || 
               type == ValueType::F32 || type == ValueType::F64;
    }

    /**
     * @brief Check if a value type is a vector type (Section 2.3.2)
     */
    constexpr bool is_vector_type(ValueType type) noexcept {
        return type == ValueType::V128;
    }

    /**
     * @brief Check if a value type is a reference type (Section 2.3.3)
     */
    constexpr bool is_reference_type(ValueType type) noexcept {
        return type == ValueType::FuncRef || type == ValueType::ExternRef;
    }

    /**
     * @brief Get the size in bytes of a value type
     */
    constexpr size_t value_type_size(ValueType type) noexcept {
        switch (type) {
            case ValueType::I32:
            case ValueType::F32:
                return 4;
            case ValueType::I64:
            case ValueType::F64:
                return 8;
            case ValueType::V128:
                return 16;
            case ValueType::FuncRef:
            case ValueType::ExternRef:
                return sizeof(void*);
            default:
                return 0;
        }
    }

    /**
     * @brief Get the alignment requirement for a value type
     */
    constexpr size_t value_type_alignment(ValueType type) noexcept {
        switch (type) {
            case ValueType::I32:
            case ValueType::F32:
                return 4;
            case ValueType::I64:
            case ValueType::F64:
                return 8;
            case ValueType::V128:
                return 16;
            case ValueType::FuncRef:
            case ValueType::ExternRef:
                return alignof(void*);
            default:
                return 1;
        }
    }

    /**
     * @brief Get human-readable name for a value type
     */
    constexpr const char* value_type_name(ValueType type) noexcept {
        switch (type) {
            case ValueType::I32: return "i32";
            case ValueType::I64: return "i64";
            case ValueType::F32: return "f32";
            case ValueType::F64: return "f64";
            case ValueType::V128: return "v128";
            case ValueType::FuncRef: return "funcref";
            case ValueType::ExternRef: return "externref";
            default: return "unknown";
        }
    }

    // Forward declarations for types to be implemented
    class Value;
    class I32Value;
    class I64Value;
    class F32Value;
    class F64Value;
    class V128Value;
    class FuncRefValue;
    class ExternRefValue;

    /**
     * @brief SIMD vector type for V128 with platform optimizations
     * 
     * This provides a portable representation of 128-bit SIMD values
     * that works across different platforms and architectures.
     */
    union alignas(16) V128 {
        // Integer interpretations
        std::array<uint8_t, 16> u8;
        std::array<int8_t, 16> i8;
        std::array<uint16_t, 8> u16;
        std::array<int16_t, 8> i16;
        std::array<uint32_t, 4> u32;
        std::array<int32_t, 4> i32;
        std::array<uint64_t, 2> u64;
        std::array<int64_t, 2> i64;
        
        // Floating-point interpretations
        std::array<float, 4> f32;
        std::array<double, 2> f64;
        
        // Raw bytes
        std::array<uint8_t, 16> bytes;

        constexpr V128() noexcept : bytes{} {}
        constexpr V128(const std::array<uint8_t, 16>& b) noexcept : bytes(b) {}
        
        // Platform-specific optimizations
        #if defined(FLIGHT_WASM_NEON_AVAILABLE) && defined(__ARM_NEON)
            // NEON intrinsics integration for PS Vita (only when actually on ARM with NEON)
            #include <arm_neon.h>
            uint8x16_t neon_u8;
            int8x16_t neon_i8;
            uint16x8_t neon_u16;
            int16x8_t neon_i16;
            uint32x4_t neon_u32;
            int32x4_t neon_i32;
            float32x4_t neon_f32;
        #endif
        
        // Equality operators
        constexpr bool operator==(const V128& other) const noexcept {
            for (size_t i = 0; i < 16; ++i) {
                if (bytes[i] != other.bytes[i]) return false;
            }
            return true;
        }
        
        constexpr bool operator!=(const V128& other) const noexcept {
            return !(*this == other);
        }
    };

    static_assert(sizeof(V128) == 16, "V128 must be exactly 16 bytes");
    static_assert(alignof(V128) == 16, "V128 must be 16-byte aligned");

    // =============================================================================
    // Advanced Type Traits System
    // =============================================================================

    /**
     * @brief Comprehensive type traits for WebAssembly value types
     * 
     * This system provides zero-overhead compile-time type information
     * with platform-specific optimizations for embedded systems.
     */
    template<ValueType Type>
    struct value_type_traits {
        static_assert(is_valid_value_type(Type), "Invalid value type");
        
        static constexpr ValueType type = Type;
        static constexpr size_t size = value_type_size(Type);
        static constexpr size_t alignment = value_type_alignment(Type);
        static constexpr const char* name = value_type_name(Type);
        
        static constexpr bool is_number = is_number_type(Type);
        static constexpr bool is_vector = is_vector_type(Type);
        static constexpr bool is_reference = is_reference_type(Type);
        static constexpr bool is_valid = is_valid_value_type(Type);
    };

    // Platform-optimized storage type specializations
    template<>
    struct value_type_traits<ValueType::I32> {
        using storage_type = int32_t;
        using fast_type = int32_t;
        
        static constexpr ValueType type = ValueType::I32;
        static constexpr size_t size = 4;
        static constexpr size_t alignment = 4;
        static constexpr const char* name = "i32";
        
        static constexpr bool is_number = true;
        static constexpr bool is_vector = false;
        static constexpr bool is_reference = false;
        static constexpr bool is_signed = true;
        static constexpr bool is_valid = true;
        
        // Platform-specific optimization flags
        #if defined(FLIGHT_WASM_PLATFORM_DREAMCAST) || defined(FLIGHT_WASM_PLATFORM_PSP) || defined(FLIGHT_WASM_PLATFORM_VITA)
            static constexpr bool has_fast_ops = true;  // All platforms handle 32-bit efficiently
        #else
            static constexpr bool has_fast_ops = true;
        #endif
        
        static constexpr storage_type default_value() noexcept { return 0; }
        static constexpr bool in_range(int64_t value) noexcept {
            return value >= INT32_MIN && value <= INT32_MAX;
        }
        static constexpr storage_type min_value() noexcept { return INT32_MIN; }
        static constexpr storage_type max_value() noexcept { return INT32_MAX; }
    };

    template<>
    struct value_type_traits<ValueType::I64> {
        using storage_type = int64_t;
        using fast_type = int64_t;
        
        static constexpr ValueType type = ValueType::I64;
        static constexpr size_t size = 8;
        static constexpr size_t alignment = 8;
        static constexpr const char* name = "i64";
        
        static constexpr bool is_number = true;
        static constexpr bool is_vector = false;
        static constexpr bool is_reference = false;
        static constexpr bool is_signed = true;
        static constexpr bool is_valid = true;
        
        // Platform-specific 64-bit optimization
        #if defined(FLIGHT_WASM_PLATFORM_DREAMCAST) || defined(FLIGHT_WASM_PLATFORM_PSP)
            static constexpr bool has_fast_ops = false;  // Limited 64-bit support
            static constexpr bool needs_emulation = true;
        #else
            static constexpr bool has_fast_ops = true;   // Native 64-bit support
            static constexpr bool needs_emulation = false;
        #endif
        
        static constexpr storage_type default_value() noexcept { return 0; }
        static constexpr storage_type min_value() noexcept { return INT64_MIN; }
        static constexpr storage_type max_value() noexcept { return INT64_MAX; }
    };

    template<>
    struct value_type_traits<ValueType::F32> {
        using storage_type = float;
        using fast_type = float;
        
        static constexpr ValueType type = ValueType::F32;
        static constexpr size_t size = 4;
        static constexpr size_t alignment = 4;
        static constexpr const char* name = "f32";
        
        static constexpr bool is_number = true;
        static constexpr bool is_vector = false;
        static constexpr bool is_reference = false;
        static constexpr bool is_signed = true;
        static constexpr bool is_floating_point = true;
        static constexpr bool is_valid = true;
        
        // Verify IEEE 754 compliance
        static_assert(std::numeric_limits<float>::is_iec559, "f32 must be IEEE 754 compliant");
        static_assert(sizeof(float) == 4, "f32 must be 4 bytes");
        
        // Most embedded platforms have FPU for single precision
        static constexpr bool has_fast_ops = true;
        
        static constexpr storage_type default_value() noexcept { return 0.0f; }
        static constexpr int mantissa_bits = std::numeric_limits<float>::digits - 1;  // 23
        static constexpr int exponent_bits = 8;
    };

    template<>
    struct value_type_traits<ValueType::F64> {
        using storage_type = double;
        using fast_type = double;
        
        static constexpr ValueType type = ValueType::F64;
        static constexpr size_t size = 8;
        static constexpr size_t alignment = 8;
        static constexpr const char* name = "f64";
        
        static constexpr bool is_number = true;
        static constexpr bool is_vector = false;
        static constexpr bool is_reference = false;
        static constexpr bool is_signed = true;
        static constexpr bool is_floating_point = true;
        static constexpr bool is_valid = true;
        
        // Verify IEEE 754 compliance
        static_assert(std::numeric_limits<double>::is_iec559, "f64 must be IEEE 754 compliant");
        static_assert(sizeof(double) == 8, "f64 must be 8 bytes");
        
        // Platform-specific double precision support
        #if defined(FLIGHT_WASM_PLATFORM_DREAMCAST) || defined(FLIGHT_WASM_PLATFORM_PSP)
            static constexpr bool has_fast_ops = false;  // Limited double precision
        #else
            static constexpr bool has_fast_ops = true;   // Full double precision
        #endif
        
        static constexpr storage_type default_value() noexcept { return 0.0; }
        static constexpr int mantissa_bits = std::numeric_limits<double>::digits - 1;  // 52
        static constexpr int exponent_bits = 11;
    };

    template<>
    struct value_type_traits<ValueType::V128> {
        using storage_type = V128;
        using fast_type = V128;
        
        static constexpr ValueType type = ValueType::V128;
        static constexpr size_t size = 16;
        static constexpr size_t alignment = 16;
        static constexpr const char* name = "v128";
        
        static constexpr bool is_number = false;
        static constexpr bool is_vector = true;
        static constexpr bool is_reference = false;
        static constexpr bool is_valid = true;
        
        // Platform-specific SIMD support
        #if defined(FLIGHT_WASM_NEON_AVAILABLE) && defined(__ARM_NEON)
            static constexpr bool has_simd_support = true;
            static constexpr bool has_fast_ops = true;
        #else
            static constexpr bool has_simd_support = false;
            static constexpr bool has_fast_ops = false;  // Emulated SIMD
        #endif
        
        static constexpr storage_type default_value() noexcept { 
            return storage_type{}; // Zero-initialized
        }
    };

    template<>
    struct value_type_traits<ValueType::FuncRef> {
        using storage_type = void*;
        using fast_type = void*;
        
        static constexpr ValueType type = ValueType::FuncRef;
        static constexpr size_t size = sizeof(void*);
        static constexpr size_t alignment = alignof(void*);
        static constexpr const char* name = "funcref";
        
        static constexpr bool is_number = false;
        static constexpr bool is_vector = false;
        static constexpr bool is_reference = true;
        static constexpr bool is_nullable = true;
        static constexpr bool is_valid = true;
        static constexpr bool has_fast_ops = true;  // Pointer operations are fast
        
        static constexpr storage_type default_value() noexcept { return nullptr; }
        static constexpr storage_type null_value() noexcept { return nullptr; }
    };

    template<>
    struct value_type_traits<ValueType::ExternRef> {
        using storage_type = void*;
        using fast_type = void*;
        
        static constexpr ValueType type = ValueType::ExternRef;
        static constexpr size_t size = sizeof(void*);
        static constexpr size_t alignment = alignof(void*);
        static constexpr const char* name = "externref";
        
        static constexpr bool is_number = false;
        static constexpr bool is_vector = false;
        static constexpr bool is_reference = true;
        static constexpr bool is_nullable = true;
        static constexpr bool is_valid = true;
        static constexpr bool has_fast_ops = true;  // Pointer operations are fast
        
        static constexpr storage_type default_value() noexcept { return nullptr; }
        static constexpr storage_type null_value() noexcept { return nullptr; }
    };

    // =============================================================================
    // Type Mapping and Conversion Utilities
    // =============================================================================

    /**
     * @brief Map C++ types to WebAssembly value types
     */
    template<typename T>
    struct cpp_to_value_type {
        static constexpr ValueType value = ValueType::I32; // Default fallback
    };

    template<> struct cpp_to_value_type<int32_t> { static constexpr ValueType value = ValueType::I32; };
    template<> struct cpp_to_value_type<uint32_t> { static constexpr ValueType value = ValueType::I32; };
    template<> struct cpp_to_value_type<int64_t> { static constexpr ValueType value = ValueType::I64; };
    template<> struct cpp_to_value_type<uint64_t> { static constexpr ValueType value = ValueType::I64; };
    template<> struct cpp_to_value_type<float> { static constexpr ValueType value = ValueType::F32; };
    template<> struct cpp_to_value_type<double> { static constexpr ValueType value = ValueType::F64; };
    template<> struct cpp_to_value_type<V128> { static constexpr ValueType value = ValueType::V128; };

    template<typename T>
    constexpr ValueType cpp_to_value_type_v = cpp_to_value_type<T>::value;

    /**
     * @brief Check if a C++ type can be converted to a WebAssembly value
     */
    template<typename T>
    struct is_convertible_to_wasm_value {
        static constexpr bool value = 
            std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t> ||
            std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t> ||
            std::is_same_v<T, float> || std::is_same_v<T, double> ||
            std::is_same_v<T, V128>;
    };

    template<typename T>
    constexpr bool is_convertible_to_wasm_value_v = is_convertible_to_wasm_value<T>::value;

    /**
     * @brief WebAssembly value type trait detection
     */
    template<typename T>
    struct is_wasm_value_type : std::false_type {};

    template<> struct is_wasm_value_type<int32_t> : std::true_type {};
    template<> struct is_wasm_value_type<int64_t> : std::true_type {};
    template<> struct is_wasm_value_type<float> : std::true_type {};
    template<> struct is_wasm_value_type<double> : std::true_type {};
    template<> struct is_wasm_value_type<V128> : std::true_type {};

    template<typename T>
    constexpr bool is_wasm_value_type_v = is_wasm_value_type<T>::value;

    // =============================================================================
    // Zero-Overhead Type Operations
    // =============================================================================

    /**
     * @brief Fast type comparison (compile-time when possible)
     */
    template<ValueType A, ValueType B>
    constexpr bool types_equal() noexcept {
        return A == B;
    }

    /**
     * @brief Runtime type comparison (single instruction)
     */
    inline constexpr bool types_equal(ValueType a, ValueType b) noexcept {
        return static_cast<uint8_t>(a) == static_cast<uint8_t>(b);
    }

    /**
     * @brief Type compatibility checking for conversions
     */
    template<ValueType From, ValueType To>
    constexpr bool is_convertible() noexcept {
        if constexpr (From == To) {
            return true;
        } else if constexpr (is_number_type(From) && is_number_type(To)) {
            return true;  // Numbers are inter-convertible with explicit conversion
        } else {
            return false;  // No implicit conversions for reference/vector types
        }
    }

    /**
     * @brief Runtime type compatibility checking
     */
    inline constexpr bool is_convertible(ValueType from, ValueType to) noexcept {
        if (from == to) {
            return true;
        } else if (is_number_type(from) && is_number_type(to)) {
            return true;
        } else {
            return false;
        }
    }

    // =============================================================================
    // Binary Format Integration
    // =============================================================================

    /**
     * @brief Encode value type to binary format (WebAssembly specification compliant)
     */
    inline constexpr uint8_t encode_value_type(ValueType type) noexcept {
        return static_cast<uint8_t>(type);
    }

    /**
     * @brief Decode value type from binary format
     */
    inline Result<ValueType> decode_value_type(uint8_t byte) noexcept {
        auto type = static_cast<ValueType>(byte);
        if (is_valid_value_type(type)) {
            return Result<ValueType>{type};
        }
        return Result<ValueType>{ErrorCode::TypeMismatch, "Invalid value type encoding"};
    }

    // =============================================================================
    // Type Validation Framework
    // =============================================================================

    /**
     * @brief Comprehensive type validation for WebAssembly compliance
     */
    namespace validation {
        
        /**
         * @brief Validate type encoding from binary format
         */
        inline Result<ValueType> validate_type_encoding(uint8_t encoding) noexcept {
            return decode_value_type(encoding);
        }

        /**
         * @brief Validate type compatibility for operations
         */
        template<ValueType Expected, ValueType Actual>
        constexpr bool validate_type_match() noexcept {
            return Expected == Actual;
        }

        /**
         * @brief Runtime type compatibility validation
         */
        inline bool validate_type_match(ValueType expected, ValueType actual) noexcept {
            return expected == actual;
        }

        /**
         * @brief Validate type for specific operations
         */
        inline bool validate_numeric_type(ValueType type) noexcept {
            return is_number_type(type);
        }

        inline bool validate_reference_type(ValueType type) noexcept {
            return is_reference_type(type);
        }

        inline bool validate_vector_type(ValueType type) noexcept {
            return is_vector_type(type);
        }

        /**
         * @brief Type sequence validation (for function signatures)
         */
        inline bool validate_type_sequence(const ValueType* types, size_t count) noexcept {
            for (size_t i = 0; i < count; ++i) {
                if (!is_valid_value_type(types[i])) {
                    return false;
                }
            }
            return true;
        }

        /**
         * @brief Validate parameter/result type lists
         */
        inline bool validate_function_types(const ValueType* params, size_t param_count,
                                           const ValueType* results, size_t result_count) noexcept {
            return validate_type_sequence(params, param_count) &&
                   validate_type_sequence(results, result_count);
        }

    } // namespace validation

    // =============================================================================
    // Performance Optimization Utilities
    // =============================================================================

    /**
     * @brief Platform-specific performance optimizations for type operations
     */
    namespace performance {

        /**
         * @brief Performance targets for different platforms
         */
        template<platform::Platform P>
        struct TypePerformanceTargets;

        template<>
        struct TypePerformanceTargets<platform::Platform::Dreamcast> {
            static constexpr auto max_type_construction_cycles = 2;
            static constexpr auto max_type_comparison_cycles = 1;
            static constexpr auto max_type_conversion_cycles = 10;
            static constexpr size_t max_type_memory_bytes = 8;  // Compact for 16MB system
        };

        template<>
        struct TypePerformanceTargets<platform::Platform::PSP> {
            static constexpr auto max_type_construction_cycles = 1;
            static constexpr auto max_type_comparison_cycles = 1;
            static constexpr auto max_type_conversion_cycles = 5;
            static constexpr size_t max_type_memory_bytes = 8;
        };

        template<>
        struct TypePerformanceTargets<platform::Platform::PSVita> {
            static constexpr auto max_type_construction_cycles = 1;
            static constexpr auto max_type_comparison_cycles = 1;
            static constexpr auto max_type_conversion_cycles = 3;
            static constexpr size_t max_type_memory_bytes = 16;
        };

        template<>
        struct TypePerformanceTargets<platform::Platform::MacOS> {
            static constexpr auto max_type_construction_cycles = 1;
            static constexpr auto max_type_comparison_cycles = 1;
            static constexpr auto max_type_conversion_cycles = 1;
            static constexpr size_t max_type_memory_bytes = 32;
        };

        // Current platform performance targets
        using CurrentPlatformTargets = TypePerformanceTargets<platform::current_platform()>;

    } // namespace performance

    // =============================================================================
    // Compact Type Representation for Embedded Platforms
    // =============================================================================

    /**
     * @brief Memory-efficient type representation for embedded platforms
     */
    class CompactValueType {
    public:
        /**
         * @brief Construct from ValueType
         */
        constexpr explicit CompactValueType(ValueType type) noexcept 
            : type_(static_cast<uint8_t>(type)) {}

        /**
         * @brief Get the underlying ValueType
         */
        constexpr ValueType type() const noexcept {
            return static_cast<ValueType>(type_);
        }

        /**
         * @brief Equality comparison
         */
        constexpr bool operator==(const CompactValueType& other) const noexcept {
            return type_ == other.type_;
        }

        constexpr bool operator!=(const CompactValueType& other) const noexcept {
            return type_ != other.type_;
        }

        /**
         * @brief Fast type category checks
         */
        constexpr bool is_number() const noexcept {
            return is_number_type(type());
        }

        constexpr bool is_vector() const noexcept {
            return is_vector_type(type());
        }

        constexpr bool is_reference() const noexcept {
            return is_reference_type(type());
        }

        /**
         * @brief Binary encoding
         */
        constexpr uint8_t encoding() const noexcept {
            return type_;
        }

    private:
        uint8_t type_;
    };

    static_assert(sizeof(CompactValueType) == 1, "CompactValueType must be single byte");

} // namespace flight::wasm

#endif // FLIGHT_WASM_TYPES_VALUES_HPP
