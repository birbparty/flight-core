#ifndef FLIGHT_WASM_UTILITIES_MEMORY_HPP
#define FLIGHT_WASM_UTILITIES_MEMORY_HPP

/**
 * @file memory.hpp
 * @brief Platform-aware memory management utilities for Flight WASM
 * 
 * This header provides memory allocation strategies optimized for different
 * platforms, with special support for embedded systems with limited memory.
 */

#include <flight/wasm/utilities/platform.hpp>
#include <memory>
#include <new>
#include <cstdlib>
#include <stack>
#include <cstring>

namespace flight::wasm::memory {

    // WebAssembly memory requirements
    constexpr size_t WASM_PAGE_SIZE = 65536; // 64KB pages (spec requirement)
    constexpr size_t MAX_MEMORY_PAGES = 65536; // 4GB max (spec limit)
    constexpr size_t MAX_WASM_MEMORY = MAX_MEMORY_PAGES * WASM_PAGE_SIZE;

    // Platform-specific memory allocation
    namespace detail {
        // Platform-specific aligned allocation implementations
        #ifdef __DREAMCAST__
        #include <kos.h>
        #include <dc/sq.h>
        
        inline void* dreamcast_memalign(size_t alignment, size_t size) noexcept {
            // Use Dreamcast's store queue for aligned allocations when possible
            if (alignment == 32 && size <= 32) {
                // Store queue aligned allocation for small objects
                static thread_local uintptr_t sq_ptr = 0xe0000000;
                if (sq_ptr + size <= 0xe0000020) { // 32-byte store queue
                    void* result = reinterpret_cast<void*>(sq_ptr);
                    sq_ptr += ((size + 31) & ~31); // Align to 32 bytes
                    return result;
                }
            }
            return memalign(alignment, size);
        }
        
        inline void dreamcast_free(void* ptr) noexcept {
            uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
            if (addr >= 0xe0000000 && addr < 0xe0000020) {
                // Store queue memory, no need to free
                return;
            }
            free(ptr);
        }
        
        #elif defined(__PSP__)
        #include <pspsdk.h>
        #include <pspkernel.h>
        
        inline void* psp_memalign(size_t alignment, size_t size) noexcept {
            return memalign(alignment, size);
        }
        
        inline void psp_free(void* ptr) noexcept {
            free(ptr);
        }
        
        #elif defined(__vita__)
        #include <vitasdk.h>
        #include <psp2/kernel/processmgr.h>
        
        inline void* vita_memalign(size_t alignment, size_t size) noexcept {
            return memalign(alignment, size);
        }
        
        inline void vita_free(void* ptr) noexcept {
            free(ptr);
        }
        
        #else
        // Standard platforms
        inline void* standard_memalign(size_t alignment, size_t size) noexcept {
            #if defined(_WIN32)
                return _aligned_malloc(size, alignment);
            #elif defined(__APPLE__) || defined(__linux__)
                void* ptr = nullptr;
                if (posix_memalign(&ptr, alignment, size) == 0) {
                    return ptr;
                }
                return nullptr;
            #else
                return std::aligned_alloc(alignment, size);
            #endif
        }
        
        inline void standard_free(void* ptr) noexcept {
            #if defined(_WIN32)
                _aligned_free(ptr);
            #else
                std::free(ptr);
            #endif
        }
        #endif
    }

    // Platform-aware aligned allocator
    class PlatformAllocator {
    public:
        // Allocate aligned memory using platform-specific optimizations
        static void* allocate_aligned(size_t size, size_t alignment) noexcept {
            if (size == 0) return nullptr;
            
            // Ensure alignment is power of 2 and at least sizeof(void*)
            if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
                alignment = sizeof(void*);
            }
            if (alignment < sizeof(void*)) {
                alignment = sizeof(void*);
            }
            
            // Round size up to alignment boundary for better cache behavior
            size = (size + alignment - 1) & ~(alignment - 1);
            
            // Check platform memory limits
            if constexpr (platform::CurrentPlatform::is_embedded) {
                if (size > platform::CurrentPlatform::max_memory / 4) {
                    return nullptr; // Allocation too large for embedded platform
                }
            }
            
            #ifdef __DREAMCAST__
                return detail::dreamcast_memalign(alignment, size);
            #elif defined(__PSP__)
                return detail::psp_memalign(alignment, size);
            #elif defined(__vita__)
                return detail::vita_memalign(alignment, size);
            #else
                return detail::standard_memalign(alignment, size);
            #endif
        }
        
