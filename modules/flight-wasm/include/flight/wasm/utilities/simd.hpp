#ifndef FLIGHT_WASM_UTILITIES_SIMD_HPP
#define FLIGHT_WASM_UTILITIES_SIMD_HPP

/**
 * @file simd.hpp
 * @brief Cross-platform SIMD abstraction for WebAssembly v128 operations
 * 
 * This header provides a unified interface for SIMD operations across all
 * platforms, with optimized implementations for platforms that support SIMD
 * and fallback implementations for those that don't.
 */

#include <flight/wasm/utilities/platform.hpp>
#include <flight/wasm/utilities/endian.hpp>
#include <array>
#include <cstring>
#include <cstdint>

// Platform-specific SIMD includes
#if defined(__vita__) && defined(__ARM_NEON)
    #include <arm_neon.h>
#elif defined(__EMSCRIPTEN__) && defined(__wasm_simd128__)
    #include <wasm_simd128.h>
#elif defined(__SSE2__) && (defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86))
    #include <emmintrin.h>
    #include <immintrin.h>
#endif

namespace flight::wasm::simd {

    // WebAssembly v128 SIMD type abstraction
    struct alignas(16) v128 {
        // Union for different interpretations of the 128-bit data
        union {
            std::array<uint8_t, 16> u8;
            std::array<int8_t, 16> i8;
            std::array<uint16_t, 8> u16;
            std::array<int16_t, 8> i16;
            std::array<uint32_t, 4> u32;
            std::array<int32_t, 4> i32;
            std::array<uint64_t, 2> u64;
            std::array<int64_t, 2> i64;
            std::array<float, 4> f32;
            std::array<double, 2> f64;
            
            // Platform-specific native types for optimization
            #if defined(__vita__) && defined(__ARM_NEON)
                uint8x16_t neon_u8;
                int8x16_t neon_i8;
                uint16x8_t neon_u16;
                int16x8_t neon_i16;
                uint32x4_t neon_u32;
                int32x4_t neon_i32;
                uint64x2_t neon_u64;
                int64x2_t neon_i64;
                float32x4_t neon_f32;
                float64x2_t neon_f64;
            #elif defined(__EMSCRIPTEN__) && defined(__wasm_simd128__)
                v128_t wasm_v128;
            #elif defined(__SSE2__)
                __m128i sse_i;
                __m128 sse_f;
                __m128d sse_d;
            #endif
        };
        
        // Default constructor - zero initialize
        v128() noexcept {
            u64[0] = 0;
            u64[1] = 0;
        }
        
        // Constructor from bytes
        explicit v128(const std::array<uint8_t, 16>& bytes) noexcept : u8(bytes) {}
        
        // Constructor from platform-specific types
        #if defined(__vita__) && defined(__ARM_NEON)
        explicit v128(uint8x16_t neon_vec) noexcept : neon_u8(neon_vec) {}
        explicit v128(int8x16_t neon_vec) noexcept : neon_i8(neon_vec) {}
        explicit v128(uint16x8_t neon_vec) noexcept : neon_u16(neon_vec) {}
        explicit v128(int16x8_t neon_vec) noexcept : neon_i16(neon_vec) {}
        explicit v128(uint32x4_t neon_vec) noexcept : neon_u32(neon_vec) {}
        explicit v128(int32x4_t neon_vec) noexcept : neon_i32(neon_vec) {}
        explicit v128(float32x4_t neon_vec) noexcept : neon_f32(neon_vec) {}
        #elif defined(__EMSCRIPTEN__) && defined(__wasm_simd128__)
        explicit v128(v128_t wasm_vec) noexcept : wasm_v128(wasm_vec) {}
        #elif defined(__SSE2__)
        explicit v128(__m128i sse_vec) noexcept : sse_i(sse_vec) {}
        explicit v128(__m128 sse_vec) noexcept : sse_f(sse_vec) {}
        explicit v128(__m128d sse_vec) noexcept : sse_d(sse_vec) {}
        #endif
        
