#ifndef FLIGHT_WASM_TYPES_INSTRUCTIONS_HPP
#define FLIGHT_WASM_TYPES_INSTRUCTIONS_HPP

/**
 * @file instructions.hpp
 * @brief WebAssembly instruction definitions and representation
 * 
 * This header provides forward declarations and basic structure for WebAssembly
 * instructions. Complete implementation will be done in later tasks focused on
 * instruction definitions and opcode handling.
 */

#include <cstdint>
#include <flight/wasm/utilities/platform.hpp>

namespace flight::wasm {

    /**
     * @brief WebAssembly instruction opcodes
     * 
     * These opcodes correspond to the WebAssembly Core Specification.
     * This is a placeholder - full opcode definitions will be implemented
     * in the webassembly-opcodes-definition task.
     */
    enum class Opcode : uint8_t {
        // Control flow instructions
        Unreachable = 0x00,
        Nop = 0x01,
        Block = 0x02,
        Loop = 0x03,
        If = 0x04,
        Else = 0x05,
        End = 0x0B,
        Br = 0x0C,
        BrIf = 0x0D,
        BrTable = 0x0E,
        Return = 0x0F,
        Call = 0x10,
        CallIndirect = 0x11,

        // Parametric instructions
        Drop = 0x1A,
        Select = 0x1B,
        SelectWithType = 0x1C,

        // Variable instructions
        LocalGet = 0x20,
        LocalSet = 0x21,
        LocalTee = 0x22,
        GlobalGet = 0x23,
        GlobalSet = 0x24,

        // Table instructions
        TableGet = 0x25,
        TableSet = 0x26,

        // Memory instructions
        I32Load = 0x28,
        I64Load = 0x29,
        F32Load = 0x2A,
        F64Load = 0x2B,
        I32Load8S = 0x2C,
        I32Load8U = 0x2D,
        I32Load16S = 0x2E,
        I32Load16U = 0x2F,
        I64Load8S = 0x30,
        I64Load8U = 0x31,
        I64Load16S = 0x32,
        I64Load16U = 0x33,
        I64Load32S = 0x34,
        I64Load32U = 0x35,
        I32Store = 0x36,
        I64Store = 0x37,
        F32Store = 0x38,
        F64Store = 0x39,
        I32Store8 = 0x3A,
        I32Store16 = 0x3B,
        I64Store8 = 0x3C,
        I64Store16 = 0x3D,
        I64Store32 = 0x3E,
        MemorySize = 0x3F,
        MemoryGrow = 0x40,

        // Numeric instructions (constants)
        I32Const = 0x41,
        I64Const = 0x42,
        F32Const = 0x43,
        F64Const = 0x44,

        // Additional opcodes will be defined in the opcode definition task
        // This is just a representative sample

        // Extended opcodes (0xFC prefix)
        ExtendedOpcode = 0xFC,
        
        // SIMD opcodes (0xFD prefix)  
        SimdOpcode = 0xFD
    };

    /**
     * @brief Check if an opcode is a control flow instruction
     */
    constexpr bool is_control_instruction(Opcode opcode) noexcept {
        return static_cast<uint8_t>(opcode) <= 0x11;
    }

    /**
     * @brief Check if an opcode is a parametric instruction
     */
    constexpr bool is_parametric_instruction(Opcode opcode) noexcept {
        return static_cast<uint8_t>(opcode) >= 0x1A && static_cast<uint8_t>(opcode) <= 0x1C;
    }

    /**
     * @brief Check if an opcode is a variable instruction
     */
    constexpr bool is_variable_instruction(Opcode opcode) noexcept {
        return static_cast<uint8_t>(opcode) >= 0x20 && static_cast<uint8_t>(opcode) <= 0x24;
    }

    /**
     * @brief Check if an opcode is a memory instruction
     */
    constexpr bool is_memory_instruction(Opcode opcode) noexcept {
        return static_cast<uint8_t>(opcode) >= 0x28 && static_cast<uint8_t>(opcode) <= 0x40;
    }

    /**
     * @brief Check if an opcode is a numeric constant instruction
     */
    constexpr bool is_const_instruction(Opcode opcode) noexcept {
        return static_cast<uint8_t>(opcode) >= 0x41 && static_cast<uint8_t>(opcode) <= 0x44;
    }

    // Forward declarations for instruction-related types
    class Instruction;
    class ControlInstruction;
    class ParametricInstruction;
    class VariableInstruction;
    class MemoryInstruction;
    class NumericInstruction;
    class VectorInstruction;

    /**
     * @brief Immediate value types for instructions
     * 
     * Different instructions have different immediate value requirements.
     * This will be fully implemented in the immediate-value-system task.
     */
    struct Immediate {
        enum class Type : uint8_t {
            None,
            U32,        // Unsigned 32-bit integer
            U64,        // Unsigned 64-bit integer
            I32,        // Signed 32-bit integer
            I64,        // Signed 64-bit integer
            F32,        // 32-bit float
            F64,        // 64-bit float
            BlockType,  // Block signature
            MemArg,     // Memory argument (align, offset)
            BrTable,    // Branch table
            FuncType,   // Function type index
            TypeIdx,    // Type index
            FuncIdx,    // Function index
            TableIdx,   // Table index
            MemIdx,     // Memory index
            GlobalIdx,  // Global index
            LocalIdx,   // Local index
            LabelIdx    // Label index
        };

        Type type = Type::None;
        
        union Value {
            uint32_t u32;
            uint64_t u64;
            int32_t i32;
            int64_t i64;
            float f32;
            double f64;
            
            constexpr Value() : u64(0) {}
            constexpr Value(uint32_t v) : u32(v) {}
            constexpr Value(uint64_t v) : u64(v) {}
            constexpr Value(int32_t v) : i32(v) {}
            constexpr Value(int64_t v) : i64(v) {}
            constexpr Value(float v) : f32(v) {}
            constexpr Value(double v) : f64(v) {}
        } value;

        constexpr Immediate() noexcept = default;
        constexpr Immediate(Type t, Value v) noexcept : type(t), value(v) {}
    };

    /**
     * @brief Memory argument for memory instructions
     */
    struct MemArg {
        uint32_t align;  // Alignment hint (power of 2)
        uint32_t offset; // Memory offset

        constexpr MemArg() noexcept : align(0), offset(0) {}
        constexpr MemArg(uint32_t a, uint32_t o) noexcept : align(a), offset(o) {}
    };

    /**
     * @brief Block type for control instructions
     */
    struct BlockType {
        enum class Kind : uint8_t {
            Empty,     // No result type
            ValueType, // Single value type result
            TypeIndex  // Function type index
        };

        Kind kind = Kind::Empty;
        
        union {
            ValueType value_type;
            uint32_t type_index;
        };

        constexpr BlockType() noexcept : kind(Kind::Empty), type_index(0) {}
        constexpr BlockType(ValueType vt) noexcept : kind(Kind::ValueType), value_type(vt) {}
        constexpr BlockType(uint32_t idx) noexcept : kind(Kind::TypeIndex), type_index(idx) {}
    };

} // namespace flight::wasm

#endif // FLIGHT_WASM_TYPES_INSTRUCTIONS_HPP
