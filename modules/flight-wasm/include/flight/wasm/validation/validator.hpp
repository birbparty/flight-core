#ifndef FLIGHT_WASM_VALIDATION_VALIDATOR_HPP
#define FLIGHT_WASM_VALIDATION_VALIDATOR_HPP

/**
 * @file validator.hpp
 * @brief WebAssembly Type Validation Framework
 * 
 * This header implements the core type validation framework for WebAssembly modules
 * according to the WebAssembly Core Specification Section 3 (Validation).
 * 
 * The framework provides:
 * - Complete type safety validation
 * - Stack effect verification  
 * - Control flow validation
 * - Function signature checking
 * - Module-level validation
 * 
 * Performance targets:
 * - Dreamcast (SH-4): ≤20 cycles per instruction, ≤64KB validation memory
 * - PSP (MIPS): ≤15 cycles per instruction, ≤128KB validation memory
 * - PS Vita (ARM): ≤10 cycles per instruction, ≤256KB validation memory
 * - Desktop: ≤5 cycles per instruction, ≤1MB validation memory
 */

#include <flight/wasm/types/value.hpp>
#include <flight/wasm/types/values.hpp>
#include <flight/wasm/types/modules.hpp>
#include <flight/wasm/types/instructions.hpp>
#include <flight/wasm/utilities/error.hpp>
#include <flight/wasm/utilities/platform.hpp>
#include <vector>
#include <string>
#include <variant>
#include <optional>

namespace flight::wasm::validation {

    // =========================================================================
    // Validation Error Types
    // =========================================================================

    /**
     * @brief Validation error codes following WebAssembly specification
     */
    enum class ValidationErrorCode : uint16_t {
        // Type errors (Section 3.3.1)
        TypeMismatch = 1000,
        UnknownType = 1001,
        InvalidValueType = 1002,
        InvalidResultType = 1003,
        
        // Stack errors (Section 3.3.2)
        StackUnderflow = 2000,
        StackOverflow = 2001,
        InvalidStackHeight = 2002,
        UnexpectedStackContents = 2003,
        
        // Control flow errors (Section 3.3.6)
        InvalidLabelIndex = 3000,
        InvalidBranchTarget = 3001,
        MissingEndInstruction = 3002,
        UnreachableCodeViolation = 3003,
        InvalidBlockType = 3004,
        ControlStackUnderflow = 3005,
        
        // Function errors (Section 3.3.4)
        InvalidFunctionIndex = 4000,
        InvalidLocalIndex = 4001,
        FunctionSignatureMismatch = 4002,
        InvalidParameterCount = 4003,
        InvalidResultCount = 4004,
        
        // Global errors (Section 3.3.5)
        InvalidGlobalIndex = 5000,
        GlobalTypeMismatch = 5001,
        MutableGlobalWrite = 5002,
        ImmutableGlobalWrite = 5003,
        
        // Table errors (Section 3.3.7)  
        InvalidTableIndex = 6000,
        TableTypeMismatch = 6001,
        InvalidElementType = 6002,
        
        // Memory errors (Section 3.3.8)
        InvalidMemoryIndex = 7000,
        InvalidMemoryAlignment = 7001,
        InvalidMemoryOffset = 7002,
        
        // Module errors (Section 3.3)
        InvalidImport = 8000,
        InvalidExport = 8001,
        DuplicateExportName = 8002,
        InvalidStartFunction = 8003,
        ModuleStructureViolation = 8004,
        
        // Instruction errors (Section 3.3.6)
        InvalidInstruction = 9000,
        UnsupportedInstruction = 9001,
        InvalidImmediate = 9002,
        ConstantExpressionViolation = 9003,
        
        // Resource limits
        ValidationMemoryExhausted = 10000,
        ValidationStackTooDeep = 10001,
        ValidationTimeout = 10002
    };

    /**
     * @brief Detailed validation error with precise location and context
     */
    struct ValidationError {
        ValidationErrorCode code;
        std::string message;
        std::string context;
        
        // Location information
        size_t instruction_offset = 0;
        uint32_t function_index = UINT32_MAX;
        std::string function_name;
        
        // Stack trace for nested validation
        std::vector<std::string> validation_stack;
        
        ValidationError() = default;
        
        ValidationError(ValidationErrorCode c, std::string_view msg, 
                       size_t offset = 0, std::string_view ctx = "")
            : code(c), message(msg), context(ctx), instruction_offset(offset) {}
        