        // Conversion operators to platform-specific types
        #if defined(__vita__) && defined(__ARM_NEON)
        operator uint8x16_t() const noexcept { return neon_u8; }
        operator int8x16_t() const noexcept { return neon_i8; }
        operator uint16x8_t() const noexcept { return neon_u16; }
        operator int16x8_t() const noexcept { return neon_i16; }
        operator uint32x4_t() const noexcept { return neon_u32; }
        operator int32x4_t() const noexcept { return neon_i32; }
        operator float32x4_t() const noexcept { return neon_f32; }
        #elif defined(__EMSCRIPTEN__) && defined(__wasm_simd128__)
        operator v128_t() const noexcept { return wasm_v128; }
        #elif defined(__SSE2__)
        operator __m128i() const noexcept { return sse_i; }
        operator __m128() const noexcept { return sse_f; }
        operator __m128d() const noexcept { return sse_d; }
        #endif
        
        // Element access
        uint8_t& operator[](size_t index) noexcept {
            return u8[index];
        }
        
        const uint8_t& operator[](size_t index) const noexcept {
            return u8[index];
        }
        
        // Comparison operators
        bool operator==(const v128& other) const noexcept {
            return u64[0] == other.u64[0] && u64[1] == other.u64[1];
        }
        
        bool operator!=(const v128& other) const noexcept {
            return !(*this == other);
        }
    };

    // Static factory functions for common values
    namespace constants {
        inline v128 zero() noexcept {
            return v128{};
        }
        
        inline v128 all_ones() noexcept {
            v128 result;
            result.u64[0] = UINT64_MAX;
            result.u64[1] = UINT64_MAX;
            return result;
        }
        
        inline v128 splat_u8(uint8_t value) noexcept {
            v128 result;
            for (size_t i = 0; i < 16; ++i) {
                result.u8[i] = value;
            }
            return result;
        }
        
        inline v128 splat_u16(uint16_t value) noexcept {
            v128 result;
            for (size_t i = 0; i < 8; ++i) {
                result.u16[i] = value;
            }
            return result;
        }
        
        inline v128 splat_u32(uint32_t value) noexcept {
            v128 result;
            for (size_t i = 0; i < 4; ++i) {
                result.u32[i] = value;
            }
            return result;
        }
        
        inline v128 splat_f32(float value) noexcept {
            v128 result;
            for (size_t i = 0; i < 4; ++i) {
                result.f32[i] = value;
            }
            return result;
        }
    }

    // SIMD operations with platform-specific optimizations
    namespace ops {
        
        // Bitwise operations
        inline v128 v128_and(const v128& a, const v128& b) noexcept {
            #if defined(__vita__) && defined(__ARM_NEON)
                return v128(vandq_u8(a.neon_u8, b.neon_u8));
            #elif defined(__EMSCRIPTEN__) && defined(__wasm_simd128__)
                return v128(wasm_v128_and(a.wasm_v128, b.wasm_v128));
            #elif defined(__SSE2__)
                return v128(_mm_and_si128(a.sse_i, b.sse_i));
            #else
                v128 result;
                result.u64[0] = a.u64[0] & b.u64[0];
                result.u64[1] = a.u64[1] & b.u64[1];
                return result;
            #endif
        }
        
        inline v128 v128_or(const v128& a, const v128& b) noexcept {
            #if defined(__vita__) && defined(__ARM_NEON)
                return v128(vorrq_u8(a.neon_u8, b.neon_u8));
            #elif defined(__EMSCRIPTEN__) && defined(__wasm_simd128__)
                return v128(wasm_v128_or(a.wasm_v128, b.wasm_v128));
            #elif defined(__SSE2__)
                return v128(_mm_or_si128(a.sse_i, b.sse_i));
            #else
                v128 result;
                result.u64[0] = a.u64[0] | b.u64[0];
                result.u64[1] = a.u64[1] | b.u64[1];
                return result;
            #endif
        }
        
