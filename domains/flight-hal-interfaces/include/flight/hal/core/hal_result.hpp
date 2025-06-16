/**
 * @file hal_result.hpp
 * @brief Flight HAL Result Type for Zero-Allocation Error Handling
 * 
 * Provides a Rust-inspired Result<T,E> type with union-based storage,
 * move semantics, and zero-allocation guarantees for embedded systems.
 */

#pragma once

#include "hal_error.hpp"
#include <type_traits>
#include <utility>

namespace flight::hal {

/**
 * @brief Result type for HAL operations with zero-allocation error handling
 * 
 * A union-based Result<T,E> type that either contains a successful value of type T
 * or an error of type E. Designed for embedded systems with no dynamic allocation,
 * efficient move semantics, and minimal memory overhead.
 * 
 * @tparam T The type of the successful result value
 * @tparam E The type of the error (defaults to HALError)
 */
template<typename T, typename E = HALError>
class HALResult {
private:
    // Union storage for value or error - no dynamic allocation
    union Storage {
        T value;
        E error;
        
        // Trivial constructors for union
        Storage() noexcept {}
        ~Storage() noexcept {}
    };
    
    Storage storage_;
    bool has_value_;
    
    // Helper for destroying the active union member
    void destroy() noexcept {
        if (has_value_) {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                storage_.value.~T();
            }
        } else {
            if constexpr (!std::is_trivially_destructible_v<E>) {
                storage_.error.~E();
            }
        }
    }

public:
    // Type aliases for convenience
    using value_type = T;
    using error_type = E;
    
    /**
     * @brief Construct successful result with value
     * @param value The success value (moved)
     */
    HALResult(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>)
        : has_value_(true) {
        new(&storage_.value) T(std::move(value));
    }
    
    /**
     * @brief Construct successful result with value (copy)
     * @param value The success value (copied)
     */
    HALResult(const T& value) noexcept(std::is_nothrow_copy_constructible_v<T>)
        : has_value_(true) {
        new(&storage_.value) T(value);
    }
    
    /**
     * @brief Construct error result
     * @param error The error value (moved)
     */
    HALResult(E&& error) noexcept(std::is_nothrow_move_constructible_v<E>)
        : has_value_(false) {
        new(&storage_.error) E(std::move(error));
    }
    
    /**
     * @brief Construct error result (copy)
     * @param error The error value (copied)
     */
    HALResult(const E& error) noexcept(std::is_nothrow_copy_constructible_v<E>)
        : has_value_(false) {
        new(&storage_.error) E(error);
    }
    
    /**
     * @brief Move constructor
     * @param other Result to move from
     */
    HALResult(HALResult&& other) noexcept
        : has_value_(other.has_value_) {
        if (has_value_) {
            new(&storage_.value) T(std::move(other.storage_.value));
        } else {
            new(&storage_.error) E(std::move(other.storage_.error));
        }
    }
    
    /**
     * @brief Copy constructor
     * @param other Result to copy from
     */
    HALResult(const HALResult& other)
        : has_value_(other.has_value_) {
        if (has_value_) {
            new(&storage_.value) T(other.storage_.value);
        } else {
            new(&storage_.error) E(other.storage_.error);
        }
    }
    
    /**
     * @brief Destructor - destroys active union member
     */
    ~HALResult() noexcept {
        destroy();
    }
    
    /**
     * @brief Move assignment operator
     * @param other Result to move from
     * @return Reference to this result
     */
    HALResult& operator=(HALResult&& other) noexcept {
        if (this != &other) {
            destroy();
            has_value_ = other.has_value_;
            if (has_value_) {
                new(&storage_.value) T(std::move(other.storage_.value));
            } else {
                new(&storage_.error) E(std::move(other.storage_.error));
            }
        }
        return *this;
    }
    
    /**
     * @brief Copy assignment operator
     * @param other Result to copy from
     * @return Reference to this result
     */
    HALResult& operator=(const HALResult& other) {
        if (this != &other) {
            destroy();
            has_value_ = other.has_value_;
            if (has_value_) {
                new(&storage_.value) T(other.storage_.value);
            } else {
                new(&storage_.error) E(other.storage_.error);
            }
        }
        return *this;
    }
    
    // Static factory methods for clear construction
    
    /**
     * @brief Create successful result
     * @param value The success value
     * @return HALResult containing the success value
     */
    static HALResult success(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>) {
        return HALResult(std::move(value));
    }
    
    static HALResult success(const T& value) noexcept(std::is_nothrow_copy_constructible_v<T>) {
        return HALResult(value);
    }
    
    /**
     * @brief Create error result
     * @param error The error value
     * @return HALResult containing the error
     */
    static HALResult error(E&& error) noexcept(std::is_nothrow_move_constructible_v<E>) {
        return HALResult(std::move(error));
    }
    
    static HALResult error(const E& error) noexcept(std::is_nothrow_copy_constructible_v<E>) {
        return HALResult(error);
    }
    