        ValidationError(ValidationErrorCode c, std::string_view msg,
                       uint32_t func_idx, size_t offset = 0, std::string_view ctx = "")
            : code(c), message(msg), context(ctx), instruction_offset(offset), function_index(func_idx) {}
        
        /**
         * @brief Add context to validation stack for error tracing
         */
        void push_context(std::string_view context_info) {
            validation_stack.emplace_back(context_info);
        }
        
        /**
         * @brief Format complete error message with context
         */
        std::string format() const {
            std::string result = message;
            
            if (function_index != UINT32_MAX) {
                result += " (function " + std::to_string(function_index);
                if (!function_name.empty()) {
                    result += " '" + function_name + "'";
                }
                result += ")";
            }
            
            if (instruction_offset != 0) {
                result += " at offset " + std::to_string(instruction_offset);
            }
            
            if (!context.empty()) {
                result += ": " + context;
            }
            
            // Add validation stack trace
            for (const auto& ctx : validation_stack) {
                result += "\n  in " + ctx;
            }
            
            return result;
        }
    };

    // =========================================================================
    // Function Type and Block Type Handling
    // =========================================================================

    /**
     * @brief Function type representation for validation
     */
    struct FunctionType {
        std::vector<ValueType> parameters;
        std::vector<ValueType> results;
        
        FunctionType() = default;
        FunctionType(std::vector<ValueType> params, std::vector<ValueType> rets)
            : parameters(std::move(params)), results(std::move(rets)) {}
        
        bool operator==(const FunctionType& other) const noexcept {
            return parameters == other.parameters && results == other.results;
        }
        
        bool operator!=(const FunctionType& other) const noexcept {
            return !(*this == other);
        }
        
        /**
         * @brief Get arity (number of parameters)
         */
        size_t arity() const noexcept { return parameters.size(); }
        
        /**
         * @brief Get result count
         */
        size_t result_arity() const noexcept { return results.size(); }
    };

    /**
     * @brief Block type for control instructions
     */
    using BlockType = std::variant<std::monostate, ValueType, uint32_t>;

    /**
     * @brief Resolve block type to function type
     */
    inline FunctionType resolve_block_type(const BlockType& block_type,
                                         const std::vector<FunctionType>& types) {
        if (std::holds_alternative<std::monostate>(block_type)) {
            // Empty block type
            return FunctionType{{}, {}};
        } else if (std::holds_alternative<ValueType>(block_type)) {
            // Single value type result
            return FunctionType{{}, {std::get<ValueType>(block_type)}};
        } else {
            // Function type index
            uint32_t type_index = std::get<uint32_t>(block_type);
            if (type_index < types.size()) {
                return types[type_index];
            } else {
                // Invalid type index - return empty type
                return FunctionType{{}, {}};
            }
        }
    }

    // =========================================================================
    // Label Type for Control Flow Validation
    // =========================================================================

    /**
     * @brief Label type for control flow validation (Section 3.3.6.4)
     */
    struct LabelType {
        std::vector<ValueType> result_types;  // Expected result types
        uint32_t arity;                      // Number of results
        bool is_loop;                        // Loop vs block/if
        
        LabelType() : arity(0), is_loop(false) {}
        
        LabelType(std::vector<ValueType> results, bool loop = false)
            : result_types(std::move(results)), arity(result_types.size()), is_loop(loop) {}
        
        /**
         * @brief Create label from function type
         */
        static LabelType from_function_type(const FunctionType& func_type, bool is_loop_label = false) {
            if (is_loop_label) {
                // Loop labels expect parameters for backward branches
                return LabelType{func_type.parameters, true};
            } else {
                // Block/If labels expect results for forward branches
                return LabelType{func_type.results, false};
            }
        }
    };

    // =========================================================================
    // Validation Context
    // =========================================================================

    /**
     * @brief Validation context for tracking state during validation
     */
    class ValidationContext {
    public:
        ValidationContext() = default;
        
        // =====================================================================
        // Operand Stack Operations (Section 3.3.2)
        // =====================================================================
        
        /**
         * @brief Pop operand from stack with type checking
         */
        Result<ValueType> pop_operand() noexcept {
            if (unreachable_) {
                // In unreachable code, assume any type can be popped (polymorphism)
                return Result<ValueType>{ValueType::I32};  // Dummy type
            }
            
            if (operand_stack_.empty()) {
                return Result<ValueType>{ErrorCode::StackUnderflow, "Operand stack underflow"};
            }
            
            ValueType type = operand_stack_.back();
            operand_stack_.pop_back();
            return Result<ValueType>{type};
        }
        
