#ifndef FLIGHT_WASM_BINARY_VALIDATION_HPP
#define FLIGHT_WASM_BINARY_VALIDATION_HPP

/**
 * @file validation.hpp
 * @brief WebAssembly binary format validation functionality
 * 
 * This header provides forward declarations and basic structure for validating
 * WebAssembly binary format files. Full implementation will be completed in
 * the validation framework tasks.
 */

#include <cstdint>
#include <cstddef>
#include <vector>

namespace flight::wasm {

    // Forward declarations
    class Module;
    class Error;
    enum class ValueType : uint8_t;
    template<typename T> class Result;
    template<typename T> class span;

    /**
     * @brief Validation levels for binary format checking
     */
    enum class ValidationLevel : uint8_t {
        /**
         * @brief Basic structural validation
         * 
         * Checks:
         * - Magic number and version
         * - Section ordering and sizes
         * - LEB128 encoding validity
         * - UTF-8 string validity
         */
        Structural = 0,

        /**
         * @brief Type-level validation
         * 
         * Includes structural validation plus:
         * - Type indices validity
         * - Function signature matching
         * - Import/export consistency
         * - Memory and table limits
         */
        Semantic = 1,

        /**
         * @brief Full WebAssembly specification validation
         * 
         * Includes semantic validation plus:
         * - Instruction sequence validation
         * - Stack type checking
         * - Control flow validation
         * - Constant expression validation
         */
        Complete = 2
    };

    /**
     * @brief Validation result information
     */
    struct ValidationResult {
        bool is_valid = false;
        ValidationLevel level_validated = ValidationLevel::Structural;
        size_t bytes_validated = 0;
        
        // Error details (implementation will use proper Error types)
        uint32_t error_code = 0;
        const char* error_message = nullptr;
        size_t error_offset = 0;

        ValidationResult() = default;
        ValidationResult(bool valid, ValidationLevel level, size_t bytes)
            : is_valid(valid), level_validated(level), bytes_validated(bytes) {}
    };

    /**
     * @brief WebAssembly binary format validator
     * 
     * Provides comprehensive validation of WebAssembly binary format files
     * with configurable validation levels and detailed error reporting.
     */
    class BinaryValidator {
    public:
        /**
         * @brief Validate a WebAssembly binary with specified validation level
         */
        static ValidationResult validate(span<const uint8_t> data, 
                                       ValidationLevel level = ValidationLevel::Complete) noexcept;

        /**
         * @brief Quick check if data appears to be a valid WebAssembly binary
         */
        static bool is_valid_wasm(span<const uint8_t> data) noexcept;

        /**
         * @brief Validate only the binary format structure (no semantic checking)
         */
        static ValidationResult validate_structure(span<const uint8_t> data) noexcept;

        /**
         * @brief Validate module semantics (requires parsed module)
         */
        static ValidationResult validate_semantics(const Module& module) noexcept;

        /**
         * @brief Validate WebAssembly instructions and control flow
         */
        static ValidationResult validate_instructions(const Module& module) noexcept;

        /**
         * @brief Streaming validation for large files
         */
        static Result<void> validate_streaming(span<const uint8_t> data,
                                             ValidationLevel level = ValidationLevel::Complete) noexcept;

    private:
        BinaryValidator() = default;

        // Internal validation methods (to be implemented)
        static ValidationResult validate_header(span<const uint8_t> data) noexcept;
        static ValidationResult validate_sections(span<const uint8_t> data, ValidationLevel level) noexcept;
        static ValidationResult validate_section_structure(span<const uint8_t> section_data, uint8_t section_id) noexcept;

