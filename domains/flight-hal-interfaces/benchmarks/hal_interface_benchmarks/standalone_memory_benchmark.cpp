/**
 * @file standalone_memory_benchmark.cpp
 * @brief Standalone Memory Benchmarks (no HAL dependencies)
 * 
 * Simple performance benchmarks for memory operations without requiring
 * the full HAL infrastructure to build successfully.
 */

#include <benchmark/benchmark.h>
#include <vector>
#include <memory>
#include <random>
#include <cstring>
#include <chrono>

namespace {

// Simple memory allocator simulation
class SimpleAllocator {
public:
    SimpleAllocator() = default;
    
    void* allocate(size_t size, size_t alignment = 8) {
        // Simple aligned allocation
        void* ptr = std::aligned_alloc(alignment, size);
        if (ptr) {
            allocations_.emplace_back(ptr, size);
        }
        return ptr;
    }
    
    void deallocate(void* ptr) {
        if (!ptr) return;
        
        auto it = std::find_if(allocations_.begin(), allocations_.end(),
            [ptr](const auto& alloc) { return alloc.first == ptr; });
        
        if (it != allocations_.end()) {
            std::free(ptr);
            allocations_.erase(it);
        }
    }
    
    size_t get_allocation_count() const {
        return allocations_.size();
    }
    
    void cleanup() {
        for (auto& [ptr, size] : allocations_) {
            std::free(ptr);
        }
        allocations_.clear();
    }
    
private:
    std::vector<std::pair<void*, size_t>> allocations_;
};

} // anonymous namespace

// === Allocation Latency Benchmarks ===

static void BM_Memory_Allocation_Small(benchmark::State& state) {
    SimpleAllocator allocator;
    std::vector<void*> ptrs;
    ptrs.reserve(1000);
    
    for (auto _ : state) {
        void* ptr = allocator.allocate(64);
        if (ptr) {
            ptrs.push_back(ptr);
        }
    }
    
    // Cleanup
    for (void* ptr : ptrs) {
        allocator.deallocate(ptr);
    }
    allocator.cleanup();
}
BENCHMARK(BM_Memory_Allocation_Small);

static void BM_Memory_Allocation_Medium(benchmark::State& state) {
    SimpleAllocator allocator;
    std::vector<void*> ptrs;
    ptrs.reserve(1000);
    
    for (auto _ : state) {
        void* ptr = allocator.allocate(1024);
        if (ptr) {
            ptrs.push_back(ptr);
        }
    }
    
    // Cleanup
    for (void* ptr : ptrs) {
        allocator.deallocate(ptr);
    }
    allocator.cleanup();
}
BENCHMARK(BM_Memory_Allocation_Medium);

static void BM_Memory_Allocation_Large(benchmark::State& state) {
    SimpleAllocator allocator;
    std::vector<void*> ptrs;
    ptrs.reserve(100);
    
    for (auto _ : state) {
        void* ptr = allocator.allocate(65536);
        if (ptr) {
            ptrs.push_back(ptr);
        }
    }
    
    // Cleanup
    for (void* ptr : ptrs) {
        allocator.deallocate(ptr);
    }
    allocator.cleanup();
}
BENCHMARK(BM_Memory_Allocation_Large);

// === Memory Copy Benchmarks ===

static void BM_Memory_Copy_Performance(benchmark::State& state) {
    size_t size = static_cast<size_t>(state.range(0));
    
    auto src = std::make_unique<char[]>(size);
    auto dst = std::make_unique<char[]>(size);
    
    // Initialize source with pattern
    std::memset(src.get(), 0xAA, size);
    
    size_t total_bytes = 0;
    for (auto _ : state) {
        std::memcpy(dst.get(), src.get(), size);
        total_bytes += size;
    }
    
    state.SetBytesProcessed(total_bytes);
}
BENCHMARK(BM_Memory_Copy_Performance)->Range(1024, 1048576); // 1KB to 1MB

// === Memory Set Benchmarks ===

static void BM_Memory_Set_Performance(benchmark::State& state) {
    size_t size = static_cast<size_t>(state.range(0));
    auto buffer = std::make_unique<char[]>(size);
    
    size_t total_bytes = 0;
    for (auto _ : state) {
        std::memset(buffer.get(), 0x55, size);
        total_bytes += size;
    }
    
    state.SetBytesProcessed(total_bytes);
}
BENCHMARK(BM_Memory_Set_Performance)->Range(1024, 1048576); // 1KB to 1MB

