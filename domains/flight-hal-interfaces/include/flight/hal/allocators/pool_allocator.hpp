/**
 * @file pool_allocator.hpp
 * @brief Pool Allocator Implementation
 * 
 * Zero-fragmentation fixed-size allocator perfect for frequent allocations
 * of the same size. Essential for embedded systems like Dreamcast where
 * fragmentation can cause out-of-memory failures.
 */

#pragma once

#include "../interfaces/memory.hpp"
#include <atomic>
#include <mutex>
#include <vector>

namespace flight::hal::allocators {

/**
 * @brief Pool allocator for fixed-size objects
 * 
 * Manages a pool of fixed-size blocks with O(1) allocation and deallocation.
 * Zero fragmentation as all blocks are the same size. Perfect for objects
 * that are frequently allocated and deallocated.
 */
class PoolAllocator : public IMemoryAllocator {
public:
    /**
     * @brief Constructor
     * @param buffer Pre-allocated buffer to manage
     * @param buffer_size Size of the buffer in bytes
     * @param block_size Size of each block
     * @param alignment Block alignment requirements
     * @param name Human-readable name for this allocator
     */
    PoolAllocator(void* buffer, size_t buffer_size, size_t block_size, 
                  size_t alignment = alignof(std::max_align_t),
                  std::string_view name = "Pool");
    
    /**
     * @brief Constructor with self-managed buffer
     * @param block_count Number of blocks to allocate
     * @param block_size Size of each block
     * @param alignment Block alignment requirements  
     * @param name Human-readable name for this allocator
     */
    PoolAllocator(size_t block_count, size_t block_size,
                  size_t alignment = alignof(std::max_align_t),
                  std::string_view name = "Pool");
    
    /**
     * @brief Destructor
     */
    ~PoolAllocator() override;
    
    // IMemoryAllocator implementation
    HALResult<void*> allocate(size_t size, size_t alignment = alignof(std::max_align_t)) override;
    HALResult<void> deallocate(void* ptr) override;
    HALResult<void*> reallocate(void* ptr, size_t new_size) override;
    AllocatorStats get_stats() const override;
    AllocatorType get_type() const override;
    std::string_view get_name() const override;
    bool supports_size(size_t size) const override;
    bool supports_alignment(size_t alignment) const override;
    bool owns_pointer(void* ptr) const override;
    HALResult<void> reset() override;
    HALResult<void> defragment(DefragmentationCallback callback = nullptr) override;
    
    // Pool allocator specific methods
    
    /**
     * @brief Get block size for this pool
     * @return Size of each block in bytes
     */
    size_t get_block_size() const { return block_size_; }
    
    /**
     * @brief Get total number of blocks
     * @return Total number of blocks in the pool
     */
    size_t get_block_count() const { return block_count_; }
    
    /**
     * @brief Get number of free blocks
     * @return Number of available blocks
     */
    size_t get_free_blocks() const;
    
    /**
     * @brief Get number of used blocks
     * @return Number of allocated blocks
     */
    size_t get_used_blocks() const;
    
    /**
     * @brief Check if pool is full
     * @return true if no free blocks available
     */
    bool is_full() const;
    
    /**
     * @brief Check if pool is empty
     * @return true if all blocks are free
     */
    bool is_empty() const;
    
    /**
     * @brief Check if buffer is owned by this allocator
     * @return true if buffer is self-managed
     */
    bool owns_buffer() const { return owns_buffer_; }

private:
    void* buffer_;              ///< Memory buffer
    size_t buffer_size_;        ///< Total buffer size
    size_t block_size_;         ///< Size of each block
    size_t block_count_;        ///< Total number of blocks
    size_t alignment_;          ///< Block alignment
    bool owns_buffer_;          ///< Whether we own the buffer
    std::string name_;          ///< Allocator name
    
    // Free list management
    void* free_head_;           ///< Head of free list
    std::atomic<size_t> free_count_; ///< Number of free blocks
    
    // Statistics (protected by mutex for thread safety)
    mutable std::mutex stats_mutex_;
    size_t total_allocations_;
    size_t peak_usage_;
    
    // Helper methods
    void initialize_free_list();
    bool is_valid_block_pointer(void* ptr) const;
    size_t get_block_index(void* ptr) const;
    void* get_block_at_index(size_t index) const;
    void update_stats();
};

/**
 * @brief Thread-safe pool allocator
 * 
 * Version of PoolAllocator with full thread safety.
 * Uses lock-free operations where possible for maximum performance.
 */
class ThreadSafePoolAllocator : public PoolAllocator {
public:
    using PoolAllocator::PoolAllocator;
    
