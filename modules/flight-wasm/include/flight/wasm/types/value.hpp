#ifndef FLIGHT_WASM_TYPES_VALUE_HPP
#define FLIGHT_WASM_TYPES_VALUE_HPP

/**
 * @file value.hpp
 * @brief WebAssembly Value class - Universal container for WebAssembly values
 * 
 * This header implements the core Value class that serves as the universal
 * container for all WebAssembly values (i32, i64, f32, f64, v128, funcref, externref).
 * The implementation is optimized for performance and memory usage across embedded
 * to server platforms with zero-overhead abstractions.
 */

#include <flight/wasm/types/values.hpp>
#include <flight/wasm/utilities/error.hpp>
#include <flight/wasm/utilities/endian.hpp>
#include <string>
#include <cstring>
#include <vector>
#include <sstream>
#include <iomanip>

namespace flight::wasm {

    /**
     * @brief Universal WebAssembly value container
     * 
     * This class provides a type-safe, memory-efficient container for all
     * WebAssembly value types. It uses a discriminated union design for
     * optimal memory usage and performance on embedded platforms.
     */
    class Value {
    public:
        // =====================================================================
        // Constructors and Assignment
        // =====================================================================

        /**
         * @brief Default constructor creates i32(0)
         */
        constexpr Value() noexcept : type_(ValueType::I32), data_{} {
            data_.i32_val = 0;
        }

        /**
         * @brief Copy constructor
         */
        constexpr Value(const Value& other) noexcept : type_(other.type_) {
            copy_data(other);
        }

        /**
         * @brief Move constructor
         */
        constexpr Value(Value&& other) noexcept : type_(other.type_) {
            copy_data(other);
            // Reset other to default state
            other.type_ = ValueType::I32;
            other.data_.i32_val = 0;
        }

        /**
         * @brief Copy assignment
         */
        Value& operator=(const Value& other) noexcept {
            if (this != &other) {
                type_ = other.type_;
                copy_data(other);
            }
            return *this;
        }

        /**
         * @brief Move assignment
         */
        Value& operator=(Value&& other) noexcept {
            if (this != &other) {
                type_ = other.type_;
                copy_data(other);
                // Reset other to default state
                other.type_ = ValueType::I32;
                other.data_.i32_val = 0;
            }
            return *this;
        }

        // =====================================================================
        // Static Factory Methods
        // =====================================================================

        /**
         * @brief Create i32 value
         */
        static constexpr Value from_i32(int32_t value) noexcept {
            Value v;
            v.type_ = ValueType::I32;
            v.data_.i32_val = value;
            return v;
        }

        /**
         * @brief Create i64 value
         */
        static constexpr Value from_i64(int64_t value) noexcept {
            Value v;
            v.type_ = ValueType::I64;
            v.data_.i64_val = value;
            return v;
        }

        /**
         * @brief Create f32 value
         */
        static constexpr Value from_f32(float value) noexcept {
            Value v;
            v.type_ = ValueType::F32;
            v.data_.f32_val = value;
            return v;
        }

        /**
         * @brief Create f64 value
         */
        static constexpr Value from_f64(double value) noexcept {
            Value v;
            v.type_ = ValueType::F64;
            v.data_.f64_val = value;
            return v;
        }

        /**
         * @brief Create v128 value
         */
        static constexpr Value from_v128(const V128& value) noexcept {
            Value v;
            v.type_ = ValueType::V128;
            v.data_.v128_val = value;
            return v;
        }

        /**
         * @brief Create funcref value
         */
        static constexpr Value from_funcref(void* func) noexcept {
            Value v;
            v.type_ = ValueType::FuncRef;
            v.data_.ref_val = func;
            return v;
        }

        /**
         * @brief Create externref value
         */
        static constexpr Value from_externref(void* ref) noexcept {
            Value v;
            v.type_ = ValueType::ExternRef;
            v.data_.ref_val = ref;
            return v;
        }

        // =====================================================================
        // Type-Safe Value Extraction
        // =====================================================================

        /**
         * @brief Extract i32 value
         */
        Result<int32_t> as_i32() const noexcept {
            if (type_ != ValueType::I32) {
                return Result<int32_t>{ErrorCode::TypeMismatch, "Expected i32 value"};
            }
            return Result<int32_t>{data_.i32_val};
        }

