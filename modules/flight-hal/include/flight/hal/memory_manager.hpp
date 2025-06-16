#ifndef FLIGHT_HAL_MEMORY_MANAGER_HPP
#define FLIGHT_HAL_MEMORY_MANAGER_HPP

#include "driver.hpp"
#include "platform.hpp"
#include <cstdint>
#include <cstring>
#include <atomic>

namespace flight
{
    namespace hal
    {

        // Memory pool types for different allocation patterns
        enum class PoolType : uint32_t
        {
            SmallObjects,    // 16-64 bytes (frequent small allocations)
            MediumObjects,   // 128-512 bytes (general purpose)
            LargeObjects,    // 1KB-4KB (buffers, arrays)
            ComponentMemory, // Component heap allocations
            CanonicalMemory, // Type conversions for Component Model
            AssetMemory,     // Textures, sounds, models
            SystemMemory     // System/HAL internal use
        };

        // Memory region info
        struct MemoryRegion
        {
            void *base;
            size_t size;
            size_t alignment;
            bool is_cacheable;
            bool is_executable;
            const char *name;
        };

        // Pool statistics
        struct PoolStats
        {
            size_t total_size;
            size_t used_size;
            size_t free_size;
            size_t block_size;
            uint32_t total_blocks;
            uint32_t used_blocks;
            uint32_t free_blocks;
            uint32_t peak_used_blocks;
            uint64_t allocation_count;
            uint64_t deallocation_count;
        };

        // Platform memory configuration
        struct MemoryConfig
        {
            size_t total_memory;     // Total available memory
            size_t component_budget; // Memory for components
            size_t runtime_budget;   // Memory for runtime
            size_t asset_budget;     // Memory for assets
            size_t system_reserved;  // Reserved for system use

            // Pool configurations
            struct PoolConfig
            {
                size_t size;       // Total pool size
                size_t block_size; // Size of each block
                uint32_t count;    // Number of blocks
            };

            PoolConfig small_pool;
            PoolConfig medium_pool;
            PoolConfig large_pool;
            PoolConfig canonical_pool;
        };

        // Platform-specific memory configurations
        namespace memory_configs
        {
            // Desktop configuration (plenty of memory)
            constexpr MemoryConfig DESKTOP_CONFIG = {
                .total_memory = 1024 * 1024 * 1024, // 1GB
                .component_budget = 512 * 1024 * 1024,
                .runtime_budget = 256 * 1024 * 1024,
                .asset_budget = 256 * 1024 * 1024,
                .system_reserved = 0,
                .small_pool = {4 * 1024 * 1024, 64, 65536},
                .medium_pool = {16 * 1024 * 1024, 512, 32768},
                .large_pool = {64 * 1024 * 1024, 4096, 16384},
                .canonical_pool = {8 * 1024 * 1024, 256, 32768}};

            // PS Vita configuration (512MB)
            constexpr MemoryConfig VITA_CONFIG = {
                .total_memory = 512 * 1024 * 1024,
                .component_budget = 256 * 1024 * 1024,
                .runtime_budget = 128 * 1024 * 1024,
                .asset_budget = 96 * 1024 * 1024,
                .system_reserved = 32 * 1024 * 1024,
                .small_pool = {2 * 1024 * 1024, 64, 32768},
                .medium_pool = {8 * 1024 * 1024, 512, 16384},
                .large_pool = {32 * 1024 * 1024, 4096, 8192},
                .canonical_pool = {4 * 1024 * 1024, 256, 16384}};

            // PSP configuration (32-64MB)
            constexpr MemoryConfig PSP_CONFIG = {
                .total_memory = 32 * 1024 * 1024,
                .component_budget = 12 * 1024 * 1024,
                .runtime_budget = 10 * 1024 * 1024,
                .asset_budget = 8 * 1024 * 1024,
                .system_reserved = 2 * 1024 * 1024,
                .small_pool = {512 * 1024, 64, 8192},
                .medium_pool = {2 * 1024 * 1024, 512, 4096},
                .large_pool = {4 * 1024 * 1024, 4096, 1024},
                .canonical_pool = {1 * 1024 * 1024, 256, 4096}};