        inline v128 v128_xor(const v128& a, const v128& b) noexcept {
            #if defined(__vita__) && defined(__ARM_NEON)
                return v128(veorq_u8(a.neon_u8, b.neon_u8));
            #elif defined(__EMSCRIPTEN__) && defined(__wasm_simd128__)
                return v128(wasm_v128_xor(a.wasm_v128, b.wasm_v128));
            #elif defined(__SSE2__)
                return v128(_mm_xor_si128(a.sse_i, b.sse_i));
            #else
                v128 result;
                result.u64[0] = a.u64[0] ^ b.u64[0];
                result.u64[1] = a.u64[1] ^ b.u64[1];
                return result;
            #endif
        }
        
        inline v128 v128_not(const v128& a) noexcept {
            #if defined(__vita__) && defined(__ARM_NEON)
                return v128(vmvnq_u8(a.neon_u8));
            #elif defined(__EMSCRIPTEN__) && defined(__wasm_simd128__)
                return v128(wasm_v128_not(a.wasm_v128));
            #elif defined(__SSE2__)
                return v128(_mm_xor_si128(a.sse_i, _mm_set1_epi32(-1)));
            #else
                v128 result;
                result.u64[0] = ~a.u64[0];
                result.u64[1] = ~a.u64[1];
                return result;
            #endif
        }
        
        // 8-bit integer operations
        inline v128 i8x16_add(const v128& a, const v128& b) noexcept {
            #if defined(__vita__) && defined(__ARM_NEON)
                return v128(vaddq_s8(a.neon_i8, b.neon_i8));
            #elif defined(__EMSCRIPTEN__) && defined(__wasm_simd128__)
                return v128(wasm_i8x16_add(a.wasm_v128, b.wasm_v128));
            #elif defined(__SSE2__)
                return v128(_mm_add_epi8(a.sse_i, b.sse_i));
            #else
                v128 result;
                for (size_t i = 0; i < 16; ++i) {
                    result.i8[i] = a.i8[i] + b.i8[i];
                }
                return result;
            #endif
        }
        
        inline v128 i8x16_sub(const v128& a, const v128& b) noexcept {
            #if defined(__vita__) && defined(__ARM_NEON)
                return v128(vsubq_s8(a.neon_i8, b.neon_i8));
            #elif defined(__EMSCRIPTEN__) && defined(__wasm_simd128__)
                return v128(wasm_i8x16_sub(a.wasm_v128, b.wasm_v128));
            #elif defined(__SSE2__)
                return v128(_mm_sub_epi8(a.sse_i, b.sse_i));
            #else
                v128 result;
                for (size_t i = 0; i < 16; ++i) {
                    result.i8[i] = a.i8[i] - b.i8[i];
                }
                return result;
            #endif
        }
        
        // 16-bit integer operations
        inline v128 i16x8_add(const v128& a, const v128& b) noexcept {
            #if defined(__vita__) && defined(__ARM_NEON)
                return v128(vaddq_s16(a.neon_i16, b.neon_i16));
            #elif defined(__EMSCRIPTEN__) && defined(__wasm_simd128__)
                return v128(wasm_i16x8_add(a.wasm_v128, b.wasm_v128));
            #elif defined(__SSE2__)
                return v128(_mm_add_epi16(a.sse_i, b.sse_i));
            #else
                v128 result;
                for (size_t i = 0; i < 8; ++i) {
                    result.i16[i] = a.i16[i] + b.i16[i];
                }
                return result;
            #endif
        }
        
        inline v128 i16x8_mul(const v128& a, const v128& b) noexcept {
            #if defined(__vita__) && defined(__ARM_NEON)
                return v128(vmulq_s16(a.neon_i16, b.neon_i16));
            #elif defined(__EMSCRIPTEN__) && defined(__wasm_simd128__)
                return v128(wasm_i16x8_mul(a.wasm_v128, b.wasm_v128));
            #elif defined(__SSE2__)
                return v128(_mm_mullo_epi16(a.sse_i, b.sse_i));
            #else
                v128 result;
                for (size_t i = 0; i < 8; ++i) {
                    result.i16[i] = a.i16[i] * b.i16[i];
                }
                return result;
            #endif
        }
        
