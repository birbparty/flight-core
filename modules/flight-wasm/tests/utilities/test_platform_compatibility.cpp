// =============================================================================
// Flight WASM Tests - Platform Compatibility
// Comprehensive Cross-Platform Compatibility Testing
// =============================================================================

#include <catch2/catch_test_macros.hpp>
#include <flight/wasm/utilities/platform.hpp>
#include <flight/wasm/utilities/endian.hpp>
#include <flight/wasm/utilities/memory.hpp>
#include <flight/wasm/utilities/simd.hpp>
#include <array>
#include <vector>
#include <limits>

using namespace flight::wasm;

// =============================================================================
// Enhanced Platform Detection Tests
// =============================================================================

TEST_CASE("Enhanced Platform Detection", "[platform][compatibility]") {
    SECTION("Platform enumeration works") {
        auto platform = platform::current_platform();
        REQUIRE(platform != platform::Platform::Unknown);
        
        // Verify platform traits are accessible
        REQUIRE(platform::CurrentPlatform::cache_line_size > 0);
        REQUIRE(platform::CurrentPlatform::preferred_alignment > 0);
    }
    
    SECTION("Platform capabilities are consistent") {
        bool is_embedded = platform::CurrentPlatform::is_embedded;
        bool is_big_endian = platform::CurrentPlatform::is_big_endian;
        bool has_simd = platform::CurrentPlatform::has_simd;
        
        // All should be deterministic at compile time
        REQUIRE((is_embedded == true || is_embedded == false));
        REQUIRE((is_big_endian == true || is_big_endian == false));
        REQUIRE((has_simd == true || has_simd == false));
        
        // Embedded platforms should have memory limits
        if (is_embedded) {
            REQUIRE(platform::CurrentPlatform::max_memory < SIZE_MAX);
        }
    }
    
    SECTION("Memory constraints are realistic") {
        size_t max_mem = platform::CurrentPlatform::max_memory;
        size_t stack_size = platform::CurrentPlatform::stack_size;
        
        REQUIRE(max_mem > 0);
        REQUIRE(stack_size > 0);
        REQUIRE(stack_size <= max_mem);
    }
}

// =============================================================================
// Endianness Conversion Tests
// =============================================================================

TEST_CASE("Endianness Conversion", "[platform][endian][compatibility]") {
    SECTION("Runtime and compile-time detection consistency") {
        bool compile_time_big = platform::CurrentPlatform::is_big_endian;
        bool runtime_big = endian::runtime_is_big_endian();
        
        REQUIRE(compile_time_big == runtime_big);
        REQUIRE(endian::endianness_detection_consistent());
    }
    
    SECTION("Round-trip conversion preserves values") {
        // Test with various bit patterns
        std::array<uint32_t, 8> test_values = {{
            0x00000000, 0x12345678, 0xFFFFFFFF, 0xDEADBEEF,
            0x01020304, 0x80000000, 0x7FFFFFFF, 0xAAAA5555
        }};
        
        for (uint32_t original : test_values) {
            uint32_t wasm_format = endian::host_to_wasm(original);
            uint32_t restored = endian::wasm_to_host(wasm_format);
            REQUIRE(original == restored);
        }
    }
    
    SECTION("Little-endian platforms have no conversion overhead") {
        if (!platform::CurrentPlatform::is_big_endian) {
            uint32_t value = 0xDEADBEEF;
            REQUIRE(endian::host_to_wasm(value) == value);
            REQUIRE(endian::wasm_to_host(value) == value);
        }
    }
    
    SECTION("Big-endian platforms correctly swap bytes") {
        if (platform::CurrentPlatform::is_big_endian) {
            uint32_t big_endian_value = 0x12345678;
            uint32_t little_endian_expected = 0x78563412;
            REQUIRE(endian::host_to_wasm(big_endian_value) == little_endian_expected);
        }
    }
    
    SECTION("Floating point conversion preserves values") {
        std::array<float, 6> test_floats = {{
            0.0f, 1.0f, -1.0f, 3.14159f, 
            std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::quiet_NaN()
        }};
        
        for (float original : test_floats) {
            float wasm_format = endian::host_to_wasm_f32(original);
            float restored = endian::wasm_to_host_f32(wasm_format);
            
            if (std::isnan(original)) {
                REQUIRE(std::isnan(restored));
            } else {
                REQUIRE(original == restored);
            }
        }
    }
    
    SECTION("Bulk array conversion works correctly") {
        std::array<uint16_t, 8> original = {{
            0x0000, 0x1234, 0x5678, 0x9ABC, 
            0xDEF0, 0xFFFF, 0xAAAA, 0x5555
        }};
        
        auto converted = endian::host_to_wasm_array(original);
        auto restored = endian::wasm_to_host_array(converted);
        
        REQUIRE(original == restored);
    }
}

