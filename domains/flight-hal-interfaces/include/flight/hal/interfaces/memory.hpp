/**
 * @file memory.hpp
 * @brief Flight HAL Elite Memory Management Interface
 * 
 * Comprehensive memory management system supporting multiple allocator types,
 * specialized memory regions, pressure management, and zero-fragmentation
 * strategies for platforms ranging from Dreamcast (16MB) to modern gigabyte systems.
 */

#pragma once

#include "../core/driver_registry.hpp"
#include "../core/hal_result.hpp"
#include "../core/hal_capabilities.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>
#include <string_view>
#include <memory>

namespace flight::hal {

/**
 * @brief Memory alignment requirements
 */
enum class MemoryAlignment : uint32_t {
    None = 1,
    Word = 4,
    Cache = 32,
    Page = 4096,
    SIMD = 16,      ///< 128-bit SIMD alignment
    GPU = 256       ///< GPU memory alignment
};

/**
 * @brief Memory allocation flags
 */
enum class MemoryFlags : uint32_t {
    None = 0,
    Cacheable = 1 << 0,      ///< Memory should be cacheable
    Executable = 1 << 1,     ///< Memory should be executable
    Persistent = 1 << 2,     ///< Memory persists across sessions
    DMACapable = 1 << 3,     ///< Memory accessible by DMA
    Shared = 1 << 4,         ///< Shared between processes/threads
    Temporary = 1 << 5,      ///< Short-lived allocation hint
    Critical = 1 << 6,       ///< Critical allocation, never swap/page
    Zero = 1 << 7           ///< Zero-initialize memory
};

/**
 * @brief Memory type classification for specialized allocation
 */
enum class MemoryType : uint8_t {
    System = 0,      ///< General purpose system memory
    Video,           ///< GPU-accessible video memory
    Audio,           ///< Audio buffer memory (DMA capable)
    Network,         ///< Network packet buffers (aligned)
    Shared,          ///< Inter-process shared memory
    Temporary,       ///< Temporary/scratch memory
    Stack,           ///< Stack-based allocations
    Pool             ///< Fixed-size pool allocations
};

/**
 * @brief Allocator strategy types
 */
enum class AllocatorType : uint8_t {
    Linear = 0,      ///< Linear/Arena allocator (O(1) alloc, bulk free)
    Stack,          ///< Stack allocator (LIFO, O(1) operations)
    Pool,           ///< Pool allocator (fixed-size, zero fragmentation)
    Buddy,          ///< Buddy allocator (power-of-2, low fragmentation)
    FreeList,       ///< Free list allocator (general purpose)
    System          ///< System default allocator
};

/**
 * @brief Memory pressure levels for resource management
 */
enum class MemoryPressureLevel : uint8_t {
    None = 0,        ///< Abundant memory available
    Low,            ///< Starting to run low on memory
    Medium,         ///< Significant memory pressure
    High,           ///< Critical memory shortage
    Critical        ///< Emergency - system may crash
};

/**
 * @brief Memory allocation result
 * 
 * Contains both the allocated pointer and metadata about the allocation.
 */
struct MemoryAllocation {
    void* ptr;                  ///< Allocated memory pointer
    size_t size;               ///< Actual allocated size (may be larger than requested)
    MemoryAlignment alignment; ///< Actual alignment used
    MemoryFlags flags;         ///< Allocation flags applied
    MemoryType type;           ///< Memory type classification
    AllocatorType allocator;   ///< Allocator used for this allocation
};

/**
 * @brief Memory statistics structure
 * 
 * Provides detailed information about memory usage and availability.
 */
struct MemoryStats {
    size_t total_bytes;        ///< Total system memory
    size_t used_bytes;         ///< Currently allocated memory
    size_t free_bytes;         ///< Available memory for allocation
    size_t peak_used_bytes;    ///< Peak memory usage since initialization
    size_t allocation_count;   ///< Number of active allocations
    size_t fragmentation_pct;  ///< Fragmentation percentage (0-100)
    size_t largest_free_block; ///< Size of largest contiguous free block
    double allocation_efficiency; ///< Allocation efficiency (0.0-1.0)
};

/**
 * @brief Allocator-specific statistics
 */
struct AllocatorStats {
    AllocatorType type;           ///< Allocator type
    std::string_view name;        ///< Human-readable allocator name
    size_t total_capacity;        ///< Total allocator capacity
    size_t used_bytes;           ///< Currently allocated bytes
    size_t free_bytes;           ///< Available bytes
    size_t allocation_count;     ///< Number of active allocations
    size_t allocation_overhead;  ///< Per-allocation overhead in bytes
    double fragmentation_ratio;  ///< Fragmentation ratio (0.0-1.0)
    bool supports_defrag;        ///< Whether defragmentation is supported
};

/**
 * @brief Memory pressure information
 */
struct MemoryPressureInfo {
    MemoryPressureLevel level;      ///< Current pressure level
    size_t available_bytes;         ///< Bytes available for allocation
    size_t emergency_reserve;       ///< Emergency reserve size
    double pressure_ratio;          ///< Pressure ratio (0.0-1.0)
    bool gc_recommended;           ///< Garbage collection recommended
    uint64_t last_pressure_time;   ///< Timestamp of last pressure event
};

/**
 * @brief Memory allocation request parameters
 */
struct AllocationRequest {
    size_t size;                    ///< Requested size in bytes
    MemoryAlignment alignment;      ///< Required alignment
    MemoryFlags flags;             ///< Allocation flags
    MemoryType preferred_type;     ///< Preferred memory type
    AllocatorType preferred_allocator; ///< Preferred allocator
    uint32_t timeout_ms;           ///< Allocation timeout in milliseconds
    bool allow_fallback;           ///< Allow fallback to different type/allocator
};

/**
 * @brief Memory region descriptor for specialized memory areas
 */
struct MemoryRegion {
    void* base_address;            ///< Base address of the region
    size_t size;                   ///< Size of the region in bytes
    MemoryType type;              ///< Type of memory in this region
    MemoryFlags capabilities;      ///< Supported capabilities
    bool is_managed;              ///< Whether region is managed by HAL
    std::string_view name;        ///< Human-readable region name
};

// Forward declarations
class IMemoryAllocator;
class IMemoryPressureMonitor;

/**
 * @brief Memory pressure callback function type
 * 
 * Called when memory pressure level changes. Implementations should
 * respond by freeing non-essential memory or preparing for low-memory conditions.
 */
using MemoryPressureCallback = std::function<void(MemoryPressureLevel level, const MemoryPressureInfo& info)>;

/**
 * @brief Memory defragmentation progress callback
 */
using DefragmentationCallback = std::function<void(size_t bytes_moved, size_t total_bytes, double progress)>;

/**
 * @brief Core memory allocator interface
 * 
 * Abstract interface for individual memory allocators. Each allocator
 * implements a specific allocation strategy (pool, buddy, linear, etc.)
 */
class IMemoryAllocator {
public:
    virtual ~IMemoryAllocator() = default;
    