        // 32-bit integer operations
        inline v128 i32x4_add(const v128& a, const v128& b) noexcept {
            #if defined(__vita__) && defined(__ARM_NEON)
                return v128(vaddq_s32(a.neon_i32, b.neon_i32));
            #elif defined(__EMSCRIPTEN__) && defined(__wasm_simd128__)
                return v128(wasm_i32x4_add(a.wasm_v128, b.wasm_v128));
            #elif defined(__SSE2__)
                return v128(_mm_add_epi32(a.sse_i, b.sse_i));
            #else
                v128 result;
                for (size_t i = 0; i < 4; ++i) {
                    result.i32[i] = a.i32[i] + b.i32[i];
                }
                return result;
            #endif
        }
        
        inline v128 i32x4_mul(const v128& a, const v128& b) noexcept {
            #if defined(__vita__) && defined(__ARM_NEON)
                return v128(vmulq_s32(a.neon_i32, b.neon_i32));
            #elif defined(__EMSCRIPTEN__) && defined(__wasm_simd128__)
                return v128(wasm_i32x4_mul(a.wasm_v128, b.wasm_v128));
            #elif defined(__SSE4_1__)
                return v128(_mm_mullo_epi32(a.sse_i, b.sse_i));
            #elif defined(__SSE2__)
                // Emulate 32-bit multiply with SSE2
                __m128i tmp1 = _mm_mul_epu32(a.sse_i, b.sse_i);
                __m128i tmp2 = _mm_mul_epu32(_mm_srli_si128(a.sse_i, 4), _mm_srli_si128(b.sse_i, 4));
                return v128(_mm_unpacklo_epi32(_mm_shuffle_epi32(tmp1, _MM_SHUFFLE(0,0,2,0)), 
                                             _mm_shuffle_epi32(tmp2, _MM_SHUFFLE(0,0,2,0))));
            #else
                v128 result;
                for (size_t i = 0; i < 4; ++i) {
                    result.i32[i] = a.i32[i] * b.i32[i];
                }
                return result;
            #endif
        }
        
        // 32-bit float operations
        inline v128 f32x4_add(const v128& a, const v128& b) noexcept {
            #if defined(__vita__) && defined(__ARM_NEON)
                return v128(vaddq_f32(a.neon_f32, b.neon_f32));
            #elif defined(__EMSCRIPTEN__) && defined(__wasm_simd128__)
                return v128(wasm_f32x4_add(a.wasm_v128, b.wasm_v128));
            #elif defined(__SSE__)
                return v128(_mm_add_ps(a.sse_f, b.sse_f));
            #else
                v128 result;
                for (size_t i = 0; i < 4; ++i) {
                    result.f32[i] = a.f32[i] + b.f32[i];
                }
                return result;
            #endif
        }
        
        inline v128 f32x4_mul(const v128& a, const v128& b) noexcept {
            #if defined(__vita__) && defined(__ARM_NEON)
                return v128(vmulq_f32(a.neon_f32, b.neon_f32));
            #elif defined(__EMSCRIPTEN__) && defined(__wasm_simd128__)
                return v128(wasm_f32x4_mul(a.wasm_v128, b.wasm_v128));
            #elif defined(__SSE__)
                return v128(_mm_mul_ps(a.sse_f, b.sse_f));
            #else
                v128 result;
                for (size_t i = 0; i < 4; ++i) {
                    result.f32[i] = a.f32[i] * b.f32[i];
                }
                return result;
            #endif
        }
        