// === Allocation Pattern Benchmarks ===

static void BM_Memory_Allocation_Deallocation_Pattern(benchmark::State& state) {
    SimpleAllocator allocator;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> size_dist(64, 4096);
    
    for (auto _ : state) {
        // Allocate some memory
        size_t size = size_dist(gen);
        void* ptr = allocator.allocate(size);
        
        if (ptr) {
            // Use the memory briefly
            std::memset(ptr, 0xBB, size);
            
            // Deallocate
            allocator.deallocate(ptr);
        }
    }
    
    allocator.cleanup();
}
BENCHMARK(BM_Memory_Allocation_Deallocation_Pattern);

// === Memory Alignment Benchmarks ===

static void BM_Memory_Aligned_Allocation(benchmark::State& state) {
    size_t alignment = static_cast<size_t>(state.range(0));
    SimpleAllocator allocator;
    std::vector<void*> ptrs;
    ptrs.reserve(100);
    
    for (auto _ : state) {
        void* ptr = allocator.allocate(1024, alignment);
        if (ptr) {
            ptrs.push_back(ptr);
        }
    }
    
    // Cleanup
    for (void* ptr : ptrs) {
        allocator.deallocate(ptr);
    }
    allocator.cleanup();
}
BENCHMARK(BM_Memory_Aligned_Allocation)
    ->Arg(8)     // 8-byte alignment
    ->Arg(16)    // 16-byte alignment
    ->Arg(64)    // 64-byte alignment (cache line)
    ->Arg(256);  // 256-byte alignment

// === Throughput Benchmarks ===

static void BM_Memory_Allocation_Throughput(benchmark::State& state) {
    SimpleAllocator allocator;
    size_t size = static_cast<size_t>(state.range(0));
    std::vector<void*> ptrs;
    ptrs.reserve(10000);
    
    for (auto _ : state) {
        void* ptr = allocator.allocate(size);
        if (ptr) {
            ptrs.push_back(ptr);
        }
    }
    
    state.SetItemsProcessed(state.iterations());
    
    // Cleanup
    for (void* ptr : ptrs) {
        allocator.deallocate(ptr);
    }
    allocator.cleanup();
}
BENCHMARK(BM_Memory_Allocation_Throughput)->Range(64, 8192);

// === Cross-Platform Validation ===

static void BM_Memory_Cross_Platform_Operations(benchmark::State& state) {
    // Simple cross-platform memory operations that should work everywhere
    SimpleAllocator allocator;
    
    for (auto _ : state) {
        // Allocate memory
        void* ptr = allocator.allocate(1024);
        
        if (ptr) {
            // Fill with pattern
            std::memset(ptr, 0xCC, 1024);
            
            // Verify first few bytes
            char* char_ptr = static_cast<char*>(ptr);
            benchmark::DoNotOptimize(char_ptr[0]);
            benchmark::DoNotOptimize(char_ptr[512]);
            benchmark::DoNotOptimize(char_ptr[1023]);
            
            // Deallocate
            allocator.deallocate(ptr);
        }
    }
    
    allocator.cleanup();
}
BENCHMARK(BM_Memory_Cross_Platform_Operations);

// === Memory Pressure Simulation ===

static void BM_Memory_Pressure_Simulation(benchmark::State& state) {
    SimpleAllocator allocator;
    std::vector<void*> long_lived_ptrs;
    std::vector<void*> short_lived_ptrs;
    
    // Pre-allocate some long-lived memory to simulate pressure
    for (int i = 0; i < 100; ++i) {
        void* ptr = allocator.allocate(8192);
        if (ptr) {
            long_lived_ptrs.push_back(ptr);
        }
    }
    
    for (auto _ : state) {
        // Allocate short-lived memory under pressure
        void* ptr = allocator.allocate(1024);
        if (ptr) {
            short_lived_ptrs.push_back(ptr);
            
            // Keep only recent allocations
            if (short_lived_ptrs.size() > 50) {
                allocator.deallocate(short_lived_ptrs.front());
                short_lived_ptrs.erase(short_lived_ptrs.begin());
            }
        }
    }
    
    // Cleanup
    for (void* ptr : long_lived_ptrs) {
        allocator.deallocate(ptr);
    }
    for (void* ptr : short_lived_ptrs) {
        allocator.deallocate(ptr);
    }
    allocator.cleanup();
}
BENCHMARK(BM_Memory_Pressure_Simulation);

// === Custom main for reporting ===
BENCHMARK_MAIN();
