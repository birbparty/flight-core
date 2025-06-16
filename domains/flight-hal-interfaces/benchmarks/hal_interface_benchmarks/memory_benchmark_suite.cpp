/**
 * @file memory_benchmark_suite.cpp
 * @brief Comprehensive Memory Interface Performance Benchmarks
 * 
 * Measures memory allocation performance, bandwidth utilization, and fragmentation
 * patterns across different allocator types and platform configurations.
 */

#include <benchmark/benchmark.h>
#include <memory>
#include <vector>
#include <random>
#include <chrono>
#include <cstring>

#include "../../include/flight/hal/interfaces/memory.hpp"
#include "../../examples/drivers/mock_memory_driver.hpp"

using namespace flight::hal;
using namespace flight::hal::examples;

/**
 * @brief Memory interface benchmark class
 */
class MemoryBenchmark {
public:
    MemoryBenchmark() {
        driver_ = std::make_unique<EliteMemoryDriver>();
        if (driver_) {
            auto init_result = driver_->initialize();
            if (!init_result.is_ok()) {
                throw std::runtime_error("Failed to initialize memory driver");
            }
        }
    }
    
    ~MemoryBenchmark() {
        cleanup_allocations();
        if (driver_) {
            driver_->shutdown();
        }
    }
    
    // === Benchmark Operations ===
    
    void perform_allocation_operation(size_t size = 1024) {
        auto result = driver_->allocate(size, MemoryAlignment::None);
        if (result.is_ok()) {
            allocated_blocks_.push_back({result.value().ptr, size});
        }
    }
    
    void perform_deallocation_operation() {
        if (!allocated_blocks_.empty()) {
            auto& block = allocated_blocks_.back();
            driver_->deallocate(block.ptr);
            allocated_blocks_.pop_back();
        }
    }
    
    void perform_reallocation_operation(size_t new_size = 2048) {
        if (!allocated_blocks_.empty()) {
            auto& block = allocated_blocks_.back();
            auto result = driver_->reallocate(block.ptr, new_size);
            if (result.is_ok()) {
                block.ptr = result.value().ptr;
                block.size = new_size;
            }
        }
    }
    
    void perform_memory_copy_operation(size_t size = 1024) {
        if (allocated_blocks_.size() >= 2) {
            auto& src = allocated_blocks_[0];
            auto& dst = allocated_blocks_[1];
            size_t copy_size = std::min({size, src.size, dst.size});
            std::memcpy(dst.ptr, src.ptr, copy_size);
        }
    }
    
    void perform_memory_set_operation(size_t size = 1024) {
        if (!allocated_blocks_.empty()) {
            auto& block = allocated_blocks_.front();
            size_t set_size = std::min(size, block.size);
            std::memset(block.ptr, 0xAA, set_size);
        }
    }
    
    void setup_benchmark() {
        cleanup_allocations();
        allocated_blocks_.reserve(1000);
        
        // Pre-allocate some blocks for operations that need existing memory
        for (int i = 0; i < 10; ++i) {
            perform_allocation_operation(1024 + i * 512);
        }
    }
    
    void cleanup_benchmark() {
        cleanup_allocations();
    }
    
    IMemoryInterface* get_interface() { return driver_.get(); }

private:
    struct AllocatedBlock {
        void* ptr;
        size_t size;
    };
    
    void cleanup_allocations() {
        for (auto& block : allocated_blocks_) {
            driver_->deallocate(block.ptr);
        }
        allocated_blocks_.clear();
    }
    
    std::unique_ptr<EliteMemoryDriver> driver_;
    std::vector<AllocatedBlock> allocated_blocks_;
};

// === Latency Benchmarks ===

static void BM_Memory_Allocation_Latency_Small(benchmark::State& state) {
    MemoryBenchmark bench;
    bench.setup_benchmark();
    
    for (auto _ : state) {
        bench.perform_allocation_operation(64);
    }
    
    bench.cleanup_benchmark();
}
BENCHMARK(BM_Memory_Allocation_Latency_Small);

static void BM_Memory_Allocation_Latency_Medium(benchmark::State& state) {
    MemoryBenchmark bench;
    bench.setup_benchmark();
    
    for (auto _ : state) {
        bench.perform_allocation_operation(1024);
    }
    
    bench.cleanup_benchmark();
}
BENCHMARK(BM_Memory_Allocation_Latency_Medium);