        // Load/Store operations with endianness handling
        inline v128 v128_load(const void* ptr) noexcept {
            v128 result;
            std::memcpy(&result, ptr, 16);
            
            // Handle endianness conversion if needed
            if constexpr (platform::CurrentPlatform::is_big_endian) {
                // Convert from little-endian WebAssembly format to host format
                for (size_t i = 0; i < 4; ++i) {
                    result.u32[i] = endian::wasm_to_host_u32(result.u32[i]);
                }
            }
            
            return result;
        }
        
        inline void v128_store(void* ptr, const v128& value) noexcept {
            v128 store_value = value;
            
            // Handle endianness conversion if needed
            if constexpr (platform::CurrentPlatform::is_big_endian) {
                // Convert from host format to little-endian WebAssembly format
                for (size_t i = 0; i < 4; ++i) {
                    store_value.u32[i] = endian::host_to_wasm_u32(store_value.u32[i]);
                }
            }
            
            std::memcpy(ptr, &store_value, 16);
        }
        
        // Lane extraction and replacement
        template<int Lane>
        inline uint8_t i8x16_extract_lane(const v128& a) noexcept {
            static_assert(Lane >= 0 && Lane < 16, "Lane index out of range");
            return a.u8[Lane];
        }
        
        template<int Lane>
        inline v128 i8x16_replace_lane(const v128& a, uint8_t value) noexcept {
            static_assert(Lane >= 0 && Lane < 16, "Lane index out of range");
            v128 result = a;
            result.u8[Lane] = value;
            return result;
        }
        
        template<int Lane>
        inline uint32_t i32x4_extract_lane(const v128& a) noexcept {
            static_assert(Lane >= 0 && Lane < 4, "Lane index out of range");
            return a.u32[Lane];
        }
        
        template<int Lane>
        inline v128 i32x4_replace_lane(const v128& a, uint32_t value) noexcept {
            static_assert(Lane >= 0 && Lane < 4, "Lane index out of range");
            v128 result = a;
            result.u32[Lane] = value;
            return result;
        }
        
        template<int Lane>
        inline float f32x4_extract_lane(const v128& a) noexcept {
            static_assert(Lane >= 0 && Lane < 4, "Lane index out of range");
            return a.f32[Lane];
        }
        
        template<int Lane>
        inline v128 f32x4_replace_lane(const v128& a, float value) noexcept {
            static_assert(Lane >= 0 && Lane < 4, "Lane index out of range");
            v128 result = a;
            result.f32[Lane] = value;
            return result;
        }
    }

    // Utility functions for SIMD capability detection
    namespace capabilities {
        constexpr bool has_simd() noexcept {
            return platform::CurrentPlatform::has_simd;
        }
        
        constexpr bool has_native_v128() noexcept {
            #if defined(__vita__) && defined(__ARM_NEON)
                return true;
            #elif defined(__EMSCRIPTEN__) && defined(__wasm_simd128__)
                return true;
            #elif defined(__SSE2__)
                return true;
            #else
                return false;
            #endif
        }
        
        constexpr const char* simd_instruction_set() noexcept {
            #if defined(__vita__) && defined(__ARM_NEON)
                return "ARM NEON";
            #elif defined(__EMSCRIPTEN__) && defined(__wasm_simd128__)
                return "WebAssembly SIMD128";
            #elif defined(__AVX2__)
                return "x86 AVX2";
            #elif defined(__AVX__)
                return "x86 AVX";
            #elif defined(__SSE4_2__)
                return "x86 SSE4.2";
            #elif defined(__SSE4_1__)
                return "x86 SSE4.1";
            #elif defined(__SSSE3__)
                return "x86 SSSE3";
            #elif defined(__SSE3__)
                return "x86 SSE3";
            #elif defined(__SSE2__)
                return "x86 SSE2";
            #elif defined(__SSE__)
                return "x86 SSE";
            #else
                return "None (scalar fallback)";
            #endif
        }
    }

} // namespace flight::wasm::simd

#endif // FLIGHT_WASM_UTILITIES_SIMD_HPP