    /**
     * @brief Allocate memory from this allocator
     * @param size Number of bytes to allocate
     * @param alignment Required alignment
     * @return HALResult containing allocated pointer on success
     */
    virtual HALResult<void*> allocate(size_t size, size_t alignment = alignof(std::max_align_t)) = 0;
    
    /**
     * @brief Deallocate memory from this allocator
     * @param ptr Pointer to deallocate
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> deallocate(void* ptr) = 0;
    
    /**
     * @brief Reallocate memory (resize existing allocation)
     * @param ptr Existing pointer
     * @param new_size New size in bytes
     * @return HALResult containing new pointer on success
     */
    virtual HALResult<void*> reallocate(void* ptr, size_t new_size) = 0;
    
    /**
     * @brief Get allocator-specific statistics
     * @return Statistics for this allocator
     */
    virtual AllocatorStats get_stats() const = 0;
    
    /**
     * @brief Get allocator type
     * @return Type of this allocator
     */
    virtual AllocatorType get_type() const = 0;
    
    /**
     * @brief Get human-readable allocator name
     * @return String name of the allocator
     */
    virtual std::string_view get_name() const = 0;
    
    /**
     * @brief Check if allocator supports specific size
     * @param size Size to check
     * @return true if size is supported
     */
    virtual bool supports_size(size_t size) const = 0;
    
    /**
     * @brief Check if allocator supports specific alignment
     * @param alignment Alignment to check
     * @return true if alignment is supported
     */
    virtual bool supports_alignment(size_t alignment) const = 0;
    
    /**
     * @brief Check if pointer was allocated by this allocator
     * @param ptr Pointer to check
     * @return true if pointer belongs to this allocator
     */
    virtual bool owns_pointer(void* ptr) const = 0;
    
    /**
     * @brief Reset allocator to initial state (if supported)
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> reset() = 0;
    
    /**
     * @brief Perform defragmentation (if supported)
     * @param callback Optional progress callback
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> defragment(DefragmentationCallback callback = nullptr) = 0;
};

/**
 * @brief Memory pressure monitoring interface
 * 
 * Provides memory pressure detection and notification capabilities
 * for proactive memory management.
 */
class IMemoryPressureMonitor {
public:
    virtual ~IMemoryPressureMonitor() = default;
    