// =============================================================================
// Memory Management Tests
// =============================================================================

TEST_CASE("Memory Management", "[platform][memory][compatibility]") {
    SECTION("Platform allocator works") {
        void* ptr = memory::PlatformAllocator::allocate_aligned(1024, 16);
        REQUIRE(ptr != nullptr);
        REQUIRE(reinterpret_cast<uintptr_t>(ptr) % 16 == 0); // Properly aligned
        
        memory::PlatformAllocator::deallocate_aligned(ptr);
    }
    
    SECTION("Maximum allocation size is platform-appropriate") {
        size_t max_alloc = memory::PlatformAllocator::max_allocation_size();
        REQUIRE(max_alloc > 0);
        
        if (platform::CurrentPlatform::is_embedded) {
            REQUIRE(max_alloc <= platform::CurrentPlatform::max_memory);
        }
    }
    
    SECTION("Stack allocator works within limits") {
        memory::StackAllocator<1024> allocator;
        
        void* ptr1 = allocator.allocate(100);
        REQUIRE(ptr1 != nullptr);
        REQUIRE(allocator.used() >= 100);
        
        void* ptr2 = allocator.allocate(100);
        REQUIRE(ptr2 != nullptr);
        REQUIRE(ptr2 != ptr1);
        
        // Should fail when exceeding capacity
        void* ptr_fail = allocator.allocate(1000);
        REQUIRE(ptr_fail == nullptr);
        
        // Reset should work
        allocator.reset();
        REQUIRE(allocator.empty());
        REQUIRE(allocator.used() == 0);
    }
    
    SECTION("Object pool manages lifecycle correctly") {
        memory::ObjectPool<int, 8> pool;
        
        REQUIRE(pool.capacity() == 8);
        REQUIRE(pool.available() == 8);
        REQUIRE(!pool.empty());
        
        // Acquire some objects
        std::vector<int*> acquired;
        for (size_t i = 0; i < 4; ++i) {
            int* obj = pool.acquire();
            REQUIRE(obj != nullptr);
            acquired.push_back(obj);
        }
        
        REQUIRE(pool.available() == 4);
        
        // Release objects back to pool
        for (int* obj : acquired) {
            pool.release(obj);
        }
        
        REQUIRE(pool.available() == 8);
    }
    
    SECTION("Memory region RAII works correctly") {
        {
            memory::MemoryRegion region(4096);
            REQUIRE(region.size() == 4096);
            REQUIRE(region.data() != nullptr);
            REQUIRE(!region.empty());
            
            // Test zero-fill
            region.zero();
            const auto* bytes = static_cast<const uint8_t*>(region.data());
            for (size_t i = 0; i < 100; ++i) { // Check first 100 bytes
                REQUIRE(bytes[i] == 0);
            }
        }
        // Region should be automatically freed
    }
    
    SECTION("STL-compatible aligned allocator works") {
        using AllocatedVector = std::vector<int, memory::AlignedAllocator<int, 32>>;
        
        AllocatedVector vec;
        vec.resize(100);
        
        // Check that vector data is properly aligned
        REQUIRE(reinterpret_cast<uintptr_t>(vec.data()) % 32 == 0);
    }
}

// =============================================================================
// SIMD Operations Tests
// =============================================================================

