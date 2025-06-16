/**
 * @file hal_logging.hpp
 * @brief Flight HAL Logging Integration for Error Handling
 * 
 * Provides lightweight logging integration for error tracking and debugging
 * with minimal overhead in release builds.
 */

#pragma once

#include "hal_error.hpp"
#include <cstdio>

namespace flight::hal {

/**
 * @brief Log severity levels
 */
enum class LogLevel : uint8_t {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
    Critical = 4
};

/**
 * @brief Simple HAL logger with minimal overhead
 * 
 * Designed for embedded systems with configurable output and
 * compile-time optimization for release builds.
 */
class HALLogger {
public:
    /**
     * @brief Log an error with context
     * @param error The HAL error to log
     * @param location Optional source location
     */
    static void log_error(const HALError& error, const char* location = nullptr) noexcept {
        log(LogLevel::Error, "HAL Error", error, location);
    }
    
    /**
     * @brief Log a warning
     * @param error The HAL error to log as warning
     * @param location Optional source location
     */
    static void log_warning(const HALError& error, const char* location = nullptr) noexcept {
        log(LogLevel::Warning, "HAL Warning", error, location);
    }
    
    /**
     * @brief Set minimum log level (compile-time configurable)
     * @param level Minimum level to log
     */
    static void set_log_level(LogLevel level) noexcept {
        min_log_level_ = level;
    }
    
    /**
     * @brief Check if a log level would be output
     * @param level Level to check
     * @return true if this level would be logged
     */
    static bool would_log(LogLevel level) noexcept {
        return static_cast<uint8_t>(level) >= static_cast<uint8_t>(min_log_level_);
    }
    
    /**
     * @brief Log a simple message
     * @param level Log level
     * @param message Message to log
     */
    static void log_message(LogLevel level, const char* message) noexcept {
        if (!would_log(level)) return;
        
        const char* level_str = level_to_string(level);
        
        // Simple printf-based logging for now
        // In a real implementation, this could be replaced with
        // platform-specific logging or telemetry systems
        printf("[%s] %s\n", level_str, message);
    }

private:
    static LogLevel min_log_level_;
    
    /**
     * @brief Internal logging implementation
     * @param level Log level
     * @param prefix Log prefix
     * @param error HAL error to log
     * @param location Source location
     */
    static void log(LogLevel level, const char* prefix, const HALError& error, 
                   const char* location) noexcept {
        if (!would_log(level)) return;
        
        const char* level_str = level_to_string(level);
        const char* category_str = category_to_string(error.category());
        
        // Format: [LEVEL] PREFIX: [CATEGORY] Message (Code: X) [Location]
        if (location && error.context()) {
            printf("[%s] %s: [%s] %s (Code: %u, Context: %s) [%s]\n",
                   level_str, prefix, category_str, error.message(), 
                   error.code(), error.context(), location);
        } else if (location) {
            printf("[%s] %s: [%s] %s (Code: %u) [%s]\n",
                   level_str, prefix, category_str, error.message(), 
                   error.code(), location);
        } else if (error.context()) {
            printf("[%s] %s: [%s] %s (Code: %u, Context: %s)\n",
                   level_str, prefix, category_str, error.message(), 
                   error.code(), error.context());
        } else {
            printf("[%s] %s: [%s] %s (Code: %u)\n",
                   level_str, prefix, category_str, error.message(), 
                   error.code());
        }
    }
    
    /**
     * @brief Convert log level to string
     * @param level Log level to convert
     * @return String representation
     */
    static constexpr const char* level_to_string(LogLevel level) noexcept {
        switch (level) {
            case LogLevel::Debug: return "DEBUG";
            case LogLevel::Info: return "INFO";
            case LogLevel::Warning: return "WARN";
            case LogLevel::Error: return "ERROR";
            case LogLevel::Critical: return "CRITICAL";
            default: return "UNKNOWN";
        }
    }
};

// Default log level - can be overridden at compile time
#ifndef HAL_DEFAULT_LOG_LEVEL
#define HAL_DEFAULT_LOG_LEVEL LogLevel::Warning
#endif

// Static member definition
inline LogLevel HALLogger::min_log_level_ = HAL_DEFAULT_LOG_LEVEL;

/**
 * @brief RAII error logger for automatic error tracking
 * 
 * Logs errors when HALResult objects are destroyed while containing errors.
 * Helps catch unhandled errors during development.
 */
template<typename T, typename E = HALError>
class ErrorTracker {
public:
    explicit ErrorTracker(HALResult<T, E>&& result, const char* location = nullptr) 
        : result_(std::move(result)), location_(location) {}
    
    ~ErrorTracker() {
        if (result_.is_err()) {
            if constexpr (std::is_same_v<E, HALError>) {
                HALLogger::log_error(result_.error(), location_);
            }
        }
    }
    
    // Move-only type
    ErrorTracker(const ErrorTracker&) = delete;
    ErrorTracker& operator=(const ErrorTracker&) = delete;
    ErrorTracker(ErrorTracker&&) = default;
    ErrorTracker& operator=(ErrorTracker&&) = default;
    
    /**
     * @brief Get the result (ownership transfer)
     * @return The tracked result
     */
    HALResult<T, E> take_result() && {
        return std::move(result_);
    }
    
    /**
     * @brief Check if result is successful
     * @return true if successful
     */
    bool is_ok() const noexcept { return result_.is_ok(); }
    
    /**
     * @brief Check if result is error
     * @return true if error
     */
    bool is_err() const noexcept { return result_.is_err(); }

private:
    HALResult<T, E> result_;
    const char* location_;
};

/**
 * @brief Create an error tracker for a result
 * @tparam T Result value type
 * @tparam E Error type
 * @param result Result to track
 * @param location Source location
 * @return Error tracker
 */
template<typename T, typename E>
auto track_errors(HALResult<T, E>&& result, const char* location = nullptr) {
    return ErrorTracker<T, E>(std::move(result), location);
}

} // namespace flight::hal

// Logging macros for convenience

/**
 * @brief Log HAL error
 */
#define HAL_LOG_ERROR(error) \
    flight::hal::HALLogger::log_error(error, __FUNCTION__)

/**
 * @brief Log HAL warning  
 */
#define HAL_LOG_WARNING(error) \
    flight::hal::HALLogger::log_warning(error, __FUNCTION__)

/**
 * @brief Log simple message
 */
#define HAL_LOG_MESSAGE(level, message) \
    flight::hal::HALLogger::log_message(level, message)

/**
 * @brief Track errors in result (RAII)
 */
#define HAL_TRACK_ERRORS(result) \
    flight::hal::track_errors(std::move(result), __FUNCTION__)

/**
 * @brief Debug-only logging (compiled out in release)
 */
#ifdef NDEBUG
#define HAL_LOG_DEBUG(message) ((void)0)
#else
#define HAL_LOG_DEBUG(message) \
    flight::hal::HALLogger::log_message(flight::hal::LogLevel::Debug, message)
#endif
