/**
 * @file hal_error_macros.hpp
 * @brief Flight HAL Error Propagation Macros and Utilities
 * 
 * Provides convenient macros for monadic error handling patterns,
 * early returns, and error propagation with minimal boilerplate.
 */

#pragma once

#include "hal_result.hpp"

namespace flight::hal {

/**
 * @brief Utility for converting errors between different HALResult types
 * @tparam TargetError Target error type
 * @tparam SourceError Source error type
 * @param source_error Error to convert
 * @return Converted error
 */
template<typename TargetError, typename SourceError>
constexpr TargetError convert_error(const SourceError& source_error) {
    if constexpr (std::is_same_v<TargetError, SourceError>) {
        return source_error;
    } else {
        // Default conversion for HALError types
        return TargetError(source_error.category(), source_error.code(), 
                          source_error.message(), source_error.context());
    }
}

/**
 * @brief Error context builder for adding location information
 */
class ErrorContext {
public:
    constexpr ErrorContext(const char* function, const char* file, int line) noexcept
        : function_(function), file_(file), line_(line) {}
    
    template<typename E>
    E with_context(E error) const noexcept {
        // For HALError, we could extend to include location context
        // For now, just return the error as-is
        return error;
    }
    
private:
    const char* function_;
    const char* file_;
    int line_;
};

/**
 * @brief Helper function for trying operations and propagating errors
 * @tparam T Result value type
 * @tparam E Error type
 * @param result The HALResult to check
 * @param value_out Output parameter for the success value
 * @return true if successful, false if error (caller should return)
 */
template<typename T, typename E>
bool try_get_value(HALResult<T, E>&& result, T& value_out) {
    if (result.is_ok()) {
        value_out = std::move(result.value());
        return true;
    }
    return false;
}

/**
 * @brief Helper function for checking void results
 * @tparam E Error type
 * @param result The HALResult to check
 * @return true if successful, false if error (caller should return)
 */
template<typename E>
bool try_check_void(const HALResult<void, E>& result) {
    return result.is_ok();
}

/**
 * @brief Common pattern: Try operation and assign result on success
 * 
 * Usage:
 *   auto result = get_device();
 *   if (result.is_err()) return result.error();
 *   auto device = std::move(result.value());
 */
template<typename T, typename E>
inline T unwrap_or_return(HALResult<T, E>&& result) {
    // This is meant to be used in a function that can return the error
    // The actual early return must be handled by the caller
    return std::move(result.value());
}

/**
 * @brief Get value or default
 * 
 * Usage:
 *   auto value = hal::value_or(get_config(), default_config);
 */
template<typename T, typename E, typename U>
inline T value_or(const HALResult<T, E>& result, U&& default_value) {
    return result.is_ok() ? result.value() : static_cast<T>(std::forward<U>(default_value));
}

/**
 * @brief Helper class for match-like result handling
 */
template<typename Result>
class ResultMatcher {
public:
    explicit ResultMatcher(Result&& result) : result_(std::move(result)) {}
    
    template<typename SuccessFunc>
    auto on_success(SuccessFunc&& func) && {
        if (result_.is_ok()) {
            if constexpr (std::is_void_v<typename Result::value_type>) {
                func();
            } else {
                func(std::move(result_.value()));
            }
        }
        return std::move(*this);
    }
    
    template<typename ErrorFunc>
    auto on_error(ErrorFunc&& func) && {
        if (result_.is_err()) {
            func(std::move(result_.error()));
        }
        return std::move(*this);
    }
    
private:
    Result result_;
};

} // namespace flight::hal

/**
 * @brief Try macro for void results
 * 
 * Usage:
 *   HAL_TRY_VOID(some_void_operation());
 */
#define HAL_TRY_VOID(expr) \
    do { \
        auto __hal_result = (expr); \
        if (__hal_result.is_err()) { \
            return flight::hal::HALVoidResult::error(std::move(__hal_result.error())); \
        } \
    } while(0)