        /**
         * @brief Extract i64 value
         */
        Result<int64_t> as_i64() const noexcept {
            if (type_ != ValueType::I64) {
                return Result<int64_t>{ErrorCode::TypeMismatch, "Expected i64 value"};
            }
            return Result<int64_t>{data_.i64_val};
        }

        /**
         * @brief Extract f32 value
         */
        Result<float> as_f32() const noexcept {
            if (type_ != ValueType::F32) {
                return Result<float>{ErrorCode::TypeMismatch, "Expected f32 value"};
            }
            return Result<float>{data_.f32_val};
        }

        /**
         * @brief Extract f64 value
         */
        Result<double> as_f64() const noexcept {
            if (type_ != ValueType::F64) {
                return Result<double>{ErrorCode::TypeMismatch, "Expected f64 value"};
            }
            return Result<double>{data_.f64_val};
        }

        /**
         * @brief Extract v128 value
         */
        Result<V128> as_v128() const noexcept {
            if (type_ != ValueType::V128) {
                return Result<V128>{ErrorCode::TypeMismatch, "Expected v128 value"};
            }
            return Result<V128>{data_.v128_val};
        }

        /**
         * @brief Extract funcref value
         */
        Result<void*> as_funcref() const noexcept {
            if (type_ != ValueType::FuncRef) {
                return Result<void*>{ErrorCode::TypeMismatch, "Expected funcref value"};
            }
            return Result<void*>{data_.ref_val};
        }

        /**
         * @brief Extract externref value
         */
        Result<void*> as_externref() const noexcept {
            if (type_ != ValueType::ExternRef) {
                return Result<void*>{ErrorCode::TypeMismatch, "Expected externref value"};
            }
            return Result<void*>{data_.ref_val};
        }

        // =====================================================================
        // Type Introspection
        // =====================================================================

        /**
         * @brief Get the value type
         */
        constexpr ValueType type() const noexcept {
            return type_;
        }

        /**
         * @brief Check if value is a number type
         */
        constexpr bool is_number() const noexcept {
            return is_number_type(type_);
        }

        /**
         * @brief Check if value is a vector type
         */
        constexpr bool is_vector() const noexcept {
            return is_vector_type(type_);
        }

        /**
         * @brief Check if value is a reference type
         */
        constexpr bool is_reference() const noexcept {
            return is_reference_type(type_);
        }

        // =====================================================================
        // Default Value Creation
        // =====================================================================

        /**
         * @brief Create default value for a given type
         */
        static constexpr Value default_for_type(ValueType type) noexcept {
            switch (type) {
                case ValueType::I32:
                    return from_i32(0);
                case ValueType::I64:
                    return from_i64(0);
                case ValueType::F32:
                    return from_f32(0.0f);
                case ValueType::F64:
                    return from_f64(0.0);
                case ValueType::V128:
                    return from_v128(V128{});
                case ValueType::FuncRef:
                    return from_funcref(nullptr);
                case ValueType::ExternRef:
                    return from_externref(nullptr);
                default:
                    return Value{}; // Default to i32(0)
            }
        }

        // =====================================================================
        // Comparison Operations
        // =====================================================================

        /**
         * @brief Equality comparison
         */
        bool operator==(const Value& other) const noexcept {
            if (type_ != other.type_) {
                return false;
            }

            switch (type_) {
                case ValueType::I32:
                    return data_.i32_val == other.data_.i32_val;
                case ValueType::I64:
                    return data_.i64_val == other.data_.i64_val;
                case ValueType::F32:
                    return data_.f32_val == other.data_.f32_val;
                case ValueType::F64:
                    return data_.f64_val == other.data_.f64_val;
                case ValueType::V128:
                    return data_.v128_val == other.data_.v128_val;
                case ValueType::FuncRef:
                case ValueType::ExternRef:
                    return data_.ref_val == other.data_.ref_val;
                default:
                    return false;
            }
        }

        /**
         * @brief Inequality comparison
         */
        bool operator!=(const Value& other) const noexcept {
            return !(*this == other);
        }

        // =====================================================================
        // String Representation
        // =====================================================================

