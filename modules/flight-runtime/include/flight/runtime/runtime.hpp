#ifndef FLIGHT_RUNTIME_RUNTIME_HPP
#define FLIGHT_RUNTIME_RUNTIME_HPP

#include <memory>
#include <vector>

namespace flight
{
    namespace runtime
    {

        // Forward declarations
        class Interpreter;
        class ExecutionContext;
        class Module;
        class Instance;
        class Stack;

        // Trap types for runtime errors
        enum class TrapKind
        {
            Unreachable,
            MemoryOutOfBounds,
            CallStackExhausted,
            IntegerOverflow,
            IntegerDivisionByZero,
            InvalidConversionToInteger,
            IndirectCallTypeMismatch,
            UndefinedElement,
            UninitializedElement
        };

        // Execution result
        template <typename T>
        class Result
        {
        public:
            bool is_ok() const { return ok_; }
            bool is_err() const { return !ok_; }
            T &value() { return value_; }
            const T &value() const { return value_; }
            TrapKind error() const { return error_; }

        private:
            bool ok_;
            T value_;
            TrapKind error_;
        };

        // This module provides:
        // - Interpreter implementation
        // - Execution engine and context
        // - Module validation
        // - Execution stack management
        // - Function dispatch
        // - Trap handling

    } // namespace runtime
} // namespace flight

#endif // FLIGHT_RUNTIME_RUNTIME_HPP
