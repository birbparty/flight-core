#ifndef FLIGHT_WASM_UTILITIES_ENDIAN_HPP
#define FLIGHT_WASM_UTILITIES_ENDIAN_HPP

/**
 * @file endian.hpp
 * @brief Endianness conversion utilities for WebAssembly compliance
 * 
 * WebAssembly uses little-endian byte order. This header provides efficient
 * conversion utilities between host byte order and WebAssembly byte order,
 * with platform-specific optimizations.
 */

#include <flight/wasm/utilities/platform.hpp>
#include <cstring>
#include <array>

namespace flight::wasm::endian {

    // WebAssembly requires little-endian byte order
    // These functions convert between host and WebAssembly byte order
    
    template<typename T>
    constexpr T host_to_wasm(T value) noexcept {
        if constexpr (platform::CurrentPlatform::is_big_endian) {
            return platform::byteswap(value);
        } else {
            return value; // Already little-endian
        }
    }
    
    template<typename T>
    constexpr T wasm_to_host(T value) noexcept {
        return host_to_wasm(value); // Symmetric operation
    }

    // Platform-optimized byte order conversion for common types
    namespace detail {
        // SH-4 specific optimizations for Dreamcast
        #ifdef __DREAMCAST__
        inline uint16_t sh4_byteswap16(uint16_t value) noexcept {
            // Use SH-4 SWAP.B instruction if available
            #ifdef __SH4_SINGLE_ONLY__
                uint16_t result;
                asm("swap.b %1, %0" : "=r"(result) : "r"(value));
                return result;
            #else
                return platform::byteswap(value);
            #endif
        }
        
        inline uint32_t sh4_byteswap32(uint32_t value) noexcept {
            // Use SH-4 SWAP.B and SWAP.W instructions
            #ifdef __SH4_SINGLE_ONLY__
                uint32_t result;
                asm("swap.b %1, %0; swap.w %0, %0; swap.b %0, %0" 
                    : "=r"(result) : "r"(value));
                return result;
            #else
                return platform::byteswap(value);
            #endif
        }
        #endif
        
        // MIPS specific optimizations for PSP
        #ifdef __PSP__
        inline uint32_t mips_byteswap32(uint32_t value) noexcept {
            // Use MIPS WSBH (Word Swap Bytes within Halfwords) if available
            #ifdef __mips_isa_rev
                #if __mips_isa_rev >= 2
                    uint32_t result;
                    asm("wsbh %0, %1; rotr %0, %0, 16" : "=r"(result) : "r"(value));
                    return result;
                #endif
            #endif
            return platform::byteswap(value);
        }
        #endif
        
        // ARM specific optimizations for PS Vita
        #ifdef __vita__
        inline uint16_t arm_byteswap16(uint16_t value) noexcept {
            #ifdef __ARM_FEATURE_REV
                uint16_t result;
                asm("rev16 %0, %1" : "=r"(result) : "r"(value));
                return result;
            #else
                return platform::byteswap(value);
            #endif
        }
        
        inline uint32_t arm_byteswap32(uint32_t value) noexcept {
            #ifdef __ARM_FEATURE_REV
                uint32_t result;
                asm("rev %0, %1" : "=r"(result) : "r"(value));
                return result;
            #else
                return platform::byteswap(value);
            #endif
        }
        #endif
    }

    // Optimized conversions for specific types
    inline uint16_t host_to_wasm_u16(uint16_t value) noexcept {
        if constexpr (platform::CurrentPlatform::is_big_endian) {
            #ifdef __DREAMCAST__
                return detail::sh4_byteswap16(value);
            #elif defined(__vita__)
                return detail::arm_byteswap16(value);
            #else
                return platform::byteswap(value);
            #endif
        } else {
            return value;
        }
    }
    
    inline uint32_t host_to_wasm_u32(uint32_t value) noexcept {
        if constexpr (platform::CurrentPlatform::is_big_endian) {
            #ifdef __DREAMCAST__
                return detail::sh4_byteswap32(value);
            #elif defined(__PSP__)
                return detail::mips_byteswap32(value);
            #elif defined(__vita__)
                return detail::arm_byteswap32(value);
            #else
                return platform::byteswap(value);
            #endif
        } else {
            return value;
        }
    }
    
    inline uint64_t host_to_wasm_u64(uint64_t value) noexcept {
        if constexpr (platform::CurrentPlatform::is_big_endian) {
            return platform::byteswap(value);
        } else {
            return value;
        }
    }