        /**
         * @brief Convert value to string for debugging
         */
        std::string to_string() const {
            switch (type_) {
                case ValueType::I32:
                    return std::to_string(data_.i32_val);
                case ValueType::I64:
                    return std::to_string(data_.i64_val);
                case ValueType::F32: {
                    std::ostringstream oss;
                    oss << std::scientific << std::setprecision(6) << data_.f32_val;
                    return oss.str();
                }
                case ValueType::F64: {
                    std::ostringstream oss;
                    oss << std::scientific << std::setprecision(15) << data_.f64_val;
                    return oss.str();
                }
                case ValueType::V128: {
                    std::ostringstream oss;
                    oss << "v128(";
                    for (size_t i = 0; i < 16; ++i) {
                        if (i > 0) oss << " ";
                        oss << std::hex << std::setw(2) << std::setfill('0') 
                            << static_cast<unsigned>(data_.v128_val.bytes[i]);
                    }
                    oss << ")";
                    return oss.str();
                }
                case ValueType::FuncRef: {
                    std::ostringstream oss;
                    oss << "funcref(0x" << std::hex << reinterpret_cast<uintptr_t>(data_.ref_val) << ")";
                    return oss.str();
                }
                case ValueType::ExternRef: {
                    std::ostringstream oss;
                    oss << "externref(0x" << std::hex << reinterpret_cast<uintptr_t>(data_.ref_val) << ")";
                    return oss.str();
                }
                default:
                    return "unknown";
            }
        }

    private:
        // =====================================================================
        // Internal Storage
        // =====================================================================

        /**
         * @brief Discriminated union for efficient storage
         */
        union Data {
            int32_t i32_val;
            int64_t i64_val;
            float f32_val;
            double f64_val;
            V128 v128_val;
            void* ref_val;

            constexpr Data() : i64_val(0) {} // Zero-initialize largest member
        };

        ValueType type_;
        Data data_;

        /**
         * @brief Copy data from another value
         */
        constexpr void copy_data(const Value& other) noexcept {
            switch (other.type_) {
                case ValueType::I32:
                    data_.i32_val = other.data_.i32_val;
                    break;
                case ValueType::I64:
                    data_.i64_val = other.data_.i64_val;
                    break;
                case ValueType::F32:
                    data_.f32_val = other.data_.f32_val;
                    break;
                case ValueType::F64:
                    data_.f64_val = other.data_.f64_val;
                    break;
                case ValueType::V128:
                    data_.v128_val = other.data_.v128_val;
                    break;
                case ValueType::FuncRef:
                case ValueType::ExternRef:
                    data_.ref_val = other.data_.ref_val;
                    break;
                case ValueType::EmptyBlockType:
                    // Should not happen in normal value operations
                    break;
            }
        }

        // Size optimization validation
        static_assert(sizeof(Data) <= 16, "Value data must be compact for embedded platforms");
    };

    // Size optimization validation (after class definition)
    static_assert(sizeof(Value) <= 32, "Value must be compact for embedded platforms");

    // =========================================================================
    // Value Arithmetic Operations
    // =========================================================================

    /**
     * @brief WebAssembly-compliant arithmetic operations
     */
    namespace arithmetic {

        /**
         * @brief i32 addition with overflow wrapping
         */
        inline Result<Value> add_i32(const Value& a, const Value& b) noexcept {
            auto a_val = a.as_i32();
            auto b_val = b.as_i32();

            if (!a_val.success() || !b_val.success()) {
                return Result<Value>{ErrorCode::TypeMismatch, "Invalid operands for i32.add"};
            }

            // WebAssembly integer arithmetic wraps on overflow
            int32_t result = static_cast<int32_t>(
                static_cast<uint32_t>(a_val.value()) + static_cast<uint32_t>(b_val.value())
            );

            return Result<Value>{Value::from_i32(result)};
        }

        /**
         * @brief i64 addition with overflow wrapping
         */
        inline Result<Value> add_i64(const Value& a, const Value& b) noexcept {
            auto a_val = a.as_i64();
            auto b_val = b.as_i64();

            if (!a_val.success() || !b_val.success()) {
                return Result<Value>{ErrorCode::TypeMismatch, "Invalid operands for i64.add"};
            }

            // WebAssembly integer arithmetic wraps on overflow
            int64_t result = static_cast<int64_t>(
                static_cast<uint64_t>(a_val.value()) + static_cast<uint64_t>(b_val.value())
            );

            return Result<Value>{Value::from_i64(result)};
        }