static void BM_Memory_Allocation_Latency_Large(benchmark::State& state) {
    MemoryBenchmark bench;
    bench.setup_benchmark();
    
    for (auto _ : state) {
        bench.perform_allocation_operation(65536);
    }
    
    bench.cleanup_benchmark();
}
BENCHMARK(BM_Memory_Allocation_Latency_Large);

static void BM_Memory_Deallocation_Latency(benchmark::State& state) {
    MemoryBenchmark bench;
    bench.setup_benchmark();
    
    for (auto _ : state) {
        bench.perform_deallocation_operation();
    }
    
    bench.cleanup_benchmark();
}
BENCHMARK(BM_Memory_Deallocation_Latency);

static void BM_Memory_Reallocation_Latency(benchmark::State& state) {
    MemoryBenchmark bench;
    bench.setup_benchmark();
    
    for (auto _ : state) {
        bench.perform_reallocation_operation();
    }
    
    bench.cleanup_benchmark();
}
BENCHMARK(BM_Memory_Reallocation_Latency);

// === Throughput Benchmarks ===

static void BM_Memory_Allocation_Throughput(benchmark::State& state) {
    MemoryBenchmark bench;
    bench.setup_benchmark();
    
    size_t alloc_size = static_cast<size_t>(state.range(0));
    for (auto _ : state) {
        bench.perform_allocation_operation(alloc_size);
    }
    
    state.SetItemsProcessed(state.iterations());
    bench.cleanup_benchmark();
}
BENCHMARK(BM_Memory_Allocation_Throughput)->Range(64, 8192);

static void BM_Memory_Copy_Throughput(benchmark::State& state) {
    MemoryBenchmark bench;
    bench.setup_benchmark();
    
    size_t copy_size = static_cast<size_t>(state.range(0));
    size_t total_bytes = 0;
    
    for (auto _ : state) {
        bench.perform_memory_copy_operation(copy_size);
        total_bytes += copy_size;
    }
    
    state.SetBytesProcessed(total_bytes);
    bench.cleanup_benchmark();
}
BENCHMARK(BM_Memory_Copy_Throughput)->Range(1024, 1048576); // 1KB to 1MB

static void BM_Memory_Set_Throughput(benchmark::State& state) {
    MemoryBenchmark bench;
    bench.setup_benchmark();
    
    size_t set_size = static_cast<size_t>(state.range(0));
    size_t total_bytes = 0;
    
    for (auto _ : state) {
        bench.perform_memory_set_operation(set_size);
        total_bytes += set_size;
    }
    
    state.SetBytesProcessed(total_bytes);
    bench.cleanup_benchmark();
}
BENCHMARK(BM_Memory_Set_Throughput)->Range(1024, 1048576); // 1KB to 1MB

// === Memory Fragmentation Benchmarks ===

static void BM_Memory_Fragmentation_Pattern(benchmark::State& state) {
    MemoryBenchmark bench;
    bench.setup_benchmark();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> size_dist(64, 4096);
    std::uniform_int_distribution<> action_dist(0, 2); // 0=alloc, 1=dealloc, 2=realloc
    
    for (auto _ : state) {
        int action = action_dist(gen);
        size_t size = size_dist(gen);
        
        switch (action) {
            case 0:
                bench.perform_allocation_operation(size);
                break;
            case 1:
                bench.perform_deallocation_operation();
                break;
            case 2:
                bench.perform_reallocation_operation(size);
                break;
        }
    }
    
    bench.cleanup_benchmark();
}
BENCHMARK(BM_Memory_Fragmentation_Pattern);

// === Memory Stress Benchmarks ===

static void BM_Memory_Allocation_Burst(benchmark::State& state) {
    MemoryBenchmark bench;
    bench.setup_benchmark();
    
    size_t burst_size = static_cast<size_t>(state.range(0));
    
    for (auto _ : state) {
        // Allocate burst_size blocks rapidly
        for (size_t i = 0; i < burst_size; ++i) {
            bench.perform_allocation_operation(1024);
        }
        
        // Then deallocate them all
        for (size_t i = 0; i < burst_size; ++i) {
            bench.perform_deallocation_operation();
        }
    }
    
    bench.cleanup_benchmark();
}
BENCHMARK(BM_Memory_Allocation_Burst)->Range(10, 1000);

