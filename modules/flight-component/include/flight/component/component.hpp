#ifndef FLIGHT_COMPONENT_COMPONENT_HPP
#define FLIGHT_COMPONENT_COMPONENT_HPP

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace flight
{
    namespace component
    {

        // Forward declarations
        class Component;
        class Interface;
        class Resource;
        class CanonicalABI;

        // Component Model types
        enum class TypeKind
        {
            Bool,
            S8,
            S16,
            S32,
            S64,
            U8,
            U16,
            U32,
            U64,
            Float32,
            Float64,
            Char,
            String,
            List,
            Record,
            Variant,
            Tuple,
            Flags,
            Enum,
            Option,
            Result,
            Own,
            Borrow
        };

        // Resource handle
        template <typename T>
        class ResourceHandle
        {
        public:
            explicit ResourceHandle(uint32_t index) : index_(index) {}
            uint32_t index() const { return index_; }

        private:
            uint32_t index_;
        };

        // Interface type definition
        struct InterfaceType
        {
            TypeKind kind;
            std::string name;
            std::vector<InterfaceType> params;
        };

        // This module provides:
        // - Component Model types and operations
        // - Canonical ABI implementation
        // - Interface types
        // - Resource management
        // - Component linking and instantiation

    } // namespace component
} // namespace flight

#endif // FLIGHT_COMPONENT_COMPONENT_HPP
