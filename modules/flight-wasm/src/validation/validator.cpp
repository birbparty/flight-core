/**
 * @file validator.cpp
 * @brief WebAssembly Type Validation Framework Implementation
 */

#include <flight/wasm/validation/validator.hpp>

namespace flight::wasm::validation {

    // =========================================================================
    // Validator Implementation Stubs
    // =========================================================================

    Result<void> Validator::validate_module(const Module& module) noexcept {
        // Basic module validation - will be expanded in future iterations
        return Result<void>{Error{}};
    }

    Result<void> Validator::validate_function(
        const FunctionType& func_type,
        const std::vector<ValueType>& locals,
        const std::vector<uint8_t>& body,
        const std::vector<FunctionType>& module_types,
        const std::vector<GlobalType>& global_types) noexcept {
        // Basic function validation - will be expanded in future iterations
        return Result<void>{Error{}};
    }

    Result<void> Validator::validate_function_in_module(
        uint32_t function_index,
        const Module& module) noexcept {
        // Basic function in module validation - will be expanded in future iterations
        return Result<void>{Error{}};
    }

    // =========================================================================
    // Private Implementation Methods (Stubs)
    // =========================================================================

    Result<void> Validator::validate_types(const std::vector<FunctionType>& types) noexcept {
        // Validate all function types for well-formedness
        for (const auto& type : types) {
            // Check parameter types are valid
            for (const auto& param : type.parameters) {
                if (!is_valid_value_type(param)) {
                    return Result<void>{ErrorCode::TypeMismatch, "Invalid parameter type"};
                }
            }
            
            // Check result types are valid
            for (const auto& result : type.results) {
                if (!is_valid_value_type(result)) {
                    return Result<void>{ErrorCode::TypeMismatch, "Invalid result type"};
                }
            }
        }
        
        return Result<void>{Error{}};
    }

    Result<void> Validator::validate_imports(
        const std::vector<Import>& imports,
        const std::vector<FunctionType>& types) noexcept {
        // Validate all imports for well-formedness
        for (const auto& import : imports) {
            if (import.module_name.empty()) {
                return Result<void>{ErrorCode::ImportResolutionFailed, "Import module name cannot be empty"};
            }
            
            if (import.field_name.empty()) {
                return Result<void>{ErrorCode::ImportResolutionFailed, "Import field name cannot be empty"};
            }
            
            // Validate function imports have valid type indices
            if (import.kind == Import::Kind::Function) {
                if (import.descriptor.function_type_index >= types.size()) {
                    return Result<void>{ErrorCode::InvalidFunctionIndex, 
                        "Import function references invalid type index"};
                }
            }
        }
        
        return Result<void>{Error{}};
    }

    Result<void> Validator::validate_functions(
        const std::vector<uint32_t>& function_indices,
        const std::vector<FunctionType>& types) noexcept {
        // Validate all function type indices are valid
        for (size_t i = 0; i < function_indices.size(); ++i) {
            uint32_t type_index = function_indices[i];
            if (type_index >= types.size()) {
                return Result<void>{ErrorCode::InvalidFunctionIndex, 
                    "Function " + std::to_string(i) + " references invalid type index " + 
                    std::to_string(type_index)};
            }
        }
        
        return Result<void>{Error{}};
    }

    Result<void> Validator::validate_globals(const std::vector<Global>& globals) noexcept {
        // Validate all global definitions
        for (size_t i = 0; i < globals.size(); ++i) {
            const auto& global = globals[i];
            
            // Validate global type
            if (!is_valid_value_type(global.type.value_type)) {
                return Result<void>{ErrorCode::TypeMismatch, 
                    "Global " + std::to_string(i) + " has invalid value type"};
            }
            
            // TODO: Validate initializer expression when instruction parsing is available
        }
        
        return Result<void>{Error{}};
    }

    Result<void> Validator::validate_exports(
        const std::vector<Export>& exports,
        const Module& module) noexcept {
        // Check for duplicate export names
        for (size_t i = 0; i < exports.size(); ++i) {
            for (size_t j = i + 1; j < exports.size(); ++j) {
                if (exports[i].name == exports[j].name) {
                    return Result<void>{ErrorCode::InvalidModule, 
                        "Duplicate export name: " + exports[i].name};
                }
            }
        }
        
        // Validate export indices are within bounds
        for (const auto& export_desc : exports) {
            switch (export_desc.kind) {
                case Export::Kind::Function: {
                    uint32_t total_functions = module.imported_function_count() + 
                                             static_cast<uint32_t>(module.functions.size());
                    if (export_desc.index >= total_functions) {
                        return Result<void>{ErrorCode::InvalidFunctionIndex, 
                            "Export references invalid function index"};
                    }
                    break;
                }
                case Export::Kind::Global: {
                    uint32_t total_globals = module.imported_global_count() + 
                                           static_cast<uint32_t>(module.globals.size());
                    if (export_desc.index >= total_globals) {
                        return Result<void>{ErrorCode::InvalidGlobalIndex, 
                            "Export references invalid global index"};
                    }
                    break;
                }
                case Export::Kind::Table: {
                    uint32_t total_tables = module.imported_table_count() + 
                                          static_cast<uint32_t>(module.tables.size());
                    if (export_desc.index >= total_tables) {
                        return Result<void>{ErrorCode::InvalidTableIndex, 
                            "Export references invalid table index"};
                    }
                    break;
                }
                case Export::Kind::Memory: {
                    uint32_t total_memories = module.imported_memory_count() + 
                                            static_cast<uint32_t>(module.memories.size());
                    if (export_desc.index >= total_memories) {
                        return Result<void>{ErrorCode::InvalidMemoryIndex, 
                            "Export references invalid memory index"};
                    }
                    break;
                }
            }
        }
        
        return Result<void>{Error{}};
    }

    std::vector<FunctionType> Validator::collect_all_function_types(const Module& module) noexcept {
        // Convert module function types to validation function types
        std::vector<FunctionType> validation_types;
        for (const auto& module_type : module.types) {
            validation_types.emplace_back(module_type.params, module_type.results);
        }
        return validation_types;
    }

    std::vector<GlobalType> Validator::collect_all_global_types(const Module& module) noexcept {
        std::vector<GlobalType> global_types;
        
        // Add imported global types
        for (const auto& import : module.imports) {
            if (import.kind == Import::Kind::Global) {
                global_types.push_back(import.descriptor.global_type);
            }
        }
        
        // Add defined global types
        for (const auto& global : module.globals) {
            global_types.push_back(global.type);
        }
        
        return global_types;
    }

} // namespace flight::wasm::validation