        // Section-specific validators
        static ValidationResult validate_type_section(span<const uint8_t> data) noexcept;
        static ValidationResult validate_import_section(span<const uint8_t> data) noexcept;
        static ValidationResult validate_function_section(span<const uint8_t> data) noexcept;
        static ValidationResult validate_table_section(span<const uint8_t> data) noexcept;
        static ValidationResult validate_memory_section(span<const uint8_t> data) noexcept;
        static ValidationResult validate_global_section(span<const uint8_t> data) noexcept;
        static ValidationResult validate_export_section(span<const uint8_t> data) noexcept;
        static ValidationResult validate_start_section(span<const uint8_t> data) noexcept;
        static ValidationResult validate_element_section(span<const uint8_t> data) noexcept;
        static ValidationResult validate_code_section(span<const uint8_t> data) noexcept;
        static ValidationResult validate_data_section(span<const uint8_t> data) noexcept;
        static ValidationResult validate_custom_section(span<const uint8_t> data) noexcept;

        // Low-level validation utilities
        static bool validate_leb128_encoding(span<const uint8_t> data, size_t& bytes_read) noexcept;
        static bool validate_utf8_string(span<const uint8_t> data) noexcept;
        static bool validate_section_order(uint8_t current_section, uint8_t previous_section) noexcept;
    };

    /**
     * @brief Specialized validator for WebAssembly instruction sequences
     * 
     * This validator focuses specifically on instruction-level validation
     * including stack effects, control flow, and type checking.
     */
    class InstructionValidator {
    public:
        /**
         * @brief Validate a sequence of instructions
         */
        static ValidationResult validate_instruction_sequence(span<const uint8_t> code) noexcept;

        /**
         * @brief Validate control flow structure (blocks, branches, etc.)
         */
        static ValidationResult validate_control_flow(span<const uint8_t> code) noexcept;

        /**
         * @brief Validate stack effects and type consistency
         */
        static ValidationResult validate_stack_effects(span<const uint8_t> code) noexcept;

        /**
         * @brief Validate constant expressions (globals, element offsets, etc.)
         */
        static ValidationResult validate_constant_expression(span<const uint8_t> code) noexcept;

    private:
        InstructionValidator() = default;

        // Instruction validation state
        struct ValidationState {
            std::vector<ValueType> stack;
            std::vector<uint32_t> label_stack;
            size_t instruction_count = 0;
            bool unreachable = false;
        };

        static ValidationResult validate_instruction(uint8_t opcode, span<const uint8_t> immediate_data, 
                                                   ValidationState& state) noexcept;
    };

    /**
     * @brief Performance-focused fast validator
     * 
     * This validator prioritizes speed over detailed error reporting
     * and is suitable for production validation where performance matters.
     */
    class FastValidator {
    public:
        /**
         * @brief Fast validation that only checks critical safety requirements
         */
        static bool is_safe_to_parse(span<const uint8_t> data) noexcept;

        /**
         * @brief Fast structural validation without detailed error reporting
         */
        static bool has_valid_structure(span<const uint8_t> data) noexcept;

        /**
         * @brief Fast check for common malformed patterns
         */
        static bool has_malformed_patterns(span<const uint8_t> data) noexcept;

    private:
        FastValidator() = default;
    };

    /**
     * @brief Validation configuration options
     */
    struct ValidationOptions {
        ValidationLevel level = ValidationLevel::Complete;
        bool collect_warnings = true;
        bool strict_section_ordering = true;
        bool allow_unknown_sections = true;
        bool validate_utf8_strings = true;
        size_t max_function_size = 1024 * 1024;  // 1MB
        size_t max_section_size = 16 * 1024 * 1024;  // 16MB
        size_t max_module_size = 256 * 1024 * 1024;  // 256MB

        ValidationOptions() = default;
    };

    /**
     * @brief Configurable validator with custom validation options
     */
    class ConfigurableValidator {
    public:
        /**
         * @brief Create a validator with specific options
         */
        explicit ConfigurableValidator(ValidationOptions options);

        /**
         * @brief Validate with the configured options
         */
        ValidationResult validate(span<const uint8_t> data) noexcept;

        /**
         * @brief Update validation options
         */
        void set_options(ValidationOptions options) noexcept;

        /**
         * @brief Get current validation options
         */
        const ValidationOptions& get_options() const noexcept;

    private:
        ValidationOptions options_;
    };

} // namespace flight::wasm

#endif // FLIGHT_WASM_BINARY_VALIDATION_HPP