/**
 * @brief Return early with error
 * 
 * Usage:
 *   HAL_RETURN_ERROR(errors::invalid_parameter(1, "param"));
 */
#define HAL_RETURN_ERROR(error_expr) \
    return flight::hal::HALVoidResult::error(error_expr)

/**
 * @brief Return early with HAL error
 * 
 * Usage:
 *   HAL_RETURN_HAL_ERROR(HALErrorCategory::Configuration, 1, "Invalid config");
 */
#define HAL_RETURN_HAL_ERROR(category, code, message) \
    return flight::hal::HALVoidResult::error( \
        flight::hal::HALError(category, code, message))

/**
 * @brief Return early with HAL error and context
 * 
 * Usage:
 *   HAL_RETURN_HAL_ERROR_CTX(HALErrorCategory::Hardware, 1, "Init failed", "GPU driver");
 */
#define HAL_RETURN_HAL_ERROR_CTX(category, code, message, context) \
    return flight::hal::HALVoidResult::error( \
        flight::hal::HALError(category, code, message, context))

/**
 * @brief Check condition and return error if false
 * 
 * Usage:
 *   HAL_ENSURE(ptr != nullptr, errors::invalid_parameter(0, "ptr"));
 */
#define HAL_ENSURE(condition, error_expr) \
    do { \
        if (!(condition)) { \
            return flight::hal::HALVoidResult::error(error_expr); \
        } \
    } while(0)

/**
 * @brief Check condition and return HAL error if false
 * 
 * Usage:
 *   HAL_ENSURE_HAL(ptr != nullptr, HALErrorCategory::Validation, 1, "Null pointer");
 */
#define HAL_ENSURE_HAL(condition, category, code, message) \
    do { \
        if (!(condition)) { \
            return flight::hal::HALVoidResult::error( \
                flight::hal::HALError(category, code, message)); \
        } \
    } while(0)

/**
 * @brief Execute block only on success
 * 
 * Usage:
 *   HAL_IF_OK(some_operation()) {
 *       // This block runs only on success
 *   }
 */
#define HAL_IF_OK(expr) \
    if (auto __hal_result = (expr); __hal_result.is_ok())

/**
 * @brief Execute block only on error
 * 
 * Usage:
 *   HAL_IF_ERR(some_operation()) {
 *       // Handle error: __hal_result.error()
 *   }
 */
#define HAL_IF_ERR(expr) \
    if (auto __hal_result = (expr); __hal_result.is_err())

/**
 * @brief Context macro for adding location information to errors
 * 
 * Usage:
 *   auto ctx = HAL_CONTEXT();
 *   return ctx.with_context(some_error);
 */
#define HAL_CONTEXT() \
    flight::hal::ErrorContext(__FUNCTION__, __FILE__, __LINE__)

/**
 * @brief Match-like pattern for handling results
 * 
 * Usage:
 *   HAL_MATCH(some_operation())
 *       .on_success([](auto&& value) { /* handle success */ })
 *       .on_error([](auto&& error) { /* handle error */ });
 */
#define HAL_MATCH(expr) \
    flight::hal::ResultMatcher(expr)

/*
 * Common error handling patterns - recommended usage:
 *
 * 1. Simple error propagation:
 *    auto result = some_operation();
 *    if (result.is_err()) {
 *        return HALVoidResult::error(std::move(result.error()));
 *    }
 *    auto value = std::move(result.value());
 *
 * 2. Using helper functions:
 *    auto result = some_operation();
 *    if (result.is_err()) return result.error();
 *    auto value = flight::hal::unwrap_or_return(std::move(result));
 *
 * 3. With void operations:
 *    HAL_TRY_VOID(initialize_hardware());
 *
 * 4. With conditions:
 *    HAL_ENSURE_HAL(device != nullptr, HALErrorCategory::Validation, 1, "Device null");
 *
 * 5. Pattern matching:
 *    HAL_MATCH(get_device())
 *        .on_success([](auto&& device) { handle success })
 *        .on_error([](auto&& error) { handle error });
 */
