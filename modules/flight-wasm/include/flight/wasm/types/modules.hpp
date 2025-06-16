#ifndef FLIGHT_WASM_TYPES_MODULES_HPP
#define FLIGHT_WASM_TYPES_MODULES_HPP

/**
 * @file modules.hpp
 * @brief WebAssembly module structure definitions
 * 
 * This header provides forward declarations and basic structure for WebAssembly
 * modules and their components. Full implementation will be completed in the
 * module parsing tasks.
 */

#include <cstdint>
#include <vector>
#include <string>

namespace flight::wasm {

    // Forward declarations from other headers
    enum class ValueType : uint8_t;
    class Value;
    class Instruction;

    /**
     * @brief WebAssembly module section IDs
     * 
     * These correspond to the section IDs in the WebAssembly binary format.
     */
    enum class SectionId : uint8_t {
        Custom = 0,
        Type = 1,
        Import = 2,
        Function = 3,
        Table = 4,
        Memory = 5,
        Global = 6,
        Export = 7,
        Start = 8,
        Element = 9,
        Code = 10,
        Data = 11,
        DataCount = 12  // WebAssembly 2.0
    };

    /**
     * @brief Check if a section ID is valid
     */
    constexpr bool is_valid_section_id(uint8_t id) noexcept {
        return id <= static_cast<uint8_t>(SectionId::DataCount);
    }

    /**
     * @brief WebAssembly limits structure
     * 
     * Used for table and memory types to specify minimum and optional maximum sizes.
     */
    struct Limits {
        uint32_t min;
        uint32_t max;
        bool has_max;

        constexpr Limits() noexcept : min(0), max(0), has_max(false) {}
        constexpr Limits(uint32_t minimum) noexcept : min(minimum), max(0), has_max(false) {}
        constexpr Limits(uint32_t minimum, uint32_t maximum) noexcept 
            : min(minimum), max(maximum), has_max(true) {}
    };

    /**
     * @brief WebAssembly function type
     * 
     * Represents a function signature with parameter and result types.
     */
    struct FunctionType {
        std::vector<ValueType> params;
        std::vector<ValueType> results;

        FunctionType() = default;
        FunctionType(std::vector<ValueType> p, std::vector<ValueType> r)
            : params(std::move(p)), results(std::move(r)) {}
    };

    /**
     * @brief WebAssembly table type
     */
    struct TableType {
        ValueType element_type;  // funcref or externref
        Limits limits;

        constexpr TableType() noexcept : element_type{}, limits{} {}
        constexpr TableType(ValueType et, Limits l) noexcept 
            : element_type(et), limits(l) {}
    };

    /**
     * @brief WebAssembly memory type
     */
    struct MemoryType {
        Limits limits;

        constexpr MemoryType() noexcept : limits{} {}
        constexpr MemoryType(Limits l) noexcept : limits(l) {}
    };

    /**
     * @brief WebAssembly global type
     */
    struct GlobalType {
        ValueType value_type;
        bool is_mutable;

        constexpr GlobalType() noexcept : value_type{}, is_mutable(false) {}
        constexpr GlobalType(ValueType vt, bool mut) noexcept 
            : value_type(vt), is_mutable(mut) {}
    };

    /**
     * @brief WebAssembly import descriptor
     */
    struct Import {
        enum class Kind : uint8_t {
            Function = 0,
            Table = 1,
            Memory = 2,
            Global = 3
        };

        std::string module_name;
        std::string field_name;
        Kind kind;
        
        union Descriptor {
            uint32_t function_type_index;  // For function imports
            TableType table_type;          // For table imports
            MemoryType memory_type;        // For memory imports
            GlobalType global_type;        // For global imports

            constexpr Descriptor() : function_type_index(0) {}
            constexpr Descriptor(uint32_t idx) : function_type_index(idx) {}
            constexpr Descriptor(TableType tt) : table_type(tt) {}
            constexpr Descriptor(MemoryType mt) : memory_type(mt) {}
            constexpr Descriptor(GlobalType gt) : global_type(gt) {}
        } descriptor;

        Import() = default;
        Import(std::string mod, std::string field, Kind k, Descriptor desc)
            : module_name(std::move(mod)), field_name(std::move(field))
            , kind(k), descriptor(desc) {}
    };

    /**
     * @brief WebAssembly export descriptor
     */
    struct Export {
        enum class Kind : uint8_t {
            Function = 0,
            Table = 1,
            Memory = 2,
            Global = 3
        };

        std::string name;
        Kind kind;
        uint32_t index;

        Export() = default;
        Export(std::string n, Kind k, uint32_t idx)
            : name(std::move(n)), kind(k), index(idx) {}
    };