        /**
         * @brief Pop operand and check type
         */
        Result<void> pop_operand_expect(ValueType expected) noexcept {
            if (unreachable_) {
                // Accept any type in unreachable code
                return Result<void>{Error{}};
            }
            
            if (operand_stack_.empty()) {
                return Result<void>{ErrorCode::StackUnderflow, 
                    "Expected operand but stack is empty"};
            }
            
            ValueType actual = operand_stack_.back();
            if (actual != expected) {
                return Result<void>{ErrorCode::TypeMismatch,
                    "Type mismatch: expected " + std::string(value_type_name(expected)) +
                    ", got " + std::string(value_type_name(actual))};
            }
            
            operand_stack_.pop_back();
            return Result<void>{Error{}};
        }
        
        /**
         * @brief Push operand to stack
         */
        void push_operand(ValueType type) noexcept {
            if (unreachable_) {
                // Don't modify stack in unreachable code
                return;
            }
            
            if (operand_stack_.size() >= max_operand_stack_size()) {
                // Stack overflow - mark as unreachable
                mark_unreachable();
                return;
            }
            
            operand_stack_.push_back(type);
        }
        
        /**
         * @brief Get current operand stack size
         */
        size_t operand_stack_size() const noexcept { 
            return unreachable_ ? 0 : operand_stack_.size(); 
        }
        
        /**
         * @brief Check if operand stack is empty
         */
        bool operand_stack_empty() const noexcept {
            return unreachable_ || operand_stack_.empty();
        }
        
        // =====================================================================
        // Control Label Operations (Section 3.3.6.4)
        // =====================================================================
        
        /**
         * @brief Push control label
         */
        void push_label(LabelType label) noexcept {
            if (label_stack_.size() >= max_label_stack_size()) {
                mark_unreachable();
                return;
            }
            
            label_stack_.push_back(std::move(label));
        }
        
        /**
         * @brief Pop control label
         */
        Result<LabelType> pop_label() noexcept {
            if (label_stack_.empty()) {
                return {ErrorCode::StackUnderflow,
                    "Control label stack underflow"};
            }
            
            LabelType label = std::move(label_stack_.back());
            label_stack_.pop_back();
            return {std::move(label)};
        }
        
        /**
         * @brief Get label at depth (0 = most recent)
         */
        Result<const LabelType&> get_label(uint32_t depth) const noexcept {
            if (depth >= label_stack_.size()) {
                return {ErrorCode::InvalidBranchTarget,
                    "Invalid label depth: " + std::to_string(depth) + 
                    " (max depth: " + std::to_string(label_stack_.size()) + ")"};
            }
            
            size_t index = label_stack_.size() - 1 - depth;
            return {label_stack_[index]};
        }
        
        /**
         * @brief Get control label stack size
         */
        size_t label_stack_size() const noexcept { return label_stack_.size(); }
        
        // =====================================================================
        // Local Variable Operations
        // =====================================================================
        
        /**
         * @brief Add local variable
         */
        void add_local(ValueType type) noexcept {
            locals_.push_back(type);
        }
        
        /**
         * @brief Set all locals at once
         */
        void set_locals(std::vector<ValueType> locals) noexcept {
            locals_ = std::move(locals);
        }
        
        /**
         * @brief Get local variable type
         */
        Result<ValueType> get_local_type(uint32_t local_index) const noexcept {
            if (local_index >= locals_.size()) {
                return {ErrorCode::InvalidLocalIndex,
                    "Invalid local index: " + std::to_string(local_index) +
                    " (max index: " + std::to_string(locals_.size()) + ")"};
            }
            
            return {locals_[local_index]};
        }
        
        /**
         * @brief Get total number of locals
         */
        size_t local_count() const noexcept { return locals_.size(); }
        
        // =====================================================================
        // Module Context Operations
        // =====================================================================
        
        /**
         * @brief Set function types for the module
         */
        void set_function_types(std::vector<FunctionType> types) noexcept {
            function_types_ = std::move(types);
        }
        
        /**
         * @brief Get function type by index
         */
        Result<const FunctionType&> get_function_type(uint32_t func_index) const noexcept {
            if (func_index >= function_types_.size()) {
                return {ErrorCode::InvalidFunctionIndex,
                    "Invalid function index: " + std::to_string(func_index) +
                    " (max index: " + std::to_string(function_types_.size()) + ")"};
            }
            
            return {function_types_[func_index]};
        }
        
