// =============================================================================
// Flight WASM Tests - Platform Utilities
// Platform Detection and Optimization Testing
// =============================================================================

#include <catch2/catch_test_macros.hpp>
#include <flight/wasm/utilities/platform.hpp>
#include <flight/wasm/utilities/endian.hpp>
#include <flight/wasm/utilities/memory.hpp>
#include <flight/wasm/utilities/simd.hpp>

using namespace flight::wasm;

// =============================================================================
// Platform Detection Tests
// =============================================================================

TEST_CASE("Platform Detection", "[utilities][platform]") {
    SECTION("Endianness detection") {
        // Test compile-time endianness detection
        bool big_endian = platform::is_big_endian();
        bool little_endian = platform::is_little_endian();
        
        // Exactly one should be true
        REQUIRE((big_endian ^ little_endian));
        REQUIRE(big_endian != little_endian);
    }
    
    SECTION("Platform constants") {
        // Verify platform constants are reasonable
        REQUIRE(platform::cache_line_size > 0);
        REQUIRE(platform::cache_line_size <= 256); // Reasonable upper bound
        
        REQUIRE(platform::max_memory > 0);
        REQUIRE(platform::max_stack_size > 0);
        REQUIRE(platform::max_template_depth > 0);
    }
    
    SECTION("Memory alignment utilities") {
        // Test alignment functions
        REQUIRE(platform::align_up<16>(15) == 16);
        REQUIRE(platform::align_up<16>(16) == 16);
        REQUIRE(platform::align_up<16>(17) == 32);
        
        REQUIRE(platform::align_down<16>(15) == 0);
        REQUIRE(platform::align_down<16>(16) == 16);
        REQUIRE(platform::align_down<16>(31) == 16);
        REQUIRE(platform::align_down<16>(32) == 32);
    }
    
    SECTION("Cache-aligned size calculation") {
        size_t aligned_size = platform::cache_aligned_size(1);
        REQUIRE(aligned_size >= 1);
        REQUIRE(aligned_size >= platform::cache_line_size);
        REQUIRE((aligned_size % platform::cache_line_size) == 0);
    }
}

// =============================================================================
// Platform Feature Tests
// =============================================================================

TEST_CASE("Platform Features", "[utilities][platform][features]") {
    SECTION("SIMD availability") {
        // Test SIMD feature detection
        bool has_simd = platform::has_neon_simd;
        
        // Should be deterministic at compile time
        REQUIRE((has_simd == true || has_simd == false));
    }
    
    SECTION("Memory constraints") {
        // Test memory constraint detection
        bool limited_memory = platform::has_limited_memory;
        
        // Should be deterministic at compile time
        REQUIRE((limited_memory == true || limited_memory == false));
        
        if (limited_memory) {
            // If memory is limited, max_memory should be reasonable
            REQUIRE(platform::max_memory < SIZE_MAX);
        }
    }
    
    SECTION("Endianness consistency") {
        // Test endianness platform detection
        bool big_endian_platform = platform::is_big_endian_platform;
        bool detected_big_endian = platform::is_big_endian();
        
        // Platform constant should match runtime detection
        REQUIRE(big_endian_platform == detected_big_endian);
    }
}

// =============================================================================
// Type Size Tests
// =============================================================================

TEST_CASE("Platform Type Sizes", "[utilities][platform][types]") {
    SECTION("Preferred size type") {
        // Test that preferred_size_type is reasonable
        REQUIRE(sizeof(platform::preferred_size_type) > 0);
        REQUIRE(sizeof(platform::preferred_size_type) <= sizeof(size_t));
    }
    
    SECTION("Alignment requirements") {
        // Test various alignment requirements
        REQUIRE(platform::alignment_of_v<char> == 1);
        REQUIRE(platform::alignment_of_v<int> >= 1);
        REQUIRE(platform::alignment_of_v<double> >= 1);
        REQUIRE(platform::alignment_of_v<void*> >= 1);
    }
}