    /**
     * @brief WebAssembly global definition
     */
    struct Global {
        GlobalType type;
        std::vector<uint8_t> initializer_bytes;  // Constant expression as raw bytes

        Global() = default;
        Global(GlobalType t, std::vector<uint8_t> init)
            : type(t), initializer_bytes(std::move(init)) {}
    };

    /**
     * @brief WebAssembly function definition
     */
    struct Function {
        uint32_t type_index;
        std::vector<ValueType> locals;
        std::vector<uint8_t> body_bytes;  // Function body as raw bytes

        Function() = default;
        Function(uint32_t type_idx, std::vector<ValueType> loc, std::vector<uint8_t> b)
            : type_index(type_idx), locals(std::move(loc)), body_bytes(std::move(b)) {}
    };

    /**
     * @brief WebAssembly element segment
     */
    struct Element {
        enum class Mode : uint8_t {
            Active = 0,    // Active with table index
            Passive = 1,   // Passive
            Declarative = 2 // Declarative
        };

        Mode mode;
        uint32_t table_index;  // For active mode
        std::vector<uint8_t> offset_bytes;  // Constant expression as raw bytes
        ValueType element_type;
        std::vector<uint32_t> function_indices;  // For function references

        Element() = default;
    };

    /**
     * @brief WebAssembly data segment
     */
    struct Data {
        enum class Mode : uint8_t {
            Active = 0,  // Active with memory index
            Passive = 1  // Passive
        };

        Mode mode;
        uint32_t memory_index;  // For active mode
        std::vector<uint8_t> offset_bytes;  // Constant expression as raw bytes
        std::vector<uint8_t> data;

        Data() = default;
    };

    /**
     * @brief WebAssembly module representation
     * 
     * This is the main structure representing a complete WebAssembly module.
     * It contains all sections and provides the interface for module operations.
     */
    class Module {
    public:
        // Module sections
        std::vector<FunctionType> types;
        std::vector<Import> imports;
        std::vector<uint32_t> function_type_indices;
        std::vector<TableType> tables;
        std::vector<MemoryType> memories;
        std::vector<Global> globals;
        std::vector<Export> exports;
        std::vector<Function> functions;
        std::vector<Element> elements;
        std::vector<Data> data;

        // Optional start function
        uint32_t start_function_index = UINT32_MAX;
        bool has_start_function = false;

        // Custom sections (name, data pairs)
        std::vector<std::pair<std::string, std::vector<uint8_t>>> custom_sections;

        Module() = default;

        /**
         * @brief Check if the module is valid
         * 
         * This will perform basic structural validation.
         * Full validation will be implemented in validation tasks.
         */
        bool is_valid() const noexcept;

        /**
         * @brief Get the total number of imported functions
         */
        uint32_t imported_function_count() const noexcept;

        /**
         * @brief Get the total number of imported tables
         */
        uint32_t imported_table_count() const noexcept;

        /**
         * @brief Get the total number of imported memories
         */
        uint32_t imported_memory_count() const noexcept;

        /**
         * @brief Get the total number of imported globals
         */
        uint32_t imported_global_count() const noexcept;

        /**
         * @brief Get the total number of functions (imported + defined)
         */
        uint32_t total_function_count() const noexcept;

        /**
         * @brief Get the total number of tables (imported + defined)
         */
        uint32_t total_table_count() const noexcept;

        /**
         * @brief Get the total number of memories (imported + defined)
         */
        uint32_t total_memory_count() const noexcept;

        /**
         * @brief Get the total number of globals (imported + defined)
         */
        uint32_t total_global_count() const noexcept;
    };

    /**
     * @brief WebAssembly module builder
     * 
     * Provides a convenient interface for constructing modules programmatically.
     */
    class ModuleBuilder {
    public:
        ModuleBuilder() = default;

        // Add various module components
        ModuleBuilder& add_type(FunctionType type);
        ModuleBuilder& add_import(Import import);
        ModuleBuilder& add_function(uint32_t type_index);
        ModuleBuilder& add_table(TableType type);
        ModuleBuilder& add_memory(MemoryType type);
        ModuleBuilder& add_global(Global global);
        ModuleBuilder& add_export(Export export_desc);
        ModuleBuilder& add_element(Element element);
        ModuleBuilder& add_data(Data data);
        ModuleBuilder& set_start_function(uint32_t function_index);

        // Build the final module
        Module build() &&;

    private:
        Module module_;
    };

} // namespace flight::wasm

#endif // FLIGHT_WASM_TYPES_MODULES_HPP