        static void deallocate_aligned(void* ptr) noexcept {
            if (!ptr) return;
            
            #ifdef __DREAMCAST__
                detail::dreamcast_free(ptr);
            #elif defined(__PSP__)
                detail::psp_free(ptr);
            #elif defined(__vita__)
                detail::vita_free(ptr);
            #else
                detail::standard_free(ptr);
            #endif
        }
        
        // Get maximum safe allocation size for current platform
        static constexpr size_t max_allocation_size() noexcept {
            if constexpr (platform::CurrentPlatform::is_embedded) {
                return platform::CurrentPlatform::max_memory / 4; // Reserve 75% for other uses
            } else {
                return SIZE_MAX;
            }
        }
    };

    // STL-compatible aligned allocator
    template<typename T, size_t Alignment = alignof(T)>
    class AlignedAllocator {
    public:
        using value_type = T;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        
        static constexpr size_t alignment = Alignment;
        
        template<typename U>
        struct rebind {
            using other = AlignedAllocator<U, Alignment>;
        };
        
        AlignedAllocator() noexcept = default;
        
        template<typename U>
        AlignedAllocator(const AlignedAllocator<U, Alignment>&) noexcept {}
        
        T* allocate(size_t n) {
            if (n == 0) return nullptr;
            
            const size_t size = n * sizeof(T);
            void* ptr = PlatformAllocator::allocate_aligned(size, alignment);
            if (!ptr) {
                throw std::bad_alloc{};
            }
            
            return static_cast<T*>(ptr);
        }
        
        void deallocate(T* ptr, size_t) noexcept {
            PlatformAllocator::deallocate_aligned(ptr);
        }
        
        template<typename U, size_t UAlignment>
        bool operator==(const AlignedAllocator<U, UAlignment>&) const noexcept {
            return alignment == UAlignment;
        }
        
        template<typename U, size_t UAlignment>
        bool operator!=(const AlignedAllocator<U, UAlignment>&) const noexcept {
            return !(*this == AlignedAllocator<U, UAlignment>{});
        }
    };

    // Stack-based allocation for small objects (embedded-friendly)
    template<size_t Size, size_t Alignment = platform::CurrentPlatform::preferred_alignment>
    class StackAllocator {
    public:
        static constexpr size_t buffer_size = Size;
        static constexpr size_t alignment = Alignment;
        
        // Adjust buffer size for embedded platforms
        static constexpr size_t actual_buffer_size = 
            platform::CurrentPlatform::is_embedded ? 
            std::min(Size, platform::CurrentPlatform::stack_size / 8) : Size;
        
        StackAllocator() noexcept : offset_(0) {}
        
        void* allocate(size_t size) noexcept {
            if (size == 0) return nullptr;
            
            const size_t aligned_size = (size + alignment - 1) & ~(alignment - 1);
            
            if (offset_ + aligned_size > actual_buffer_size) {
                return nullptr; // Stack exhausted
            }
            
            void* ptr = buffer_ + offset_;
            offset_ += aligned_size;
            return ptr;
        }
        
        // Stack allocators don't support individual deallocation
        void deallocate(void*) noexcept {
            // No-op - stack allocators reset all at once
        }
        
        void reset() noexcept {
            offset_ = 0;
        }
        
        size_t remaining() const noexcept {
            return actual_buffer_size - offset_;
        }
        
        size_t used() const noexcept {
            return offset_;
        }
        
        bool empty() const noexcept {
            return offset_ == 0;
        }
        
    private:
        alignas(Alignment) char buffer_[actual_buffer_size];
        size_t offset_;
    };

    // Memory pool for frequent allocations (reduces fragmentation)
    template<typename T, size_t PoolSize = 64>
    class ObjectPool {
    public:
        static constexpr size_t pool_size = 
            platform::CurrentPlatform::is_embedded ?
            std::min(PoolSize, platform::CurrentPlatform::max_memory / (sizeof(T) * 16)) : PoolSize;
        
        ObjectPool() {
            // Initialize free list
            for (size_t i = 0; i < pool_size; ++i) {
                free_objects_.push(reinterpret_cast<T*>(&objects_[i]));
            }
        }
        
        ~ObjectPool() {
            // Objects are automatically destroyed with the pool
        }
        
        T* acquire() noexcept {
            if (free_objects_.empty()) {
                return nullptr; // Pool exhausted
            }
            
            T* obj = free_objects_.top();
            free_objects_.pop();
            return obj;
        }
        
        void release(T* obj) noexcept {
            if (!obj) return;
            
            // Verify object belongs to this pool
            const uintptr_t obj_addr = reinterpret_cast<uintptr_t>(obj);
            const uintptr_t pool_start = reinterpret_cast<uintptr_t>(objects_);
            const uintptr_t pool_end = pool_start + sizeof(objects_);
            
            if (obj_addr >= pool_start && obj_addr < pool_end) {
                // Call destructor but don't deallocate memory
                obj->~T();
                free_objects_.push(obj);
            }
            // Ignore objects not from this pool
        }
        
        size_t available() const noexcept {
            return free_objects_.size();
        }
        
        size_t capacity() const noexcept {
            return pool_size;
        }
        
        bool empty() const noexcept {
            return free_objects_.empty();
        }
        
    private:
        // Use union to avoid default construction
        union ObjectStorage {
            T object;
            ObjectStorage() {} // No-op constructor
            ~ObjectStorage() {} // No-op destructor
        };
        
        ObjectStorage objects_[pool_size];
        std::stack<T*> free_objects_;
    };

    // Memory-mapped region for large allocations (platform-specific)
    class MemoryRegion {
    public:
        MemoryRegion() noexcept : data_(nullptr), size_(0) {}
        
        explicit MemoryRegion(size_t size) : size_(size) {
            data_ = PlatformAllocator::allocate_aligned(size, platform::CurrentPlatform::cache_line_size);
            if (!data_) {
                throw std::bad_alloc{};
            }
        }
        
        ~MemoryRegion() {
            if (data_) {
                PlatformAllocator::deallocate_aligned(data_);
            }
        }
        
        // Move-only type
        MemoryRegion(const MemoryRegion&) = delete;
        MemoryRegion& operator=(const MemoryRegion&) = delete;
        
        MemoryRegion(MemoryRegion&& other) noexcept 
            : data_(other.data_), size_(other.size_) {
            other.data_ = nullptr;
            other.size_ = 0;
        }
        
        MemoryRegion& operator=(MemoryRegion&& other) noexcept {
            if (this != &other) {
                if (data_) {
                    PlatformAllocator::deallocate_aligned(data_);
                }
                data_ = other.data_;
                size_ = other.size_;
                other.data_ = nullptr;
                other.size_ = 0;
            }
            return *this;
        }
        
        void* data() noexcept { return data_; }
        const void* data() const noexcept { return data_; }
        size_t size() const noexcept { return size_; }
        bool empty() const noexcept { return size_ == 0; }
        
        // Zero-fill the memory region
        void zero() noexcept {
            if (data_ && size_ > 0) {
                std::memset(data_, 0, size_);
            }
        }
        
    private:
        void* data_;
        size_t size_;
    };

    // Memory usage tracking for embedded platforms
    class MemoryTracker {
    public:
        static MemoryTracker& instance() {
            static MemoryTracker tracker;
            return tracker;
        }
        
        void record_allocation(size_t size) noexcept {
            current_usage_ += size;
            if (current_usage_ > peak_usage_) {
                peak_usage_ = current_usage_;
            }
            ++allocation_count_;
        }
        
        void record_deallocation(size_t size) noexcept {
            if (current_usage_ >= size) {
                current_usage_ -= size;
            }
            ++deallocation_count_;
        }
        
        size_t current_usage() const noexcept { return current_usage_; }
        size_t peak_usage() const noexcept { return peak_usage_; }
        size_t allocation_count() const noexcept { return allocation_count_; }
        size_t deallocation_count() const noexcept { return deallocation_count_; }
        
        void reset() noexcept {
            current_usage_ = 0;
            peak_usage_ = 0;
            allocation_count_ = 0;
            deallocation_count_ = 0;
        }
        
    private:
        MemoryTracker() = default;
        
        size_t current_usage_ = 0;
        size_t peak_usage_ = 0;
        size_t allocation_count_ = 0;
        size_t deallocation_count_ = 0;
    };

    // Convenience aliases for common allocators
    template<typename T>
    using CacheAlignedAllocator = AlignedAllocator<T, platform::CurrentPlatform::cache_line_size>;
    
    template<typename T>
    using PageAlignedAllocator = AlignedAllocator<T, WASM_PAGE_SIZE>;
    
    // Small object optimization stack allocator
    template<size_t Size = 4096>
    using SmallObjectAllocator = StackAllocator<Size, 16>;
    
    // Large object memory region
    using LargeObjectRegion = MemoryRegion;

} // namespace flight::wasm::memory

#endif // FLIGHT_WASM_UTILITIES_MEMORY_HPP