    /**
     * @brief Register callback for memory pressure events
     * @param level Pressure level to monitor
     * @param callback Callback function to invoke
     * @return HALResult with callback ID on success
     */
    virtual HALResult<uint32_t> register_callback(MemoryPressureLevel level, 
                                                  MemoryPressureCallback callback) = 0;
    
    /**
     * @brief Unregister pressure callback
     * @param callback_id ID returned from register_callback
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> unregister_callback(uint32_t callback_id) = 0;
    
    /**
     * @brief Get current memory pressure information
     * @return Current pressure information
     */
    virtual MemoryPressureInfo get_pressure_info() const = 0;
    
    /**
     * @brief Force pressure level check and callback invocation
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> check_pressure() = 0;
    
    /**
     * @brief Set emergency memory reserve
     * @param bytes Number of bytes to reserve
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_emergency_reserve(size_t bytes) = 0;
    
    /**
     * @brief Get emergency memory reserve size
     * @return Size of emergency reserve in bytes
     */
    virtual size_t get_emergency_reserve() const = 0;
};

/**
 * @brief Enhanced memory management interface
 * 
 * Comprehensive memory management system supporting multiple allocators,
 * memory types, pressure management, and platform-specific optimizations.
 */
class IMemoryInterface : public IHALInterface {
public:
    virtual ~IMemoryInterface() = default;
    
    // === Core Allocation Methods ===
    
    /**
     * @brief Allocate memory with specified parameters
     * @param request Allocation request parameters
     * @return HALResult containing MemoryAllocation on success
     */
    virtual HALResult<MemoryAllocation> allocate(const AllocationRequest& request) = 0;
    
    /**
     * @brief Simplified allocation with size and alignment
     * @param size Number of bytes to allocate
     * @param alignment Memory alignment requirements
     * @param flags Additional allocation flags
     * @return HALResult containing MemoryAllocation on success
     */
    virtual HALResult<MemoryAllocation> allocate(size_t size, 
                                                 MemoryAlignment alignment = MemoryAlignment::None, 
                                                 MemoryFlags flags = MemoryFlags::None) = 0;
    
    /**
     * @brief Allocate from specific memory type
     * @param size Number of bytes to allocate
     * @param type Memory type to allocate from
     * @param alignment Memory alignment requirements
     * @param flags Additional allocation flags
     * @return HALResult containing MemoryAllocation on success
     */
    virtual HALResult<MemoryAllocation> allocate_typed(size_t size,
                                                       MemoryType type,
                                                       MemoryAlignment alignment = MemoryAlignment::None,
                                                       MemoryFlags flags = MemoryFlags::None) = 0;
    
    /**
     * @brief Free previously allocated memory
     * @param ptr Pointer to memory to free
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> deallocate(void* ptr) = 0;
    
    /**
     * @brief Reallocate memory with new size
     * @param ptr Existing memory pointer
     * @param new_size New size in bytes
     * @return HALResult containing new MemoryAllocation on success
     */
    virtual HALResult<MemoryAllocation> reallocate(void* ptr, size_t new_size) = 0;
    
    // === Memory Information Methods ===
    
    /**
     * @brief Get comprehensive memory statistics
     * @return HALResult containing MemoryStats structure
     */
    virtual HALResult<MemoryStats> get_memory_stats() const = 0;
    
    /**
     * @brief Get statistics for specific memory type
     * @param type Memory type to query
     * @return HALResult containing MemoryStats for the type
     */
    virtual HALResult<MemoryStats> get_memory_stats(MemoryType type) const = 0;
    
    /**
     * @brief Get list of available memory regions
     * @return HALResult containing vector of memory regions
     */
    virtual HALResult<std::vector<MemoryRegion>> get_memory_regions() const = 0;
    
    /**
     * @brief Validate a memory pointer
     * @param ptr Pointer to validate
     * @return true if pointer was allocated by this interface and is valid
     */
    virtual bool is_valid_pointer(void* ptr) const = 0;
    
    /**
     * @brief Get allocation information for a pointer
     * @param ptr Pointer to query
     * @return HALResult containing MemoryAllocation info on success
     */
    virtual HALResult<MemoryAllocation> get_allocation_info(void* ptr) const = 0;
    
    // === Allocator Management ===
    
    /**
     * @brief Get allocator for specific type
     * @param type Allocator type to retrieve
     * @return HALResult containing allocator pointer on success
     */
    virtual HALResult<IMemoryAllocator*> get_allocator(AllocatorType type) = 0;
    