    // Status checking methods
    
    /**
     * @brief Check if result contains a success value
     * @return true if successful
     */
    constexpr bool is_ok() const noexcept { return has_value_; }
    
    /**
     * @brief Check if result contains an error
     * @return true if error
     */
    constexpr bool is_err() const noexcept { return !has_value_; }
    
    /**
     * @brief Check if result contains a success value (alias for is_ok)
     * @return true if successful
     */
    constexpr bool is_success() const noexcept { return has_value_; }
    
    /**
     * @brief Check if result contains an error (alias for is_err)
     * @return true if error
     */
    constexpr bool is_error() const noexcept { return !has_value_; }
    
    /**
     * @brief Boolean conversion (true if success)
     * @return true if successful
     */
    explicit constexpr operator bool() const noexcept { return has_value_; }
    
    // Value access methods
    
    /**
     * @brief Get reference to success value
     * @return Reference to the success value
     * @warning Only call if is_ok() returns true
     */
    T& value() & noexcept {
        return storage_.value;
    }
    
    /**
     * @brief Get const reference to success value
     * @return Const reference to the success value
     * @warning Only call if is_ok() returns true
     */
    const T& value() const & noexcept {
        return storage_.value;
    }
    
    /**
     * @brief Get rvalue reference to success value
     * @return Rvalue reference to the success value
     * @warning Only call if is_ok() returns true
     */
    T&& value() && noexcept {
        return std::move(storage_.value);
    }
    
    /**
     * @brief Get const rvalue reference to success value
     * @return Const rvalue reference to the success value
     * @warning Only call if is_ok() returns true
     */
    const T&& value() const && noexcept {
        return std::move(storage_.value);
    }
    
    /**
     * @brief Get reference to error value
     * @return Reference to the error value
     * @warning Only call if is_err() returns true
     */
    E& error() & noexcept {
        return storage_.error;
    }
    
    /**
     * @brief Get const reference to error value
     * @return Const reference to the error value
     * @warning Only call if is_err() returns true
     */
    const E& error() const & noexcept {
        return storage_.error;
    }
    
    /**
     * @brief Get rvalue reference to error value
     * @return Rvalue reference to the error value
     * @warning Only call if is_err() returns true
     */
    E&& error() && noexcept {
        return std::move(storage_.error);
    }
    
    /**
     * @brief Get const rvalue reference to error value
     * @return Const rvalue reference to the error value
     * @warning Only call if is_err() returns true
     */
    const E&& error() const && noexcept {
        return std::move(storage_.error);
    }
    
    /**
     * @brief Get value or return default
     * @param default_value Value to return if error
     * @return Success value or default
     */
    T value_or(T&& default_value) const & {
        return has_value_ ? storage_.value : std::move(default_value);
    }
    
    T value_or(T&& default_value) && {
        return has_value_ ? std::move(storage_.value) : std::move(default_value);
    }
    
    // Monadic operations for functional-style error handling
    
    /**
     * @brief Transform success value with function
     * @tparam F Function type
     * @param func Function to apply to success value
     * @return New result with transformed value or propagated error
     */
    template<typename F>
    auto map(F&& func) & -> HALResult<decltype(func(std::declval<T&>())), E> {
        using U = decltype(func(std::declval<T&>()));
        if (has_value_) {
            return HALResult<U, E>::success(func(storage_.value));
        } else {
            return HALResult<U, E>::error(storage_.error);
        }
    }
    
    template<typename F>
    auto map(F&& func) && -> HALResult<decltype(func(std::declval<T&&>())), E> {
        using U = decltype(func(std::declval<T&&>()));
        if (has_value_) {
            return HALResult<U, E>::success(func(std::move(storage_.value)));
        } else {
            return HALResult<U, E>::error(std::move(storage_.error));
        }
    }
    
    /**
     * @brief Chain operations that return HALResult
     * @tparam F Function type
     * @param func Function to apply to success value
     * @return Result returned by function or propagated error
     */
    template<typename F>
    auto and_then(F&& func) & -> decltype(func(std::declval<T&>())) {
        if (has_value_) {
            return func(storage_.value);
        } else {
            using ResultType = decltype(func(std::declval<T&>()));
            return ResultType::error(storage_.error);
        }
    }
    
    template<typename F>
    auto and_then(F&& func) && -> decltype(func(std::declval<T&&>())) {
        if (has_value_) {
            return func(std::move(storage_.value));
        } else {
            using ResultType = decltype(func(std::declval<T&&>()));
            return ResultType::error(std::move(storage_.error));
        }
    }
    
    /**
     * @brief Transform error with function
     * @tparam F Function type
     * @param func Function to apply to error value
     * @return New result with transformed error or propagated success
     */
    template<typename F>
    auto map_error(F&& func) & -> HALResult<T, decltype(func(std::declval<E&>()))> {
        using U = decltype(func(std::declval<E&>()));
        if (has_value_) {
            return HALResult<T, U>::success(storage_.value);
        } else {
            return HALResult<T, U>::error(func(storage_.error));
        }
    }
    
