/**
 * @file linear_allocator.hpp
 * @brief Linear/Arena Allocator Implementation
 * 
 * High-performance linear allocator providing O(1) allocation and bulk deallocation.
 * Perfect for temporary allocations and frame-based memory management.
 * Critical for Dreamcast's 16MB constraints.
 */

#pragma once

#include "../interfaces/memory.hpp"
#include <atomic>
#include <mutex>

namespace flight::hal::allocators {

/**
 * @brief Linear/Arena allocator implementation
 * 
 * Allocates memory sequentially from a pre-allocated buffer.
 * Provides extremely fast O(1) allocation but no individual deallocation.
 * Must be reset in bulk. Perfect for frame-based allocations.
 */
class LinearAllocator : public IMemoryAllocator {
public:
    /**
     * @brief Constructor
     * @param buffer Pre-allocated buffer to manage
     * @param size Size of the buffer in bytes
     * @param name Human-readable name for this allocator
     */
    LinearAllocator(void* buffer, size_t size, std::string_view name = "Linear");
    
    /**
     * @brief Constructor with self-managed buffer
     * @param size Size of buffer to allocate
     * @param name Human-readable name for this allocator
     */
    explicit LinearAllocator(size_t size, std::string_view name = "Linear");
    
    /**
     * @brief Destructor
     */
    ~LinearAllocator() override;
    
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
    
    // Linear allocator specific methods
    
    /**
     * @brief Get current position in the buffer
     * @return Current allocation offset
     */
    size_t get_position() const;
    
    /**
     * @brief Set position marker (for checkpoint/restore functionality)
     * @param position Position to set
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_position(size_t position);
    
    /**
     * @brief Create a checkpoint for later restoration
     * @return Current position that can be used with restore_checkpoint
     */
    size_t create_checkpoint() const;
    
    /**
     * @brief Restore to a previous checkpoint
     * @param checkpoint Checkpoint returned from create_checkpoint
     * @return HALResult indicating success or failure
     */
    HALResult<void> restore_checkpoint(size_t checkpoint);
    
    /**
     * @brief Get remaining free space
     * @return Number of bytes remaining
     */
    size_t get_remaining_space() const;
    
    /**
     * @brief Check if buffer is owned by this allocator
     * @return true if buffer is self-managed
     */
    bool owns_buffer() const { return owns_buffer_; }

private:
    void* buffer_;                  ///< Memory buffer
    size_t buffer_size_;           ///< Total buffer size
    std::atomic<size_t> position_; ///< Current allocation position
    bool owns_buffer_;             ///< Whether we own the buffer
    std::string name_;             ///< Allocator name
    
    // Statistics (protected by mutex for thread safety)
    mutable std::mutex stats_mutex_;
    size_t allocation_count_;
    size_t peak_usage_;
    size_t total_allocations_;
    
    // Helper methods
    bool is_aligned(size_t value, size_t alignment) const;
    size_t align_up(size_t value, size_t alignment) const;
    void update_stats(size_t allocated_size);
};

/**
 * @brief Thread-safe linear allocator
 * 
 * Version of LinearAllocator with full thread safety.
 * Slightly slower but safe for multi-threaded use.
 */
class ThreadSafeLinearAllocator : public LinearAllocator {
public:
    using LinearAllocator::LinearAllocator;
    
    // Thread-safe overrides
    HALResult<void*> allocate(size_t size, size_t alignment = alignof(std::max_align_t)) override;
    HALResult<void> reset() override;
    HALResult<void> set_position(size_t position) override;
    
private:
    mutable std::mutex allocation_mutex_;
};

/**
 * @brief Stack-based linear allocator
 * 
 * Linear allocator that manages multiple stack frames,
 * allowing for nested allocation scopes with automatic cleanup.
 */
class StackLinearAllocator : public LinearAllocator {
public:
    using LinearAllocator::LinearAllocator;
    
    /**
     * @brief Push a new allocation frame
     * @return Frame ID for later pop operation
     */
    size_t push_frame();
    
    /**
     * @brief Pop the most recent allocation frame
     * @return HALResult indicating success or failure
     */
    HALResult<void> pop_frame();
    
    /**
     * @brief Pop to a specific frame
     * @param frame_id Frame ID returned from push_frame
     * @return HALResult indicating success or failure
     */
    HALResult<void> pop_to_frame(size_t frame_id);
    
    /**
     * @brief Get current frame depth
     * @return Number of active frames
     */
    size_t get_frame_depth() const;

private:
    std::vector<size_t> frame_stack_;
    mutable std::mutex frame_mutex_;
};

} // namespace flight::hal::allocators
