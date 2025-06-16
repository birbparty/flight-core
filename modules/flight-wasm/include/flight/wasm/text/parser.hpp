#ifndef FLIGHT_WASM_TEXT_PARSER_HPP
#define FLIGHT_WASM_TEXT_PARSER_HPP

/**
 * @file parser.hpp
 * @brief WebAssembly Text Format (WAT) parsing functionality
 * 
 * This header provides forward declarations and basic structure for parsing
 * WebAssembly Text Format files. Full implementation is planned for future
 * development phases focused on text format support.
 */

#include <cstdint>
#include <string>
#include <string_view>

namespace flight::wasm {

    // Forward declarations
    class Module;
    class Error;
    template<typename T> class Result;

    /**
     * @brief WebAssembly Text Format parser
     * 
     * This parser converts WebAssembly Text Format (WAT) files into Module objects.
     * Currently this is a placeholder for future text format support.
     */
    class TextParser {
    public:
        /**
         * @brief Parse a WebAssembly text format module
         */
        static Result<Module> parse(std::string_view text) noexcept;

        /**
         * @brief Parse a WebAssembly text format module from a file
         */
        static Result<Module> parse_file(const std::string& filename) noexcept;

        /**
         * @brief Validate WebAssembly text format syntax
         */
        static Result<void> validate(std::string_view text) noexcept;

        /**
         * @brief Check if text appears to be valid WebAssembly text format
         */
        static bool is_wat_text(std::string_view text) noexcept;

    private:
        TextParser() = default;

        // Internal parsing methods (to be implemented in future)
        struct ParseContext;
        Result<Module> parse_module(ParseContext& context) noexcept;
        Result<void> parse_declarations(ParseContext& context, Module& module) noexcept;
    };

    /**
     * @brief WebAssembly text format encoder (Module to WAT)
     * 
     * This encoder converts Module objects back to WebAssembly Text Format.
     * Currently this is a placeholder for future text format support.
     */
    class TextEncoder {
    public:
        /**
         * @brief Encode a WebAssembly module to text format
         */
        static Result<std::string> encode(const Module& module) noexcept;

        /**
         * @brief Encode a WebAssembly module to a text file
         */
        static Result<void> encode_to_file(const Module& module, const std::string& filename) noexcept;

        /**
         * @brief Encoding options for text format output
         */
        struct EncodingOptions {
            bool pretty_print = true;
            bool include_comments = false;
            bool use_folded_expressions = true;
            size_t indent_size = 2;
            size_t max_line_length = 80;
        };

        /**
         * @brief Encode with custom formatting options
         */
        static Result<std::string> encode_with_options(const Module& module, 
                                                     const EncodingOptions& options) noexcept;

    private:
        TextEncoder() = default;
    };

    /**
     * @brief Text format validation utilities
     */
    class TextValidator {
    public:
        /**
         * @brief Validate text format syntax
         */
        static Result<void> validate_syntax(std::string_view text) noexcept;

        /**
         * @brief Check for common text format errors
         */
        static Result<void> validate_semantics(std::string_view text) noexcept;

    private:
        TextValidator() = default;
    };

} // namespace flight::wasm

#endif // FLIGHT_WASM_TEXT_PARSER_HPP