        /**
         * @brief f32 addition
         */
        inline Result<Value> add_f32(const Value& a, const Value& b) noexcept {
            auto a_val = a.as_f32();
            auto b_val = b.as_f32();

            if (!a_val.success() || !b_val.success()) {
                return Result<Value>{ErrorCode::TypeMismatch, "Invalid operands for f32.add"};
            }

            float result = a_val.value() + b_val.value();
            return Result<Value>{Value::from_f32(result)};
        }

        /**
         * @brief f64 addition
         */
        inline Result<Value> add_f64(const Value& a, const Value& b) noexcept {
            auto a_val = a.as_f64();
            auto b_val = b.as_f64();

            if (!a_val.success() || !b_val.success()) {
                return Result<Value>{ErrorCode::TypeMismatch, "Invalid operands for f64.add"};
            }

            double result = a_val.value() + b_val.value();
            return Result<Value>{Value::from_f64(result)};
        }

    } // namespace arithmetic

    // =========================================================================
    // Binary Format Serialization
    // =========================================================================

    /**
     * @brief Binary format serialization for WebAssembly compliance
     */
    namespace binary {

        /**
         * @brief Serialize value to binary format
         */
        inline void serialize_value(const Value& value, std::vector<uint8_t>& output) noexcept {
            switch (value.type()) {
                case ValueType::I32: {
                    auto val = value.as_i32().value();
                    uint32_t bits = endian::host_to_wasm_u32(static_cast<uint32_t>(val));
                    const auto* bytes = reinterpret_cast<const uint8_t*>(&bits);
                    output.insert(output.end(), bytes, bytes + 4);
                    break;
                }
                case ValueType::I64: {
                    auto val = value.as_i64().value();
                    uint64_t bits = endian::host_to_wasm_u64(static_cast<uint64_t>(val));
                    const auto* bytes = reinterpret_cast<const uint8_t*>(&bits);
                    output.insert(output.end(), bytes, bytes + 8);
                    break;
                }
                case ValueType::F32: {
                    auto val = value.as_f32().value();
                    uint32_t bits;
                    std::memcpy(&bits, &val, 4);
                    bits = endian::host_to_wasm_u32(bits);
                    const auto* bytes = reinterpret_cast<const uint8_t*>(&bits);
                    output.insert(output.end(), bytes, bytes + 4);
                    break;
                }
                case ValueType::F64: {
                    auto val = value.as_f64().value();
                    uint64_t bits;
                    std::memcpy(&bits, &val, 8);
                    bits = endian::host_to_wasm_u64(bits);
                    const auto* bytes = reinterpret_cast<const uint8_t*>(&bits);
                    output.insert(output.end(), bytes, bytes + 8);
                    break;
                }
                case ValueType::V128: {
                    auto val = value.as_v128().value();
                    output.insert(output.end(), val.bytes.begin(), val.bytes.end());
                    break;
                }
                case ValueType::FuncRef:
                case ValueType::ExternRef: {
                    // References are not directly serializable in binary format
                    // This would be used for debugging/display only
                    auto val = value.as_funcref().value();
                    uintptr_t ptr_val = reinterpret_cast<uintptr_t>(val);
                    uint64_t bits = endian::host_to_wasm_u64(static_cast<uint64_t>(ptr_val));
                    const auto* bytes = reinterpret_cast<const uint8_t*>(&bits);
                    output.insert(output.end(), bytes, bytes + 8);
                    break;
                }
                case ValueType::EmptyBlockType:
                    // Should not happen in normal value serialization
                    break;
            }
        }