        /**
         * @brief Set global types for the module
         */
        void set_global_types(std::vector<GlobalType> types) noexcept {
            global_types_ = std::move(types);
        }
        
        /**
         * @brief Get global type by index
         */
        Result<const GlobalType&> get_global_type(uint32_t global_index) const noexcept {
            if (global_index >= global_types_.size()) {
                return {ErrorCode::InvalidGlobalIndex,
                    "Invalid global index: " + std::to_string(global_index) +
                    " (max index: " + std::to_string(global_types_.size()) + ")"};
            }
            
            return {global_types_[global_index]};
        }
        
        // =====================================================================
        // Validation State Management
        // =====================================================================
        
        /**
         * @brief Mark code as unreachable (stack polymorphism)
         */
        void mark_unreachable() noexcept { 
            unreachable_ = true; 
            // Clear operand stack in unreachable code
            operand_stack_.clear();
        }
        
        /**
         * @brief Check if current code is unreachable
         */
        bool is_unreachable() const noexcept { return unreachable_; }
        
        /**
         * @brief Reset unreachable state (e.g., at end of block)
         */
        void reset_unreachable() noexcept { unreachable_ = false; }
        
        /**
         * @brief Set current instruction offset for error reporting
         */
        void set_instruction_offset(size_t offset) noexcept { instruction_offset_ = offset; }
        
        /**
         * @brief Get current instruction offset
         */
        size_t get_instruction_offset() const noexcept { return instruction_offset_; }
        
        /**
         * @brief Set current function index for error reporting
         */
        void set_function_index(uint32_t func_index) noexcept { function_index_ = func_index; }
        
        /**
         * @brief Get current function index
         */
        uint32_t get_function_index() const noexcept { return function_index_; }
        
        // =====================================================================
        // Platform-Specific Memory Limits
        // =====================================================================
        
    private:
        std::vector<ValueType> operand_stack_;
        std::vector<LabelType> label_stack_;
        std::vector<ValueType> locals_;
        std::vector<FunctionType> function_types_;
        std::vector<GlobalType> global_types_;
        
        bool unreachable_ = false;
        size_t instruction_offset_ = 0;
        uint32_t function_index_ = UINT32_MAX;
        
        /**
         * @brief Get maximum operand stack size for current platform
         */
        static constexpr size_t max_operand_stack_size() noexcept {
            #ifdef FLIGHT_WASM_EMBEDDED
                return 1024;  // Limit for embedded platforms
            #else
                return 8192;  // Higher limit for desktop platforms
            #endif
        }
        
        /**
         * @brief Get maximum label stack size for current platform
         */
        static constexpr size_t max_label_stack_size() noexcept {
            #ifdef FLIGHT_WASM_EMBEDDED
                return 64;    // Limit for embedded platforms
            #else
                return 512;   // Higher limit for desktop platforms
            #endif
        }
    };

    // =========================================================================
    // Main Validator Interface
    // =========================================================================

    /**
     * @brief Main WebAssembly type validator
     */
    class Validator {
    public:
        /**
         * @brief Validate a complete WebAssembly module
         */
        static Result<void> validate_module(const Module& module) noexcept;
        
        /**
         * @brief Validate a single function
         */
        static Result<void> validate_function(
            const FunctionType& func_type,
            const std::vector<ValueType>& locals,
            const std::vector<uint8_t>& body,
            const std::vector<FunctionType>& module_types,
            const std::vector<GlobalType>& global_types = {}) noexcept;
        
        /**
         * @brief Validate function body with full module context
         */
        static Result<void> validate_function_in_module(
            uint32_t function_index,
            const Module& module) noexcept;
        
    private:
        Validator() = default;
        
        // Module-level validation methods
        static Result<void> validate_types(const std::vector<FunctionType>& types) noexcept;
        static Result<void> validate_imports(const std::vector<Import>& imports,
                                           const std::vector<FunctionType>& types) noexcept;
        static Result<void> validate_functions(const std::vector<uint32_t>& function_indices,
                                             const std::vector<FunctionType>& types) noexcept;
        static Result<void> validate_globals(const std::vector<Global>& globals) noexcept;
        static Result<void> validate_exports(const std::vector<Export>& exports,
                                           const Module& module) noexcept;
        
        // Helper methods for module context setup
        static std::vector<FunctionType> collect_all_function_types(const Module& module) noexcept;
        static std::vector<GlobalType> collect_all_global_types(const Module& module) noexcept;
    };

} // namespace flight::wasm::validation

#endif // FLIGHT_WASM_VALIDATION_VALIDATOR_HPP