    // Thread-safe overrides
    HALResult<void*> allocate(size_t size, size_t alignment = alignof(std::max_align_t)) override;
    HALResult<void> deallocate(void* ptr) override;
    HALResult<void> reset() override;
    
    // Thread-safe pool-specific methods
    size_t get_free_blocks() const;
    size_t get_used_blocks() const;
    bool is_full() const;
    bool is_empty() const;
    
private:
    mutable std::mutex allocation_mutex_;
    std::atomic<void*> atomic_free_head_{nullptr};
    
    // Lock-free operations
    void* pop_free_block();
    void push_free_block(void* block);
};

/**
 * @brief Multi-size pool allocator
 * 
 * Manages multiple pool allocators for different sizes,
 * providing efficient allocation for various object sizes
 * while maintaining zero fragmentation.
 */
class MultiSizePoolAllocator : public IMemoryAllocator {
public:
    /**
     * @brief Size configuration for pool
     */
    struct PoolConfig {
        size_t block_size;      ///< Size of blocks in this pool
        size_t block_count;     ///< Number of blocks to allocate
        size_t alignment;       ///< Block alignment
    };
    
    /**
     * @brief Constructor
     * @param configs Pool configurations
     * @param name Human-readable name for this allocator
     */
    MultiSizePoolAllocator(const std::vector<PoolConfig>& configs,
                          std::string_view name = "MultiPool");
    
    /**
     * @brief Destructor
     */
    ~MultiSizePoolAllocator() override;
    
    // IMemoryAllocator implementation
    HALResult<void*> allocate(size_t size, size_t alignment = alignof(std::max_align_t)) override;
    HALResult<void> deallocate(void* ptr) override;
    HALResult<void*> reallocate(void* ptr, size_t new_size) override;
    AllocatorStats get_stats() const override;
    AllocatorType get_type() const override;
    std::string_view get_name() const override;
    bool supports_size(size_t size) const override;
    bool supports_alignment(size_t alignment) const override;
    bool owns_pointer(void* ptr) const override;
    HALResult<void> reset() override;
    HALResult<void> defragment(DefragmentationCallback callback = nullptr) override;
    
    // Multi-pool specific methods
    
    /**
     * @brief Get number of pools
     * @return Number of managed pools
     */
    size_t get_pool_count() const { return pools_.size(); }
    
    /**
     * @brief Get pool for specific size
     * @param size Size to find pool for
     * @return Pointer to pool allocator or nullptr if not found
     */
    PoolAllocator* get_pool_for_size(size_t size) const;
    
    /**
     * @brief Get all pool configurations
     * @return Vector of pool configurations
     */
    std::vector<PoolConfig> get_pool_configs() const;

private:
    std::vector<std::unique_ptr<ThreadSafePoolAllocator>> pools_;
    std::vector<PoolConfig> configs_;
    std::string name_;
    
    // Helper methods
    size_t find_best_fit_pool(size_t size, size_t alignment) const;
    void initialize_pools();
};

/**
 * @brief Object pool template
 * 
 * Template wrapper around PoolAllocator for type-safe object allocation.
 * Automatically handles construction and destruction.
 */
template<typename T>
class ObjectPool {
public:
    /**
     * @brief Constructor
     * @param capacity Number of objects to pool
     */
    explicit ObjectPool(size_t capacity) 
        : pool_(capacity, sizeof(T), alignof(T), "ObjectPool<T>") {}
    
    /**
     * @brief Acquire an object from the pool
     * @param args Constructor arguments
     * @return Pointer to constructed object or nullptr if pool is full
     */
    template<typename... Args>
    T* acquire(Args&&... args) {
        auto result = pool_.allocate(sizeof(T), alignof(T));
        if (!result) {
            return nullptr;
        }
        
        return new(result.value()) T(std::forward<Args>(args)...);
    }
    
    /**
     * @brief Return an object to the pool
     * @param obj Object to return (must have been acquired from this pool)
     */
    void release(T* obj) {
        if (obj && pool_.owns_pointer(obj)) {
            obj->~T();
            pool_.deallocate(obj);
        }
    }
    
    /**
     * @brief Get pool statistics
     * @return Allocator statistics
     */
    AllocatorStats get_stats() const {
        return pool_.get_stats();
    }
    
    /**
     * @brief Check if pool is full
     * @return true if no objects available
     */
    bool is_full() const {
        return pool_.is_full();
    }
    
    /**
     * @brief Check if pool is empty
     * @return true if all objects are available
     */
    bool is_empty() const {
        return pool_.is_empty();
    }

private:
    ThreadSafePoolAllocator pool_;
};

} // namespace flight::hal::allocators