            // Dreamcast configuration (16MB)
            constexpr MemoryConfig DREAMCAST_CONFIG = {
                .total_memory = 16 * 1024 * 1024,
                .component_budget = 4 * 1024 * 1024,
                .runtime_budget = 6 * 1024 * 1024,
                .asset_budget = 4 * 1024 * 1024,
                .system_reserved = 2 * 1024 * 1024,
                .small_pool = {256 * 1024, 64, 4096},
                .medium_pool = {1 * 1024 * 1024, 512, 2048},
                .large_pool = {2 * 1024 * 1024, 4096, 512},
                .canonical_pool = {512 * 1024, 256, 2048}};

            // Web/Emscripten configuration (browser dependent)
            constexpr MemoryConfig WEB_CONFIG = {
                .total_memory = 256 * 1024 * 1024, // 256MB default
                .component_budget = 128 * 1024 * 1024,
                .runtime_budget = 64 * 1024 * 1024,
                .asset_budget = 48 * 1024 * 1024,
                .system_reserved = 16 * 1024 * 1024,
                .small_pool = {1 * 1024 * 1024, 64, 16384},
                .medium_pool = {4 * 1024 * 1024, 512, 8192},
                .large_pool = {16 * 1024 * 1024, 4096, 4096},
                .canonical_pool = {2 * 1024 * 1024, 256, 8192}};
        }

        // Memory manager driver interface
        class MemoryManager : public Driver
        {
        public:
            // Driver interface
            DriverType type() const override { return DriverType::Memory; }

            // Initialize memory pools with platform configuration
            virtual Result<void> initialize_pools(const MemoryConfig &config) = 0;

            // Pool allocation (NO malloc/free!)
            virtual void *pool_alloc(PoolType pool, size_t size) = 0;
            virtual void pool_free(PoolType pool, void *ptr) = 0;

            // Aligned allocation from pools
            virtual void *pool_alloc_aligned(PoolType pool, size_t size, size_t alignment) = 0;

            // Get pool statistics
            virtual PoolStats get_pool_stats(PoolType pool) const = 0;

            // Memory regions for components
            virtual Result<MemoryRegion> allocate_component_region(
                uint32_t component_id,
                size_t heap_size,
                size_t stack_size) = 0;

            virtual void free_component_region(uint32_t component_id) = 0;

            // Query memory info
            virtual size_t get_total_memory() const = 0;
            virtual size_t get_used_memory() const = 0;
            virtual size_t get_free_memory() const = 0;

            // Debug/diagnostic functions
            virtual void dump_memory_stats() const = 0;
            virtual bool validate_pools() const = 0;

            // Memory pressure handling
            virtual void on_memory_pressure() = 0;
            virtual void set_oom_handler(std::function<void()> handler) = 0;
        };

        // Simple pool allocator implementation
        class PoolAllocator
        {
        private:
            struct BlockHeader
            {
                uint32_t next_free; // Index of next free block
                uint32_t magic;     // Magic number for corruption detection
            };

            uint8_t *memory_base_;
            size_t pool_size_;
            size_t block_size_;
            uint32_t block_count_;
            uint32_t free_list_;
            std::atomic<uint32_t> used_blocks_;
            uint32_t peak_blocks_;
            std::atomic<uint64_t> alloc_count_;
            std::atomic<uint64_t> free_count_;

            static constexpr uint32_t BLOCK_MAGIC = 0xDEADBEEF;
            static constexpr uint32_t FREE_BLOCK_MAGIC = 0xFEEDFACE;

        public:
            PoolAllocator()
                : memory_base_(nullptr), pool_size_(0), block_size_(0),
                  block_count_(0), free_list_(0), used_blocks_(0),
                  peak_blocks_(0), alloc_count_(0), free_count_(0) {}