    template<typename F>
    auto map_error(F&& func) && -> HALResult<T, decltype(func(std::declval<E&&>()))> {
        using U = decltype(func(std::declval<E&&>()));
        if (has_value_) {
            return HALResult<T, U>::success(std::move(storage_.value));
        } else {
            return HALResult<T, U>::error(func(std::move(storage_.error)));
        }
    }
};

/**
 * @brief Specialization for void results
 * 
 * HALResult<void> for operations that don't return a value
 * but still need success/error status.
 */
template<typename E>
class HALResult<void, E> {
private:
    union Storage {
        char dummy;  // Placeholder for void
        E error;
        
        Storage() noexcept : dummy(0) {}
        ~Storage() noexcept {}
    };
    
    Storage storage_;
    bool has_value_;
    
    void destroy() noexcept {
        if (!has_value_) {
            if constexpr (!std::is_trivially_destructible_v<E>) {
                storage_.error.~E();
            }
        }
    }

public:
    using value_type = void;
    using error_type = E;
    
    /**
     * @brief Construct successful void result
     */
    HALResult() noexcept : has_value_(true) {}
    
    /**
     * @brief Construct error result
     * @param error The error value
     */
    HALResult(E&& error) noexcept(std::is_nothrow_move_constructible_v<E>)
        : has_value_(false) {
        new(&storage_.error) E(std::move(error));
    }
    
    HALResult(const E& error) noexcept(std::is_nothrow_copy_constructible_v<E>)
        : has_value_(false) {
        new(&storage_.error) E(error);
    }
    
    /**
     * @brief Move constructor
     */
    HALResult(HALResult&& other) noexcept : has_value_(other.has_value_) {
        if (!has_value_) {
            new(&storage_.error) E(std::move(other.storage_.error));
        }
    }
    
    /**
     * @brief Copy constructor
     */
    HALResult(const HALResult& other) : has_value_(other.has_value_) {
        if (!has_value_) {
            new(&storage_.error) E(other.storage_.error);
        }
    }
    
    /**
     * @brief Destructor
     */
    ~HALResult() noexcept {
        destroy();
    }
    
    /**
     * @brief Move assignment
     */
    HALResult& operator=(HALResult&& other) noexcept {
        if (this != &other) {
            destroy();
            has_value_ = other.has_value_;
            if (!has_value_) {
                new(&storage_.error) E(std::move(other.storage_.error));
            }
        }
        return *this;
    }
    
    /**
     * @brief Copy assignment
     */
    HALResult& operator=(const HALResult& other) {
        if (this != &other) {
            destroy();
            has_value_ = other.has_value_;
            if (!has_value_) {
                new(&storage_.error) E(other.storage_.error);
            }
        }
        return *this;
    }
    
    // Static factory methods
    
    /**
     * @brief Create successful void result
     * @return HALResult representing success
     */
    static HALResult success() noexcept {
        return HALResult();
    }
    
    /**
     * @brief Create error result
     * @param error The error value
     * @return HALResult containing the error
     */
    static HALResult error(E&& error) noexcept(std::is_nothrow_move_constructible_v<E>) {
        return HALResult(std::move(error));
    }
    
    static HALResult error(const E& error) noexcept(std::is_nothrow_copy_constructible_v<E>) {
        return HALResult(error);
    }
    
    // Status checking
    
    constexpr bool is_ok() const noexcept { return has_value_; }
    constexpr bool is_err() const noexcept { return !has_value_; }
    constexpr bool is_success() const noexcept { return has_value_; }
    constexpr bool is_error() const noexcept { return !has_value_; }
    explicit constexpr operator bool() const noexcept { return has_value_; }
    
    // Error access
    
    E& error() & noexcept { return storage_.error; }
    const E& error() const & noexcept { return storage_.error; }
    E&& error() && noexcept { return std::move(storage_.error); }
    const E&& error() const && noexcept { return std::move(storage_.error); }
    
    // Monadic operations
    
    template<typename F>
    auto map(F&& func) & -> HALResult<decltype(func()), E> {
        using U = decltype(func());
        if (has_value_) {
            return HALResult<U, E>::success(func());
        } else {
            return HALResult<U, E>::error(storage_.error);
        }
    }
    
    template<typename F>
    auto and_then(F&& func) & -> decltype(func()) {
        if (has_value_) {
            return func();
        } else {
            using ResultType = decltype(func());
            return ResultType::error(storage_.error);
        }
    }
    
    template<typename F>
    auto map_error(F&& func) & -> HALResult<void, decltype(func(std::declval<E&>()))> {
        using U = decltype(func(std::declval<E&>()));
        if (has_value_) {
            return HALResult<void, U>::success();
        } else {
            return HALResult<void, U>::error(func(storage_.error));
        }
    }
};

// Type aliases for common HAL result types
using HALVoidResult = HALResult<void, HALError>;

} // namespace flight::hal
