#ifndef FLIGHT_WASM_WASM_HPP
#define FLIGHT_WASM_WASM_HPP

/**
 * @file wasm.hpp
 * @brief Main entry point for Flight WASM library
 * 
 * This header provides access to all Flight WASM functionality including:
 * - WebAssembly type system (values, instructions, modules)
 * - Binary format parsing and encoding
 * - Validation and error handling
 * - Platform-specific optimizations
 * - Text format support (future)
 * 
 * Simply include this header to access the complete Flight WASM toolkit.
 */

// Core utilities - platform detection and error handling
#include <flight/wasm/utilities/platform.hpp>
#include <flight/wasm/utilities/error.hpp>

// WebAssembly type system
#include <flight/wasm/types/values.hpp>
#include <flight/wasm/types/instructions.hpp>
#include <flight/wasm/types/modules.hpp>

// Binary format handling
#include <flight/wasm/binary/parser.hpp>
#include <flight/wasm/binary/encoder.hpp>
#include <flight/wasm/binary/validation.hpp>

// Text format support (placeholder for future development)
#include <flight/wasm/text/parser.hpp>

namespace flight::wasm {

    /**
     * @brief Flight WASM library version information
     */
    namespace version {
        constexpr int MAJOR = 1;
        constexpr int MINOR = 0;
        constexpr int PATCH = 0;
        constexpr const char* STRING = "1.0.0";
        constexpr const char* BUILD_TYPE = 
            #ifdef FLIGHT_WASM_DEBUG
                "debug";
            #else
                "release";
            #endif
    }

    /**
     * @brief WebAssembly specification constants
     */
    namespace spec {
        constexpr uint32_t BINARY_MAGIC = 0x6d736100;    // "\0asm"
        constexpr uint32_t BINARY_VERSION = 1;
        constexpr size_t PAGE_SIZE = 65536;              // 64KiB
        constexpr size_t MAX_PAGES = 65536;              // 4GiB max memory
        constexpr size_t MAX_FUNCTION_LOCALS = 50000;    // Implementation limit
        constexpr size_t MAX_FUNCTION_PARAMS = 1000;     // Implementation limit
        constexpr size_t MAX_FUNCTION_RESULTS = 1000;    // Implementation limit
        constexpr size_t MAX_TABLE_SIZE = UINT32_MAX;
        constexpr size_t MAX_MEMORY_SIZE = UINT32_MAX;
    }

    /**
     * @brief High-level API for common WebAssembly operations
     * 
     * This namespace provides convenient functions for the most common
     * WebAssembly operations without requiring detailed knowledge of
     * the underlying implementation.
     */
    namespace api {

        /**
         * @brief Parse a WebAssembly binary module from raw data
         */
        inline Result<Module> parse_binary(span<const uint8_t> data) {
            return BinaryParser::parse(data);
        }

        /**
         * @brief Parse a WebAssembly binary module from a file
         */
        inline Result<Module> parse_binary_file(const std::string& filename) {
            return BinaryParser::parse_file(filename);
        }

        /**
         * @brief Encode a module to WebAssembly binary format
         */
        inline Result<std::vector<uint8_t>> encode_binary(const Module& module) {
            return BinaryEncoder::encode(module);
        }

        /**
         * @brief Validate a WebAssembly binary
         */
        inline ValidationResult validate_binary(span<const uint8_t> data, 
                                               ValidationLevel level = ValidationLevel::Complete) {
            return BinaryValidator::validate(data, level);
        }

        /**
         * @brief Quick check if data appears to be a valid WebAssembly binary
         */
        inline bool is_valid_wasm(span<const uint8_t> data) {
            return BinaryValidator::is_valid_wasm(data);
        }

        /**
         * @brief Parse a WebAssembly text format module (future)
         */
        inline Result<Module> parse_text(std::string_view text) {
            return TextParser::parse(text);
        }

        /**
         * @brief Encode a module to WebAssembly text format (future)
         */
        inline Result<std::string> encode_text(const Module& module) {
            return TextEncoder::encode(module);
        }

    } // namespace api

    /**
     * @brief Convenience type aliases for common operations
     */
    using ParseResult = Result<Module>;
    using ValidationResult = ValidationResult;
    using BinaryData = std::vector<uint8_t>;
    using TextData = std::string;

    /**
     * @brief Library initialization and configuration
     */
    namespace config {
        
        /**
         * @brief Global configuration options for the Flight WASM library
         */
        struct Options {
            // Validation settings
            ValidationLevel default_validation_level = ValidationLevel::Complete;
            bool strict_validation = true;
            bool collect_warnings = true;
            
            // Performance settings
            bool enable_fast_path_optimizations = true;
            bool enable_simd_optimizations = platform::has_neon_simd;
            size_t parser_buffer_size = 1024 * 1024; // 1MB
            
            // Memory limits for embedded platforms
            size_t max_module_size = platform::max_memory / 4; // 25% of available memory
            size_t max_function_size = 1024 * 1024; // 1MB
            
            // Debug options
            bool enable_debug_info = FLIGHT_WASM_DEBUG;
            bool verbose_errors = FLIGHT_WASM_DEBUG;
        };

        /**
         * @brief Get the current global configuration
         */
        const Options& get_global_options() noexcept;

        /**
         * @brief Set the global configuration
         */
        void set_global_options(const Options& options) noexcept;

        /**
         * @brief Reset to default configuration
         */
        void reset_to_defaults() noexcept;

        /**
         * @brief Get platform-optimized default configuration
         */
        Options get_platform_defaults() noexcept;

    } // namespace config

    /**
     * @brief Utility functions for working with WebAssembly data
     */
    namespace utils {

        /**
         * @brief Create a span from a vector
         */
        template<typename T>
        constexpr span<const T> make_span(const std::vector<T>& vec) noexcept {
            return span<const T>(vec.data(), vec.size());
        }

        /**
         * @brief Create a span from an array
         */
        template<typename T, size_t N>
        constexpr span<const T> make_span(const T (&arr)[N]) noexcept {
            return span<const T>(arr, N);
        }

        /**
         * @brief Get a human-readable string for a value type
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

        /**
         * @brief Get platform information as a string
         */
        std::string get_platform_info() noexcept;

        /**
         * @brief Get library build information
         */
        std::string get_build_info() noexcept;

    } // namespace utils

} // namespace flight::wasm

/**
 * @brief Global convenience macros for common operations
 */
#define FLIGHT_WASM_PARSE_BINARY(data) flight::wasm::api::parse_binary(data)
#define FLIGHT_WASM_VALIDATE_BINARY(data) flight::wasm::api::validate_binary(data)
#define FLIGHT_WASM_IS_VALID(data) flight::wasm::api::is_valid_wasm(data)

/**
 * @brief Version check macros
 */
#define FLIGHT_WASM_VERSION_CHECK(major, minor, patch) \
    ((flight::wasm::version::MAJOR > (major)) || \
     (flight::wasm::version::MAJOR == (major) && flight::wasm::version::MINOR > (minor)) || \
     (flight::wasm::version::MAJOR == (major) && flight::wasm::version::MINOR == (minor) && flight::wasm::version::PATCH >= (patch)))

#endif // FLIGHT_WASM_WASM_HPP