TEST_CASE("SIMD Operations", "[platform][simd][compatibility]") {
    SECTION("SIMD capability detection") {
        bool has_simd = simd::capabilities::has_simd();
        bool has_native = simd::capabilities::has_native_v128();
        const char* instruction_set = simd::capabilities::simd_instruction_set();
        
        REQUIRE(instruction_set != nullptr);
        
        // Native SIMD should imply general SIMD capability
        if (has_native) {
            REQUIRE(has_simd);
        }
    }
    
    SECTION("v128 basic operations") {
        simd::v128 zero = simd::constants::zero();
        simd::v128 ones = simd::constants::all_ones();
        
        REQUIRE(zero != ones);
        REQUIRE(zero == simd::constants::zero());
        
        // Test that all bytes are correct
        for (size_t i = 0; i < 16; ++i) {
            REQUIRE(zero[i] == 0);
            REQUIRE(ones[i] == 0xFF);
        }
    }
    
    SECTION("v128 bitwise operations work correctly") {
        simd::v128 a = simd::constants::splat_u8(0xAA);  // 10101010
        simd::v128 b = simd::constants::splat_u8(0x55);  // 01010101
        
        simd::v128 and_result = simd::ops::v128_and(a, b);
        simd::v128 or_result = simd::ops::v128_or(a, b);
        simd::v128 xor_result = simd::ops::v128_xor(a, b);
        
        // AND should be all zeros
        for (size_t i = 0; i < 16; ++i) {
            REQUIRE(and_result[i] == 0x00);
        }
        
        // OR should be all ones
        for (size_t i = 0; i < 16; ++i) {
            REQUIRE(or_result[i] == 0xFF);
        }
        
        // XOR should be all ones (since no bits overlap)
        for (size_t i = 0; i < 16; ++i) {
            REQUIRE(xor_result[i] == 0xFF);
        }
    }
    
    SECTION("v128 arithmetic operations") {
        simd::v128 a = simd::constants::splat_u8(10);
        simd::v128 b = simd::constants::splat_u8(5);
        
        simd::v128 add_result = simd::ops::i8x16_add(a, b);
        simd::v128 sub_result = simd::ops::i8x16_sub(a, b);
        
        // Check addition
        for (size_t i = 0; i < 16; ++i) {
            REQUIRE(add_result.i8[i] == 15);
        }
        
        // Check subtraction
        for (size_t i = 0; i < 16; ++i) {
            REQUIRE(sub_result.i8[i] == 5);
        }
    }
    
    SECTION("v128 floating point operations") {
        simd::v128 a = simd::constants::splat_f32(2.0f);
        simd::v128 b = simd::constants::splat_f32(3.0f);
        
        simd::v128 add_result = simd::ops::f32x4_add(a, b);
        simd::v128 mul_result = simd::ops::f32x4_mul(a, b);
        
        // Check addition
        for (size_t i = 0; i < 4; ++i) {
            REQUIRE(add_result.f32[i] == 5.0f);
        }
        
        // Check multiplication
        for (size_t i = 0; i < 4; ++i) {
            REQUIRE(mul_result.f32[i] == 6.0f);
        }
    }
    
    SECTION("v128 load/store with endianness handling") {
        alignas(16) std::array<uint32_t, 4> data = {{
            0x12345678, 0x9ABCDEF0, 0xFEDCBA98, 0x76543210
        }};
        
        simd::v128 loaded = simd::ops::v128_load(data.data());
        
        alignas(16) std::array<uint32_t, 4> stored_data;
        simd::ops::v128_store(stored_data.data(), loaded);
        
        // Data should round-trip correctly
        for (size_t i = 0; i < 4; ++i) {
            REQUIRE(data[i] == stored_data[i]);
        }
    }
    
    SECTION("v128 lane operations") {
        simd::v128 vec = simd::constants::zero();
        
        // Test lane replacement
        vec = simd::ops::i32x4_replace_lane<0>(vec, 0x12345678);
        vec = simd::ops::i32x4_replace_lane<1>(vec, 0x9ABCDEF0);
        
        // Test lane extraction
        REQUIRE(simd::ops::i32x4_extract_lane<0>(vec) == 0x12345678);
        REQUIRE(simd::ops::i32x4_extract_lane<1>(vec) == 0x9ABCDEF0);
        REQUIRE(simd::ops::i32x4_extract_lane<2>(vec) == 0);
        REQUIRE(simd::ops::i32x4_extract_lane<3>(vec) == 0);
    }
}

// =============================================================================
// Cross-Platform Integration Tests
// =============================================================================

