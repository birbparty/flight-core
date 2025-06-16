#ifndef FLIGHT_WASM_UTILITIES_ERROR_HPP
#define FLIGHT_WASM_UTILITIES_ERROR_HPP

/**
 * @file error.hpp
 * @brief Comprehensive error handling framework for Flight WASM
 * 
 * This header provides a zero-overhead error handling system using modern C++
 * techniques. It includes categorized error codes, error reporting, and a 
 * Result<T> type for operations that can fail.
 */

#include <string_view>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace flight::wasm {

    /**
     * @brief Categorized error codes for different subsystems
     * 
     * Error codes are organized by category with specific ranges:
     * - Success: 0
     * - Binary format: 0x1000-0x1FFF
     * - Validation: 0x2000-0x2FFF
     * - Memory: 0x3000-0x3FFF
     * - Instruction: 0x4000-0x4FFF
     * - Module: 0x5000-0x5FFF
     */
    enum class ErrorCode : uint32_t {
        // Success
        Success = 0,
        
        // Binary format errors (0x1000-0x1FFF)
        InvalidMagicNumber = 0x1000,
        InvalidVersion = 0x1001,
        InvalidSectionId = 0x1002,
        UnexpectedEndOfFile = 0x1003,
        InvalidLEB128Encoding = 0x1004,
        InvalidUTF8Sequence = 0x1005,
        SectionTooLarge = 0x1006,
        DuplicateSection = 0x1007,
        MissingSectionSize = 0x1008,
        InvalidSectionOrder = 0x1009,
        
        // Validation errors (0x2000-0x2FFF)
        TypeMismatch = 0x2000,
        StackUnderflow = 0x2001,
        StackOverflow = 0x2002,
        UnknownOpcode = 0x2003,
        InvalidTypeIndex = 0x2004,
        InvalidFunctionIndex = 0x2005,
        InvalidTableIndex = 0x2006,
        InvalidMemoryIndex = 0x2007,
        InvalidGlobalIndex = 0x2008,
        InvalidLocalIndex = 0x2009,
        InvalidBranchTarget = 0x200A,
        UnreachableCode = 0x200B,
        InvalidConstantExpression = 0x200C,
        
        // Memory errors (0x3000-0x3FFF)
        OutOfMemory = 0x3000,
        InvalidAlignment = 0x3001,
        OutOfBounds = 0x3002,
        MemoryAccessViolation = 0x3003,
        InvalidMemorySize = 0x3004,
        MemoryLimitExceeded = 0x3005,
        
        // Instruction errors (0x4000-0x4FFF)
        InvalidInstruction = 0x4000,
        InvalidImmediate = 0x4001,
        UnsupportedInstruction = 0x4002,
        InstructionSequenceError = 0x4003,
        
        // Module errors (0x5000-0x5FFF)
        InvalidModule = 0x5000,
        MissingRequiredSection = 0x5001,
        CircularDependency = 0x5002,
        ExportNotFound = 0x5003,
        ImportResolutionFailed = 0x5004,
        ModuleInstantiationFailed = 0x5005
    };

    /**
     * @brief Get the category of an error code
     */
    constexpr uint32_t error_category(ErrorCode code) noexcept {
        return static_cast<uint32_t>(code) & 0xF000;
    }

    /**
     * @brief Check if an error code represents success
     */
    constexpr bool is_success(ErrorCode code) noexcept {
        return code == ErrorCode::Success;
    }

    /**
     * @brief Check if an error code represents a binary format error
     */
    constexpr bool is_binary_error(ErrorCode code) noexcept {
        return error_category(code) == 0x1000;
    }

    /**
     * @brief Check if an error code represents a validation error
     */
    constexpr bool is_validation_error(ErrorCode code) noexcept {
        return error_category(code) == 0x2000;
    }

    /**
     * @brief Check if an error code represents a memory error
     */
    constexpr bool is_memory_error(ErrorCode code) noexcept {
        return error_category(code) == 0x3000;
    }

    /**
     * @brief Lightweight error representation
     * 
     * This class provides zero-overhead error representation when used
     * with compile-time known error codes and messages.
     */
    class Error {
    public:
        /**
         * @brief Construct a success error (no error)
         */
        constexpr Error() noexcept : code_(ErrorCode::Success), message_("") {}

        /**
         * @brief Construct an error with code and optional message
         */
        constexpr Error(ErrorCode code, std::string_view message = "") noexcept
            : code_(code), message_(message) {}

        /**
         * @brief Get the error code
         */
        constexpr ErrorCode code() const noexcept { return code_; }

        /**
         * @brief Get the error message
         */
        constexpr std::string_view message() const noexcept { return message_; }

        /**
         * @brief Check if this represents success (no error)
         */
        constexpr bool success() const noexcept { return code_ == ErrorCode::Success; }

        /**
         * @brief Check if this represents a failure
         */
        constexpr bool failed() const noexcept { return code_ != ErrorCode::Success; }

        /**
         * @brief Implicit conversion to bool (true = success, false = error)
         */
        constexpr explicit operator bool() const noexcept { return success(); }

        /**
         * @brief Get error category
         */
        constexpr uint32_t category() const noexcept { return error_category(code_); }

    private:
        ErrorCode code_;
        std::string_view message_;
    };

    /**
     * @brief Result type for operations that can fail
     * 
     * This is a lightweight alternative to std::expected (C++23) that provides
     * zero-overhead error handling for the Flight WASM library.
     */
    template<typename T>
    class Result {
    public:
        /**
         * @brief Construct a successful result with a value
         */
        template<typename U = T, typename = std::enable_if_t<std::is_constructible_v<T, U&&>>>
        constexpr Result(U&& value) noexcept(std::is_nothrow_constructible_v<T, U&&>)
            : has_value_(true), error_(ErrorCode::Success) {
            new (&storage_.value) T(std::forward<U>(value));
        }

        /**
         * @brief Construct a failed result with an error
         */
        constexpr Result(Error error) noexcept
            : has_value_(false), error_(error) {}

        /**
         * @brief Construct a failed result with an error code
         */
        constexpr Result(ErrorCode code, std::string_view message = "") noexcept
            : has_value_(false), error_(code, message) {}

        /**
         * @brief Copy constructor
         */
        Result(const Result& other) noexcept(std::is_nothrow_copy_constructible_v<T>)
            : has_value_(other.has_value_), error_(other.error_) {
            if (has_value_) {
                new (&storage_.value) T(other.storage_.value);
            }
        }

        /**
         * @brief Move constructor
         */
        Result(Result&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
            : has_value_(other.has_value_), error_(other.error_) {
            if (has_value_) {
                new (&storage_.value) T(std::move(other.storage_.value));
            }
        }

        /**
         * @brief Destructor
         */
        ~Result() {
            if (has_value_) {
                storage_.value.~T();
            }
        }

        /**
         * @brief Copy assignment
         */
        Result& operator=(const Result& other) noexcept(std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_copy_assignable_v<T>) {
            if (this != &other) {
                if (has_value_ && other.has_value_) {
                    storage_.value = other.storage_.value;
                } else if (has_value_ && !other.has_value_) {
                    storage_.value.~T();
                    has_value_ = false;
                    error_ = other.error_;
                } else if (!has_value_ && other.has_value_) {
                    new (&storage_.value) T(other.storage_.value);
                    has_value_ = true;
                } else {
                    error_ = other.error_;
                }
            }
            return *this;
        }

        /**
         * @brief Move assignment
         */
        Result& operator=(Result&& other) noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>) {
            if (this != &other) {
                if (has_value_ && other.has_value_) {
                    storage_.value = std::move(other.storage_.value);
                } else if (has_value_ && !other.has_value_) {
                    storage_.value.~T();
                    has_value_ = false;
                    error_ = other.error_;
                } else if (!has_value_ && other.has_value_) {
                    new (&storage_.value) T(std::move(other.storage_.value));
                    has_value_ = true;
                } else {
                    error_ = other.error_;
                }
            }
            return *this;
        }

        /**
         * @brief Check if the result contains a value (success)
         */
        constexpr bool success() const noexcept { return has_value_; }

        /**
         * @brief Check if the result contains an error (failure)
         */
        constexpr bool failed() const noexcept { return !has_value_; }

        /**
         * @brief Implicit conversion to bool (true = success, false = error)
         */
        constexpr explicit operator bool() const noexcept { return success(); }

        /**
         * @brief Get the contained value (const reference)
         * @pre success() must be true
         */
        constexpr const T& value() const& noexcept {
            return storage_.value;
        }

        /**
         * @brief Get the contained value (mutable reference)
         * @pre success() must be true
         */
        constexpr T& value() & noexcept {
            return storage_.value;
        }

        /**
         * @brief Get the contained value (rvalue reference)
         * @pre success() must be true
         */
        constexpr T&& value() && noexcept {
            return std::move(storage_.value);
        }

        /**
         * @brief Get the error
         * @pre failed() must be true
         */
        constexpr const Error& error() const noexcept {
            return error_;
        }

        /**
         * @brief Get the value or a default if error
         */
        template<typename U>
        constexpr T value_or(U&& default_value) const& {
            return success() ? storage_.value : static_cast<T>(std::forward<U>(default_value));
        }

        /**
         * @brief Get the value or a default if error (rvalue version)
         */
        template<typename U>
        constexpr T value_or(U&& default_value) && {
            return success() ? std::move(storage_.value) : static_cast<T>(std::forward<U>(default_value));
        }

        /**
         * @brief Dereference operator (const)
         * @pre success() must be true
         */
        constexpr const T& operator*() const& noexcept {
            return storage_.value;
        }

        /**
         * @brief Dereference operator (mutable)
         * @pre success() must be true
         */
        constexpr T& operator*() & noexcept {
            return storage_.value;
        }

        /**
         * @brief Dereference operator (rvalue)
         * @pre success() must be true
         */
        constexpr T&& operator*() && noexcept {
            return std::move(storage_.value);
        }

        /**
         * @brief Arrow operator (const)
         * @pre success() must be true
         */
        constexpr const T* operator->() const noexcept {
            return &storage_.value;
        }

        /**
         * @brief Arrow operator (mutable)
         * @pre success() must be true
         */
        constexpr T* operator->() noexcept {
            return &storage_.value;
        }

    private:
        bool has_value_;
        Error error_;
        
        union Storage {
            Storage() {}
            ~Storage() {}
            T value;
            char dummy; // For empty storage
        } storage_;
    };

    /**
     * @brief Convenience function to create a successful Result
     */
    template<typename T>
    constexpr Result<std::decay_t<T>> make_result(T&& value) {
        return Result<std::decay_t<T>>(std::forward<T>(value));
    }

    /**
     * @brief Convenience function to create a failed Result
     */
    template<typename T>
    constexpr Result<T> make_error(ErrorCode code, std::string_view message = "") {
        return Result<T>(code, message);
    }

    /**
     * @brief Convenience function to create a failed Result from Error
     */
    template<typename T>
    constexpr Result<T> make_error(Error error) {
        return Result<T>(error);
    }

} // namespace flight::wasm

#endif // FLIGHT_WASM_UTILITIES_ERROR_HPP