    // Float and double conversions
    inline float host_to_wasm_f32(float value) noexcept {
        if constexpr (platform::CurrentPlatform::is_big_endian) {
            uint32_t bits;
            std::memcpy(&bits, &value, sizeof(float));
            bits = host_to_wasm_u32(bits);
            float result;
            std::memcpy(&result, &bits, sizeof(float));
            return result;
        } else {
            return value;
        }
    }
    
    inline double host_to_wasm_f64(double value) noexcept {
        if constexpr (platform::CurrentPlatform::is_big_endian) {
            uint64_t bits;
            std::memcpy(&bits, &value, sizeof(double));
            bits = host_to_wasm_u64(bits);
            double result;
            std::memcpy(&result, &bits, sizeof(double));
            return result;
        } else {
            return value;
        }
    }

    // Symmetric operations for WASM to host
    inline uint16_t wasm_to_host_u16(uint16_t value) noexcept { return host_to_wasm_u16(value); }
    inline uint32_t wasm_to_host_u32(uint32_t value) noexcept { return host_to_wasm_u32(value); }
    inline uint64_t wasm_to_host_u64(uint64_t value) noexcept { return host_to_wasm_u64(value); }
    inline float wasm_to_host_f32(float value) noexcept { return host_to_wasm_f32(value); }
    inline double wasm_to_host_f64(double value) noexcept { return host_to_wasm_f64(value); }

    // Memory copying with endianness conversion
    template<typename T>
    void copy_host_to_wasm(void* dest, const T* src, size_t count) noexcept {
        if constexpr (platform::CurrentPlatform::is_big_endian) {
            auto* dest_typed = static_cast<T*>(dest);
            for (size_t i = 0; i < count; ++i) {
                dest_typed[i] = host_to_wasm(src[i]);
            }
        } else {
            std::memcpy(dest, src, count * sizeof(T));
        }
    }
    
    template<typename T>
    void copy_wasm_to_host(T* dest, const void* src, size_t count) noexcept {
        if constexpr (platform::CurrentPlatform::is_big_endian) {
            const auto* src_typed = static_cast<const T*>(src);
            for (size_t i = 0; i < count; ++i) {
                dest[i] = wasm_to_host(src_typed[i]);
            }
        } else {
            std::memcpy(dest, src, count * sizeof(T));
        }
    }

    // Bulk conversion utilities for arrays
    template<typename T, size_t N>
    constexpr std::array<T, N> host_to_wasm_array(const std::array<T, N>& arr) noexcept {
        if constexpr (platform::CurrentPlatform::is_big_endian) {
            std::array<T, N> result;
            for (size_t i = 0; i < N; ++i) {
                result[i] = host_to_wasm(arr[i]);
            }
            return result;
        } else {
            return arr;
        }
    }
    
    template<typename T, size_t N>
    constexpr std::array<T, N> wasm_to_host_array(const std::array<T, N>& arr) noexcept {
        return host_to_wasm_array(arr); // Symmetric operation
    }

    // Runtime endianness detection (for debugging/validation)
    inline bool runtime_is_little_endian() noexcept {
        constexpr uint32_t test_value = 0x01020304;
        const auto* bytes = reinterpret_cast<const uint8_t*>(&test_value);
        return bytes[0] == 0x04;
    }
    
    inline bool runtime_is_big_endian() noexcept {
        return !runtime_is_little_endian();
    }

    // Validation that compile-time and runtime detection match
    inline bool endianness_detection_consistent() noexcept {
        return platform::CurrentPlatform::is_big_endian == runtime_is_big_endian();
    }

    // WebAssembly type validation - ensure IEEE 754 compliance
    namespace validation {
        static_assert(sizeof(float) == 4, "f32 must be 4 bytes on all platforms");
        static_assert(sizeof(double) == 8, "f64 must be 8 bytes on all platforms");
        static_assert(sizeof(int32_t) == 4, "i32 must be 4 bytes on all platforms");
        static_assert(sizeof(int64_t) == 8, "i64 must be 8 bytes on all platforms");
        
        // Verify IEEE 754 compliance
        static_assert(std::numeric_limits<float>::is_iec559, "float must be IEEE 754");
        static_assert(std::numeric_limits<double>::is_iec559, "double must be IEEE 754");
        
        // Verify two's complement integers
        static_assert(static_cast<int32_t>(0xFFFFFFFF) == -1, "integers must use two's complement");
        static_assert(static_cast<int64_t>(0xFFFFFFFFFFFFFFFF) == -1, "integers must use two's complement");
    }

} // namespace flight::wasm::endian

#endif // FLIGHT_WASM_UTILITIES_ENDIAN_HPP