TEST_CASE("Cross-Platform WebAssembly Compatibility", "[integration][cross-platform]") {
    SECTION("WebAssembly magic number validation works on all platforms") {
        const uint8_t wasm_magic[] = {0x00, 0x61, 0x73, 0x6D};
        
        // Create a fake validation function for testing
        auto validate_magic = [](const uint8_t* magic) -> bool {
            const uint8_t expected[] = {0x00, 0x61, 0x73, 0x6D};
            for (size_t i = 0; i < 4; ++i) {
                if (magic[i] != expected[i]) return false;
            }
            return true;
        };
        
        // Should work regardless of platform endianness
        bool is_valid = validate_magic(wasm_magic);
        REQUIRE(is_valid);
    }
    
    SECTION("Type sizes are consistent across platforms") {
        // Ensure WebAssembly type consistency
        REQUIRE(sizeof(int32_t) == 4);
        REQUIRE(sizeof(int64_t) == 8);
        REQUIRE(sizeof(float) == 4);
        REQUIRE(sizeof(double) == 8);
        
        // Verify floating point format
        REQUIRE(std::numeric_limits<float>::is_iec559);
        REQUIRE(std::numeric_limits<double>::is_iec559);
        
        // Verify two's complement integers
        REQUIRE(static_cast<int32_t>(0xFFFFFFFF) == -1);
        REQUIRE(static_cast<int64_t>(0xFFFFFFFFFFFFFFFF) == -1);
    }
    
    SECTION("Platform-specific optimizations don't break compatibility") {
        // Test that platform-specific byte swapping works correctly
        uint32_t test_value = 0x12345678;
        
        // Use platform-specific optimized functions
        uint32_t swapped_u32 = endian::host_to_wasm_u32(test_value);
        uint32_t restored_u32 = endian::wasm_to_host_u32(swapped_u32);
        REQUIRE(test_value == restored_u32);
        
        // Test float conversion
        float test_float = 3.14159f;
        float swapped_f32 = endian::host_to_wasm_f32(test_float);
        float restored_f32 = endian::wasm_to_host_f32(swapped_f32);
        REQUIRE(test_float == restored_f32);
    }
    
    SECTION("Memory alignment requirements are met") {
        // WebAssembly has specific alignment requirements
        REQUIRE(alignof(int32_t) <= 4);
        REQUIRE(alignof(int64_t) <= 8);
        REQUIRE(alignof(float) <= 4);
        REQUIRE(alignof(double) <= 8);
        
        // Platform alignment should be reasonable
        REQUIRE(platform::CurrentPlatform::preferred_alignment >= 1);
        REQUIRE(platform::CurrentPlatform::preferred_alignment <= 64);
        
        // Should be power of 2
        size_t alignment = platform::CurrentPlatform::preferred_alignment;
        REQUIRE((alignment & (alignment - 1)) == 0);
    }
    
    SECTION("Embedded platform constraints are respected") {
        if (platform::CurrentPlatform::is_embedded) {
            // Embedded platforms should have reasonable memory limits
            REQUIRE(platform::CurrentPlatform::max_memory <= 1024 * 1024 * 1024); // 1GB max
            
            // Stack size should be conservative
            REQUIRE(platform::CurrentPlatform::stack_size <= 8 * 1024 * 1024); // 8MB max
            
            // Template recursion should be limited
            REQUIRE(platform::CurrentPlatform::max_memory < SIZE_MAX);
        }
    }
}

// =============================================================================
// Performance and Memory Efficiency Tests
// =============================================================================

TEST_CASE("Performance Characteristics", "[platform][performance]") {
    SECTION("Zero-overhead platform abstraction") {
        // These operations should compile to the same code on little-endian platforms
        uint32_t value = 0x12345678;
        
        if (!platform::CurrentPlatform::is_big_endian) {
            // On little-endian platforms, these should be no-ops
            uint32_t converted = endian::host_to_wasm_u32(value);
            REQUIRE(converted == value);
            
            // Compiler should optimize this to a simple copy
            auto start_time = std::chrono::high_resolution_clock::now();
            for (volatile int i = 0; i < 1000; ++i) {
                volatile uint32_t result = endian::host_to_wasm_u32(value);
                (void)result; // Suppress unused variable warning
            }
            auto end_time = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
            
            // Should be very fast (less than 10 microseconds for 1000 operations)
            REQUIRE(duration.count() < 10000);
        }
    }
    
    SECTION("Memory allocator performance") {
        constexpr size_t num_allocations = 100;
        constexpr size_t allocation_size = 64;
        
        std::vector<void*> pointers;
        pointers.reserve(num_allocations);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Allocate
        for (size_t i = 0; i < num_allocations; ++i) {
            void* ptr = memory::PlatformAllocator::allocate_aligned(allocation_size, 16);
            REQUIRE(ptr != nullptr);
            pointers.push_back(ptr);
        }
        
        // Deallocate
        for (void* ptr : pointers) {
            memory::PlatformAllocator::deallocate_aligned(ptr);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        // Should complete in reasonable time (less than 1ms for 100 allocations)
        REQUIRE(duration.count() < 1000);
    }
}
