/**
 * @file simple_platform_memory_driver.cpp
 * @brief Implementation of Simple Platform Memory Driver
 * 
 * This implementation demonstrates all the patterns from the Integration
 * Guidelines with detailed comments explaining each design decision.
 */

#include "simple_platform_memory_driver.hpp"
#include "../../include/flight/hal/core/hal_error.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>

// Platform-specific includes (replace with your platform's headers)
#ifdef __APPLE__
#include <sys/sysctl.h>
#include <mach/mach.h>
#elif defined(__linux__)
#include <sys/sysinfo.h>
#elif defined(_WIN32)
#include <windows.h>
#elif defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

namespace flight::hal::examples::integration {

// === Constructor/Destructor ===

SimplePlatformMemoryDriver::SimplePlatformMemoryDriver() {
    // Constructor should be lightweight - initialization happens in initialize()
}

SimplePlatformMemoryDriver::~SimplePlatformMemoryDriver() {
    // Integration Guidelines: Always verify cleanup in destructor
    if (initialized_) {
        auto shutdown_result = shutdown();
        if (!shutdown_result) {
            // Log error but don't throw in destructor
            // In real implementation, use proper logging
        }
    }
    
    // Verify all allocations have been freed
    std::lock_guard<std::mutex> lock(allocations_mutex_);
    if (!allocations_.empty()) {
        // In real implementation, log this warning
        // Clean up remaining allocations to prevent leaks
        for (const auto& [ptr, entry] : allocations_) {
            platform_deallocate(ptr);
        }
    }
}

// === Core Interface Implementation ===

HALResult<void> SimplePlatformMemoryDriver::initialize() {
    if (initialized_) {
        return HALResult<void>::success(); // Already initialized
    }
    
    // Check platform availability first
    if (!platform_is_available()) {
        return flight::hal::errors::initialization_failed(1, "Platform not available for memory driver");
    }
    
    // Initialize platform capabilities (Integration Guidelines Section 1.4)
    init_platform_capabilities();
    
    // Initialize platform information
    init_platform_info();
    
    // Set up platform constraints
    total_system_memory_ = get_system_memory_size();
    if (total_system_memory_ == 0) {
        return HALError::InitializationFailed("Failed to determine system memory size");
    }
    
    // Set platform-appropriate limits
    if (performance_tier_ == PerformanceTier::Minimal) {
        max_allocation_size_ = 1 * 1024 * 1024;  // 1MB for minimal platforms
        memory_pressure_threshold_ = total_system_memory_ * 0.9; // 90% threshold
    } else if (performance_tier_ == PerformanceTier::Limited) {
        max_allocation_size_ = 16 * 1024 * 1024; // 16MB for limited platforms
        memory_pressure_threshold_ = total_system_memory_ * 0.8; // 80% threshold
    } else {
        max_allocation_size_ = 1024 * 1024 * 1024; // 1GB for high-performance platforms
        memory_pressure_threshold_ = total_system_memory_ * 0.7; // 70% threshold
    }
    
    initialized_ = true;
    active_ = true;
    
    return HALResult<void>::success();
}

HALResult<void> SimplePlatformMemoryDriver::shutdown() {
    if (!initialized_) {
        return HALResult<void>::success(); // Already shutdown
    }
    
    active_ = false;
    
    // Clean up all outstanding allocations
    std::lock_guard<std::mutex> lock(allocations_mutex_);
    for (const auto& [ptr, entry] : allocations_) {
        platform_deallocate(ptr);
    }
    allocations_.clear();
    
    // Reset statistics
    total_allocated_.store(0);
    peak_allocated_.store(0);
    allocation_count_.store(0);
    deallocation_count_.store(0);
    
    initialized_ = false;
    
    return HALResult<void>::success();
}

HALResult<MemoryAllocation> SimplePlatformMemoryDriver::allocate(const AllocationRequest& request) {
    return allocate(request.size, request.alignment, request.flags);
}

HALResult<MemoryAllocation> SimplePlatformMemoryDriver::allocate(size_t size, 
                                                               MemoryAlignment alignment, 
                                                               MemoryFlags flags) {
    // Integration Guidelines Section 1.3: HALResult error handling pattern
    
    if (!initialized_ || !active_) {
        return HALError::NotInitialized("Memory driver not initialized");
    }
    
    // Validate parameters
    if (size == 0) {
        return HALError::InvalidParameter("Allocation size cannot be zero");
    }
    
    // Check platform limits
    if (would_exceed_platform_limits(size)) {
        return HALError::InsufficientResources("Allocation exceeds platform limits");
    }
    
    // Check supported alignment
    if (!supports_alignment(alignment)) {
        return HALError::UnsupportedAlignment("Alignment not supported on this platform");
    }
    
    // Check supported flags
    if (!supports_flags(flags)) {
        return HALError::UnsupportedFlags("Memory flags not supported on this platform");
    }
    
    // Platform-specific allocation
    void* ptr = platform_allocate(size, alignment);
    if (!ptr) {
        return HALError::AllocationFailed("Platform allocation failed");
    }
    
    // Track allocation (Integration Guidelines Section 1.5)
    AllocationEntry entry(size, alignment, flags);
    {
        std::lock_guard<std::mutex> lock(allocations_mutex_);
        allocations_[ptr] = entry;
    }
    
    // Update statistics
    size_t new_total = total_allocated_.fetch_add(size) + size;
    
    // Update peak if necessary
    size_t current_peak = peak_allocated_.load();
    while (new_total > current_peak && 
           !peak_allocated_.compare_exchange_weak(current_peak, new_total)) {
        // CAS loop to update peak
    }
    
    allocation_count_.fetch_add(1);
    
    // Create allocation metadata
    MemoryAllocation allocation;
    allocation.ptr = ptr;
    allocation.size = size;
    allocation.alignment = alignment;
    allocation.flags = flags;
    
    return allocation;
}

HALResult<void> SimplePlatformMemoryDriver::deallocate(void* ptr) {
    if (!ptr) {
        return HALError::InvalidParameter("Cannot deallocate null pointer");
    }
    
    if (!initialized_ || !active_) {
        return HALError::NotInitialized("Memory driver not initialized");
    }
    
    // Find and remove allocation entry
    size_t size = 0;
    {
        std::lock_guard<std::mutex> lock(allocations_mutex_);
        auto it = allocations_.find(ptr);
        if (it == allocations_.end()) {
            return HALError::InvalidParameter("Pointer not found in allocation table");
        }
        
        if (!it->second.valid) {
            return HALError::InvalidParameter("Pointer has already been deallocated");
        }
        
        size = it->second.size;
        allocations_.erase(it);
    }
    
    // Platform-specific deallocation
    platform_deallocate(ptr);
    
    // Update statistics
    total_allocated_.fetch_sub(size);
    deallocation_count_.fetch_add(1);
    
    return HALResult<void>::success();
}

HALResult<MemoryAllocation> SimplePlatformMemoryDriver::reallocate(void* ptr, size_t new_size) {
    if (!ptr) {
        // Reallocate with null pointer is equivalent to allocate
        return allocate(new_size);
    }
    
    if (new_size == 0) {
        // Reallocate with zero size is equivalent to deallocate
        auto result = deallocate(ptr);
        if (!result) {
            return HALError::from_error(result.error());
        }
        
        MemoryAllocation empty_allocation{};
        empty_allocation.ptr = nullptr;
        empty_allocation.size = 0;
        return empty_allocation;
    }
    
    // Get current allocation info
    auto current_info = get_allocation_info(ptr);
    if (!current_info) {
        return HALError::from_error(current_info.error());
    }
    
    // If new size is same as current, return current allocation
    if (new_size == current_info.value().size) {
        return current_info.value();
    }
    
    // Allocate new memory
    auto new_allocation = allocate(new_size, current_info.value().alignment, current_info.value().flags);
    if (!new_allocation) {
        return HALError::from_error(new_allocation.error());
    }
    
    // Copy data from old to new
    size_t copy_size = std::min(current_info.value().size, new_size);
    std::memcpy(new_allocation.value().ptr, ptr, copy_size);
    
    // Deallocate old memory
    auto dealloc_result = deallocate(ptr);
    if (!dealloc_result) {
        // Clean up new allocation and return error
        deallocate(new_allocation.value().ptr);
        return HALError::from_error(dealloc_result.error());
    }
    
    return new_allocation.value();
}

// === Memory Information Methods ===

HALResult<MemoryStats> SimplePlatformMemoryDriver::get_memory_stats() const {
    if (!initialized_) {
        return HALError::NotInitialized("Memory driver not initialized");
    }
    
    MemoryStats stats;
    stats.total_memory = total_system_memory_;
    stats.used_memory = total_allocated_.load();
    stats.available_memory = total_system_memory_ - stats.used_memory;
    stats.peak_memory = peak_allocated_.load();
    stats.allocation_count = allocation_count_.load();
    stats.deallocation_count = deallocation_count_.load();
    
    // Calculate fragmentation (simplified)
    std::lock_guard<std::mutex> lock(allocations_mutex_);
    stats.fragmentation_ratio = allocations_.empty() ? 0.0f : 
        static_cast<float>(allocations_.size()) / static_cast<float>(allocation_count_.load());
    
    return stats;
}

bool SimplePlatformMemoryDriver::is_valid_pointer(void* ptr) const {
    if (!ptr) return false;
    
    std::lock_guard<std::mutex> lock(allocations_mutex_);
    auto it = allocations_.find(ptr);
    return it != allocations_.end() && it->second.valid;
}

HALResult<MemoryAllocation> SimplePlatformMemoryDriver::get_allocation_info(void* ptr) const {
    if (!ptr) {
        return HALError::InvalidParameter("Cannot get info for null pointer");
    }
    
    std::lock_guard<std::mutex> lock(allocations_mutex_);
    auto it = allocations_.find(ptr);
    if (it == allocations_.end()) {
        return HALError::InvalidParameter("Pointer not found in allocation table");
    }
    
    if (!it->second.valid) {
        return HALError::InvalidParameter("Pointer has been deallocated");
    }
    
    MemoryAllocation allocation;
    allocation.ptr = ptr;
    allocation.size = it->second.size;
    allocation.alignment = it->second.alignment;
    allocation.flags = it->second.flags;
    
    return allocation;
}

// === Capability Queries ===

bool SimplePlatformMemoryDriver::supports_alignment(MemoryAlignment alignment) const {
    // Platform-specific alignment support
    size_t align_bytes = static_cast<size_t>(alignment);
    
    // Check if alignment is power of 2 and within platform limits
    if (align_bytes == 0 || (align_bytes & (align_bytes - 1)) != 0) {
        return false; // Not power of 2
    }
    
    // Platform-specific maximum alignment
    size_t max_alignment = 4096; // 4KB page alignment as maximum
    return align_bytes <= max_alignment;
}

bool SimplePlatformMemoryDriver::supports_flags(MemoryFlags flags) const {
    // Basic implementation - check each flag
    if (flags & MemoryFlags::Executable) {
        return supports_capability(HALCapability::ExecutableMemory);
    }
    
    if (flags & MemoryFlags::DMA) {
        return supports_capability(HALCapability::DMA);
    }
    
    // Other flags are generally supported
    return true;
}

size_t SimplePlatformMemoryDriver::get_max_allocation_size() const {
    return max_allocation_size_;
}

// === Memory Management Operations ===

HALResult<void> SimplePlatformMemoryDriver::gc_hint() {
    // Platform-specific garbage collection hint
    // This is a no-op for most platforms, but useful for web/managed environments
    
#ifdef __EMSCRIPTEN__
    // In web environment, suggest garbage collection
    EM_ASM({
        if (typeof gc !== 'undefined') {
            gc();
        }
    });
#endif
    
    return HALResult<void>::success();
}

HALResult<size_t> SimplePlatformMemoryDriver::trim_memory() {
    // Trim unused memory - simplified implementation
    size_t trimmed = 0;
    
    // In real implementation, this might:
    // - Return memory to the OS
    // - Defragment memory pools
    // - Release cached allocations
    
    return trimmed;
}

// === ICapabilityProvider Implementation ===

std::vector<HALCapability> SimplePlatformMemoryDriver::get_capabilities() const {
    std::vector<HALCapability> capabilities;
    
    // Check each capability bit
    for (uint32_t i = 0; i < 32; ++i) {
        HALCapability cap = static_cast<HALCapability>(1u << i);
        if (supports_capability(cap)) {
            capabilities.push_back(cap);
        }
    }
    
    return capabilities;
}

// === Platform-Specific Implementation ===

void SimplePlatformMemoryDriver::init_platform_capabilities() {
    // Integration Guidelines Section 1.4: Platform-specific capability detection
    
    capability_mask_ = 0;
    
    // Basic memory capability is always supported
    capability_mask_ |= static_cast<uint32_t>(HALCapability::BasicMemory);
    
    // Platform-specific capability detection
#ifdef __APPLE__
    // macOS: High-performance platform with all capabilities
    capability_mask_ |= static_cast<uint32_t>(HALCapability::Threading);
    capability_mask_ |= static_cast<uint32_t>(HALCapability::VirtualMemory);
    capability_mask_ |= static_cast<uint32_t>(HALCapability::MemoryMapping);
    capability_mask_ |= static_cast<uint32_t>(HALCapability::ExecutableMemory);
    capability_mask_ |= static_cast<uint32_t>(HALCapability::Hardware3D);
    performance_tier_ = PerformanceTier::High;
    
#elif defined(__linux__)
    // Linux: Similar to macOS
    capability_mask_ |= static_cast<uint32_t>(HALCapability::Threading);
    capability_mask_ |= static_cast<uint32_t>(HALCapability::VirtualMemory);
    capability_mask_ |= static_cast<uint32_t>(HALCapability::MemoryMapping);
    capability_mask_ |= static_cast<uint32_t>(HALCapability::ExecutableMemory);
    performance_tier_ = PerformanceTier::High;
    
#elif defined(_WIN32)
    // Windows: High-performance platform
    capability_mask_ |= static_cast<uint32_t>(HALCapability::Threading);
    capability_mask_ |= static_cast<uint32_t>(HALCapability::VirtualMemory);
    capability_mask_ |= static_cast<uint32_t>(HALCapability::MemoryMapping);
    capability_mask_ |= static_cast<uint32_t>(HALCapability::ExecutableMemory);
    capability_mask_ |= static_cast<uint32_t>(HALCapability::Hardware3D);
    performance_tier_ = PerformanceTier::High;
    
#elif defined(__EMSCRIPTEN__)
    // Web: Limited platform with browser restrictions
    capability_mask_ |= static_cast<uint32_t>(HALCapability::WebGL);
    capability_mask_ |= static_cast<uint32_t>(HALCapability::WebAudio);
    capability_mask_ |= static_cast<uint32_t>(HALCapability::SandboxRestrictions);
    performance_tier_ = PerformanceTier::Standard;
    
#else
    // Unknown platform: assume minimal capabilities
    performance_tier_ = PerformanceTier::Minimal;
#endif
}

void SimplePlatformMemoryDriver::init_platform_info() {
    platform_info_ = std::make_unique<PlatformInfo>();
    
#ifdef __APPLE__
    platform_info_->platform_name = "macOS";
    #ifdef __x86_64__
    platform_info_->architecture = "x86_64";
    #elif defined(__arm64__)
    platform_info_->architecture = "arm64";
    #else
    platform_info_->architecture = "unknown";
    #endif
    platform_info_->memory_alignment = 16;
    
#elif defined(__linux__)
    platform_info_->platform_name = "Linux";
    #ifdef __x86_64__
    platform_info_->architecture = "x86_64";
    #elif defined(__aarch64__)
    platform_info_->architecture = "aarch64";
    #else
    platform_info_->architecture = "unknown";
    #endif
    platform_info_->memory_alignment = 16;
    
#elif defined(_WIN32)
    platform_info_->platform_name = "Windows";
    #ifdef _M_X64
    platform_info_->architecture = "x64";
    #elif defined(_M_ARM64)
    platform_info_->architecture = "arm64";
    #else
    platform_info_->architecture = "x86";
    #endif
    platform_info_->memory_alignment = 16;
    
#elif defined(__EMSCRIPTEN__)
    platform_info_->platform_name = "Web Browser";
    platform_info_->architecture = "WebAssembly";
    platform_info_->memory_alignment = 8;
    
#else
    platform_info_->platform_name = "Unknown";
    platform_info_->architecture = "unknown";
    platform_info_->memory_alignment = 8;
#endif
    
    platform_info_->total_memory = get_system_memory_size();
}

bool SimplePlatformMemoryDriver::platform_is_available() const {
    // Check if platform supports this driver
    // In real implementation, this might check:
    // - Required system APIs
    // - Hardware capabilities
    // - Driver dependencies
    
    return true; // Simple implementation always available
}

void* SimplePlatformMemoryDriver::platform_allocate(size_t size, MemoryAlignment alignment) {
    // Platform-specific allocation implementation
    // Integration Guidelines Section 2: Replace with platform APIs
    
    size_t align_bytes = static_cast<size_t>(alignment);
    if (align_bytes == 0) {
        align_bytes = sizeof(void*); // Default alignment
    }
    
    // Use aligned allocation
    void* ptr = nullptr;
    
#ifdef _WIN32
    ptr = _aligned_malloc(size, align_bytes);
#elif defined(__APPLE__) || defined(__linux__)
    if (posix_memalign(&ptr, align_bytes, size) != 0) {
        ptr = nullptr;
    }
#else
    // Fallback to standard malloc for unknown platforms
    ptr = std::malloc(size);
#endif
    
    return ptr;
}

void SimplePlatformMemoryDriver::platform_deallocate(void* ptr) {
    if (!ptr) return;
    
    // Platform-specific deallocation
#ifdef _WIN32
    _aligned_free(ptr);
#else
    std::free(ptr);
#endif
}

size_t SimplePlatformMemoryDriver::get_system_memory_size() const {
    // Platform-specific memory size detection
    
#ifdef __APPLE__
    int mib[2] = {CTL_HW, HW_MEMSIZE};
    uint64_t memory_size;
    size_t length = sizeof(memory_size);
    
    if (sysctl(mib, 2, &memory_size, &length, nullptr, 0) == 0) {
        return static_cast<size_t>(memory_size);
    }
    
#elif defined(__linux__)
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        return static_cast<size_t>(info.totalram * info.mem_unit);
    }
    
#elif defined(_WIN32)
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status)) {
        return static_cast<size_t>(status.ullTotalPhys);
    }
    
#elif defined(__EMSCRIPTEN__)
    // Web platform: use conservative estimate
    return 50 * 1024 * 1024; // 50MB browser limit
    
#endif
    
    // Fallback for unknown platforms
    return 1024 * 1024 * 1024; // 1GB default
}

bool SimplePlatformMemoryDriver::would_exceed_platform_limits(size_t size) const {
    // Check if allocation would exceed platform-specific limits
    
    if (size > max_allocation_size_) {
        return true;
    }
    
    size_t new_total = total_allocated_.load() + size;
    if (new_total > memory_pressure_threshold_) {
        return true;
    }
    
    return false;
}

size_t SimplePlatformMemoryDriver::calculate_aligned_size(size_t size, MemoryAlignment alignment) const {
    size_t align_bytes = static_cast<size_t>(alignment);
    if (align_bytes <= 1) {
        return size;
    }
    
    return (size + align_bytes - 1) & ~(align_bytes - 1);
}

} // namespace flight::hal::examples::integration