// === Memory Statistics Benchmarks ===

static void BM_Memory_Statistics_Collection(benchmark::State& state) {
    MemoryBenchmark bench;
    bench.setup_benchmark();
    
    for (auto _ : state) {
        auto* interface = bench.get_interface();
        auto stats_result = interface->get_memory_stats();
        benchmark::DoNotOptimize(stats_result);
    }
    
    bench.cleanup_benchmark();
}
BENCHMARK(BM_Memory_Statistics_Collection);

// === Memory Type Benchmarks ===

static void BM_Memory_Typed_Allocation(benchmark::State& state) {
    MemoryBenchmark bench;
    bench.setup_benchmark();
    
    auto* interface = bench.get_interface();
    std::vector<void*> allocations;
    
    for (auto _ : state) {
        auto result = interface->allocate_typed(1024, MemoryType::System);
        if (result.is_ok()) {
            allocations.push_back(result.value().ptr);
        }
    }
    
    // Cleanup
    for (auto* ptr : allocations) {
        interface->deallocate(ptr);
    }
    
    bench.cleanup_benchmark();
}
BENCHMARK(BM_Memory_Typed_Allocation);

// === Allocator Performance Benchmarks ===

static void BM_Memory_Allocator_Performance(benchmark::State& state) {
    MemoryBenchmark bench;
    bench.setup_benchmark();
    
    auto* interface = bench.get_interface();
    AllocatorType alloc_type = static_cast<AllocatorType>(state.range(0));
    
    for (auto _ : state) {
        auto allocator_result = interface->get_allocator(alloc_type);
        if (allocator_result.is_ok()) {
            auto* allocator = allocator_result.value();
            benchmark::DoNotOptimize(allocator);
        }
    }
    
    bench.cleanup_benchmark();
}
BENCHMARK(BM_Memory_Allocator_Performance)
    ->Arg(static_cast<int>(AllocatorType::System))
    ->Arg(static_cast<int>(AllocatorType::Linear))
    ->Arg(static_cast<int>(AllocatorType::Pool));

// === Cross-Platform Memory Validation ===

static void BM_Memory_Cross_Platform_Validation(benchmark::State& state) {
    MemoryBenchmark bench;
    bench.setup_benchmark();
    
    // Simple platform-agnostic memory operations
    for (auto _ : state) {
        bench.perform_allocation_operation(1024);
        bench.perform_deallocation_operation();
    }
    
    bench.cleanup_benchmark();
}
BENCHMARK(BM_Memory_Cross_Platform_Validation);

// === Memory Alignment Benchmarks ===

static void BM_Memory_Alignment_Performance(benchmark::State& state) {
    MemoryBenchmark bench;
    bench.setup_benchmark();
    
    auto* interface = bench.get_interface();
    MemoryAlignment alignment = static_cast<MemoryAlignment>(state.range(0));
    std::vector<void*> allocations;
    
    for (auto _ : state) {
        auto result = interface->allocate(1024, alignment);
        if (result.is_ok()) {
            allocations.push_back(result.value().ptr);
        }
    }
    
    // Cleanup
    for (auto* ptr : allocations) {
        interface->deallocate(ptr);
    }
    
    bench.cleanup_benchmark();
}
BENCHMARK(BM_Memory_Alignment_Performance)
    ->Arg(static_cast<int>(MemoryAlignment::None))
    ->Arg(static_cast<int>(MemoryAlignment::Word))
    ->Arg(static_cast<int>(MemoryAlignment::Cache));

// === Memory Pressure Monitoring ===

static void BM_Memory_Pressure_Monitoring(benchmark::State& state) {
    MemoryBenchmark bench;
    bench.setup_benchmark();
    
    auto* interface = bench.get_interface();
    auto* pressure_monitor = interface->get_pressure_monitor();
    
    if (pressure_monitor) {
        for (auto _ : state) {
            auto pressure_info = pressure_monitor->get_pressure_info();
            benchmark::DoNotOptimize(pressure_info);
        }
    }
    
    bench.cleanup_benchmark();
}
BENCHMARK(BM_Memory_Pressure_Monitoring);