    /**
     * @brief Get allocator statistics
     * @param type Allocator type to query
     * @return HALResult containing AllocatorStats on success
     */
    virtual HALResult<AllocatorStats> get_allocator_stats(AllocatorType type) const = 0;
    
    /**
     * @brief Get list of available allocator types
     * @return Vector of supported allocator types
     */
    virtual std::vector<AllocatorType> get_available_allocators() const = 0;
    
    // === Memory Pressure Management ===
    
    /**
     * @brief Get memory pressure monitor
     * @return Pointer to pressure monitor interface
     */
    virtual IMemoryPressureMonitor* get_pressure_monitor() = 0;
    
    /**
     * @brief Register memory pressure callback
     * @param level Pressure level to monitor
     * @param callback Callback function
     * @return HALResult with callback ID on success
     */
    virtual HALResult<uint32_t> register_pressure_callback(MemoryPressureLevel level,
                                                           MemoryPressureCallback callback) = 0;
    
    /**
     * @brief Unregister pressure callback
     * @param callback_id Callback ID to remove
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> unregister_pressure_callback(uint32_t callback_id) = 0;
    
    // === Capability Queries ===
    
    /**
     * @brief Check if specific alignment is supported
     * @param alignment Alignment to check
     * @return true if alignment is supported
     */
    virtual bool supports_alignment(MemoryAlignment alignment) const = 0;
    
    /**
     * @brief Check if specific memory flags are supported
     * @param flags Memory flags to check
     * @return true if all flags are supported  
     */
    virtual bool supports_flags(MemoryFlags flags) const = 0;
    
    /**
     * @brief Check if specific memory type is supported
     * @param type Memory type to check
     * @return true if type is supported
     */
    virtual bool supports_memory_type(MemoryType type) const = 0;
    
    /**
     * @brief Check if specific allocator type is supported
     * @param type Allocator type to check
     * @return true if allocator is supported
     */
    virtual bool supports_allocator_type(AllocatorType type) const = 0;
    
    /**
     * @brief Get maximum single allocation size
     * @return Maximum size in bytes for a single allocation
     */
    virtual size_t get_max_allocation_size() const = 0;
    
    /**
     * @brief Get maximum allocation size for specific type
     * @param type Memory type to query
     * @return Maximum size for the memory type
     */
    virtual size_t get_max_allocation_size(MemoryType type) const = 0;
    
    // === Memory Management Operations ===
    
    /**
     * @brief Perform global memory defragmentation
     * @param callback Optional progress callback
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> defragment(DefragmentationCallback callback = nullptr) = 0;
    
    /**
     * @brief Perform defragmentation on specific allocator
     * @param type Allocator type to defragment
     * @param callback Optional progress callback  
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> defragment_allocator(AllocatorType type, 
                                                 DefragmentationCallback callback = nullptr) = 0;
    
    /**
     * @brief Perform garbage collection hint (for platforms that support it)
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> gc_hint() = 0;
    
    /**
     * @brief Trim unused memory back to system (if supported)
     * @return HALResult with bytes returned to system
     */
    virtual HALResult<size_t> trim_memory() = 0;
    
    // === IHALInterface implementation ===
    std::string_view get_interface_name() const override {
        return "memory";
    }
};

// === Utility Functions ===

/**
 * @brief Memory management utility functions
 */
namespace memory {

/**
 * @brief Convert memory type to string
 * @param type Memory type to convert
 * @return String representation
 */
const char* to_string(MemoryType type);

/**
 * @brief Convert allocator type to string
 * @param type Allocator type to convert
 * @return String representation
 */
const char* to_string(AllocatorType type);

/**
 * @brief Convert memory pressure level to string
 * @param level Pressure level to convert
 * @return String representation
 */
const char* to_string(MemoryPressureLevel level);

/**
 * @brief Calculate aligned size
 * @param size Original size
 * @param alignment Required alignment
 * @return Aligned size
 */
inline size_t align_size(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

/**
 * @brief Check if address is aligned
 * @param ptr Address to check
 * @param alignment Required alignment
 * @return true if address is properly aligned
 */
inline bool is_aligned(void* ptr, size_t alignment) {
    return (reinterpret_cast<uintptr_t>(ptr) & (alignment - 1)) == 0;
}

/**
 * @brief Create default allocation request
 * @param size Size to allocate
 * @param type Preferred memory type
 * @return Configured AllocationRequest
 */
AllocationRequest make_allocation_request(size_t size, 
                                         MemoryType type = MemoryType::System,
                                         MemoryAlignment alignment = MemoryAlignment::None,
                                         MemoryFlags flags = MemoryFlags::None);

} // namespace memory

} // namespace flight::hal