            // Initialize pool with pre-allocated memory
            bool initialize(void *memory, size_t pool_size, size_t block_size)
            {
                if (!memory || pool_size == 0 || block_size < sizeof(BlockHeader))
                {
                    return false;
                }

                // Align block size to 8 bytes
                block_size = (block_size + 7) & ~7;

                memory_base_ = static_cast<uint8_t *>(memory);
                pool_size_ = pool_size;
                block_size_ = block_size;
                block_count_ = pool_size / block_size;
                free_list_ = 0;
                used_blocks_ = 0;
                peak_blocks_ = 0;
                alloc_count_ = 0;
                free_count_ = 0;

                // Initialize free list
                for (uint32_t i = 0; i < block_count_; ++i)
                {
                    BlockHeader *header = get_block_header(i);
                    header->next_free = i + 1;
                    header->magic = FREE_BLOCK_MAGIC;
                }

                // Last block points to invalid index
                get_block_header(block_count_ - 1)->next_free = block_count_;

                return true;
            }

            // Allocate a block
            void *allocate()
            {
                if (free_list_ >= block_count_)
                {
                    return nullptr; // Out of memory
                }

                uint32_t block_index = free_list_;
                BlockHeader *header = get_block_header(block_index);

                // Validate magic
                if (header->magic != FREE_BLOCK_MAGIC)
                {
                    return nullptr; // Corruption detected
                }

                // Update free list
                free_list_ = header->next_free;

                // Mark as allocated
                header->magic = BLOCK_MAGIC;

                // Update statistics
                uint32_t used = ++used_blocks_;
                if (used > peak_blocks_)
                {
                    peak_blocks_ = used;
                }
                ++alloc_count_;

                // Return user pointer (after header)
                return memory_base_ + (block_index * block_size_) + sizeof(BlockHeader);
            }

            // Free a block
            void deallocate(void *ptr)
            {
                if (!ptr)
                {
                    return;
                }

                // Calculate block index
                uintptr_t offset = static_cast<uint8_t *>(ptr) - memory_base_ - sizeof(BlockHeader);
                if (offset % block_size_ != 0 || offset >= pool_size_)
                {
                    return; // Invalid pointer
                }

                uint32_t block_index = offset / block_size_;
                BlockHeader *header = get_block_header(block_index);

                // Validate magic
                if (header->magic != BLOCK_MAGIC)
                {
                    return; // Double free or corruption
                }

                // Mark as free
                header->magic = FREE_BLOCK_MAGIC;
                header->next_free = free_list_;
                free_list_ = block_index;

                // Update statistics
                --used_blocks_;
                ++free_count_;
            }

            // Get pool statistics
            PoolStats get_stats() const
            {
                return PoolStats{
                    .total_size = pool_size_,
                    .used_size = used_blocks_ * block_size_,
                    .free_size = (block_count_ - used_blocks_) * block_size_,
                    .block_size = block_size_,
                    .total_blocks = block_count_,
                    .used_blocks = used_blocks_.load(),
                    .free_blocks = block_count_ - used_blocks_,
                    .peak_used_blocks = peak_blocks_,
                    .allocation_count = alloc_count_.load(),
                    .deallocation_count = free_count_.load()};
            }

            // Validate pool integrity
            bool validate() const
            {
                uint32_t free_count = 0;
                uint32_t current = free_list_;

                // Walk free list
                while (current < block_count_ && free_count < block_count_)
                {
                    BlockHeader *header = get_block_header(current);
                    if (header->magic != FREE_BLOCK_MAGIC)
                    {
                        return false;
                    }
                    current = header->next_free;
                    ++free_count;
                }

                return free_count + used_blocks_ == block_count_;
            }

        private:
            BlockHeader *get_block_header(uint32_t index) const
            {
                return reinterpret_cast<BlockHeader *>(memory_base_ + (index * block_size_));
            }
        };

    } // namespace hal
} // namespace flight

#endif // FLIGHT_HAL_MEMORY_MANAGER_HPP
