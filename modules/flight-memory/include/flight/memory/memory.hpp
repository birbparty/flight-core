#ifndef FLIGHT_MEMORY_MEMORY_HPP
#define FLIGHT_MEMORY_MEMORY_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <atomic>

namespace flight
{
    namespace memory
    {

        // Forward declarations
        class Allocator;
        class MemoryPool;
        class LinearAllocator;
        class StackAllocator;

        // Memory statistics
        struct MemoryStats
        {
            std::atomic<size_t> total_allocated{0};
            std::atomic<size_t> total_freed{0};
            std::atomic<size_t> current_usage{0};
            std::atomic<size_t> peak_usage{0};
            std::atomic<size_t> allocation_count{0};
            std::atomic<size_t> deallocation_count{0};
        };

        // Base allocator interface
        class Allocator
        {
        public:
            virtual ~Allocator() = default;

            virtual void *allocate(size_t size, size_t alignment = alignof(std::max_align_t)) = 0;
            virtual void deallocate(void *ptr, size_t size) = 0;
            virtual void reset() = 0;

            virtual size_t get_used_memory() const = 0;
            virtual size_t get_total_memory() const = 0;
        };

        // Memory allocation strategy
        enum class AllocationStrategy
        {
            Linear, // Fast bump allocator, no individual deallocation
            Stack,  // LIFO allocation pattern
            Pool,   // Fixed-size block allocator
            System  // Direct system allocation
        };

        // This module provides:
        // - Platform-independent memory interface
        // - Memory pools and allocators
        // - Linear allocator for fast allocation
        // - Stack allocator for LIFO patterns
        // - Statistics tracking
        // - Memory alignment utilities

    } // namespace memory
} // namespace flight

#endif // FLIGHT_MEMORY_MEMORY_HPP