        /**
         * @brief Deserialize value from binary format
         */
        inline Result<Value> deserialize_value(ValueType expected_type, 
                                             const uint8_t* data, 
                                             size_t& offset, 
                                             size_t size) noexcept {
            switch (expected_type) {
                case ValueType::I32: {
                    if (offset + 4 > size) {
                        return Result<Value>{ErrorCode::UnexpectedEndOfFile, "Not enough data for i32 value"};
                    }

                    uint32_t bits;
                    std::memcpy(&bits, data + offset, 4);
                    bits = endian::wasm_to_host_u32(bits);
                    offset += 4;

                    return Result<Value>{Value::from_i32(static_cast<int32_t>(bits))};
                }
                case ValueType::I64: {
                    if (offset + 8 > size) {
                        return Result<Value>{ErrorCode::UnexpectedEndOfFile, "Not enough data for i64 value"};
                    }

                    uint64_t bits;
                    std::memcpy(&bits, data + offset, 8);
                    bits = endian::wasm_to_host_u64(bits);
                    offset += 8;

                    return Result<Value>{Value::from_i64(static_cast<int64_t>(bits))};
                }
                case ValueType::F32: {
                    if (offset + 4 > size) {
                        return Result<Value>{ErrorCode::UnexpectedEndOfFile, "Not enough data for f32 value"};
                    }

                    uint32_t bits;
                    std::memcpy(&bits, data + offset, 4);
                    bits = endian::wasm_to_host_u32(bits);
                    offset += 4;

                    float val;
                    std::memcpy(&val, &bits, 4);
                    return Result<Value>{Value::from_f32(val)};
                }
                case ValueType::F64: {
                    if (offset + 8 > size) {
                        return Result<Value>{ErrorCode::UnexpectedEndOfFile, "Not enough data for f64 value"};
                    }

                    uint64_t bits;
                    std::memcpy(&bits, data + offset, 8);
                    bits = endian::wasm_to_host_u64(bits);
                    offset += 8;

                    double val;
                    std::memcpy(&val, &bits, 8);
                    return Result<Value>{Value::from_f64(val)};
                }
                case ValueType::V128: {
                    if (offset + 16 > size) {
                        return Result<Value>{ErrorCode::UnexpectedEndOfFile, "Not enough data for v128 value"};
                    }

                    V128 val;
                    std::memcpy(val.bytes.data(), data + offset, 16);
                    offset += 16;

                    return Result<Value>{Value::from_v128(val)};
                }
                default:
                    return Result<Value>{ErrorCode::TypeMismatch, "Cannot deserialize reference types"};
            }
        }

    } // namespace binary

    // =========================================================================
    // Zero-Overhead Template Operations  
    // =========================================================================

    /**
     * @brief Template-based zero-overhead value operations
     */
    namespace optimized {

        /**
         * @brief Compile-time value type dispatch
         */
        template<ValueType Type>
        struct typed_value_operations {
            using storage_type = typename value_type_traits<Type>::storage_type;

            static constexpr Value construct(storage_type value) noexcept {
                if constexpr (Type == ValueType::I32) {
                    return Value::from_i32(value);
                } else if constexpr (Type == ValueType::I64) {
                    return Value::from_i64(value);
                } else if constexpr (Type == ValueType::F32) {
                    return Value::from_f32(value);
                } else if constexpr (Type == ValueType::F64) {
                    return Value::from_f64(value);
                } else if constexpr (Type == ValueType::V128) {
                    return Value::from_v128(value);
                } else if constexpr (Type == ValueType::FuncRef) {
                    return Value::from_funcref(value);
                } else if constexpr (Type == ValueType::ExternRef) {
                    return Value::from_externref(value);
                }
            }

            static constexpr Result<storage_type> extract(const Value& value) noexcept {
                if constexpr (Type == ValueType::I32) {
                    return value.as_i32();
                } else if constexpr (Type == ValueType::I64) {
                    return value.as_i64();
                } else if constexpr (Type == ValueType::F32) {
                    return value.as_f32();
                } else if constexpr (Type == ValueType::F64) {
                    return value.as_f64();
                } else if constexpr (Type == ValueType::V128) {
                    return value.as_v128();
                } else if constexpr (Type == ValueType::FuncRef) {
                    return value.as_funcref();
                } else if constexpr (Type == ValueType::ExternRef) {
                    return value.as_externref();
                }
            }
        };

        /**
         * @brief Fast value visitor pattern
         */
        template<typename Func>
        constexpr auto visit_value(const Value& value, Func&& func) {
            switch (value.type()) {
                case ValueType::I32:
                    return func(value.as_i32().value());
                case ValueType::I64:
                    return func(value.as_i64().value());
                case ValueType::F32:
                    return func(value.as_f32().value());
                case ValueType::F64:
                    return func(value.as_f64().value());
                case ValueType::V128:
                    return func(value.as_v128().value());
                case ValueType::FuncRef:
                    return func(value.as_funcref().value());
                case ValueType::ExternRef:
                    return func(value.as_externref().value());
                default:
                    // Should never reach here with valid values
                    return func(int32_t{0});
            }
        }

    } // namespace optimized

} // namespace flight::wasm

#endif // FLIGHT_WASM_TYPES_VALUE_HPP
