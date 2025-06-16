#ifndef FLIGHT_WASM_UTILITIES_PLATFORM_HPP
#define FLIGHT_WASM_UTILITIES_PLATFORM_HPP

/**
 * @file platform.hpp
 * @brief Platform detection and optimization utilities for Flight WASM
 * 
 * This header provides compile-time platform detection and optimization
 * macros for embedded systems and cross-platform development.
 */

#include <cstdint>
#include <cstddef>
#include <type_traits>

#if __cplusplus >= 202002L
    #include <bit>
#endif

namespace flight::wasm::platform {

    // Platform enumeration
    enum class Platform {
        Dreamcast,   // Sega Dreamcast (SH-4, 16MB RAM, big-endian)
        PSP,         // PlayStation Portable (MIPS, 32MB RAM, little-endian)
        PSVita,      // PlayStation Vita (ARM Cortex-A9, 512MB RAM, little-endian)
        Emscripten,  // WebAssembly target (variable resources)
        MacOS,       // macOS (x86_64/ARM64, 8GB+ RAM)
        Linux,       // Linux (x86_64/ARM64, 2GB+ RAM)
        Windows,     // Windows (x86_64, 2GB+ RAM)
        Unknown      // Fallback for unrecognized platforms
    };

    // Platform detection at compile time
    constexpr Platform current_platform() noexcept {
        #ifdef __DREAMCAST__
            return Platform::Dreamcast;
        #elif defined(__PSP__)
            return Platform::PSP;
        #elif defined(__vita__) || defined(__VITA__)
            return Platform::PSVita;
        #elif defined(__EMSCRIPTEN__)
            return Platform::Emscripten;
        #elif defined(__APPLE__)
            return Platform::MacOS;
        #elif defined(__linux__)
            return Platform::Linux;
        #elif defined(_WIN32)
            return Platform::Windows;
        #else
            return Platform::Unknown;
        #endif
    }

    // Platform capability traits
    template<Platform P>
    struct PlatformTraits;

    template<>
    struct PlatformTraits<Platform::Dreamcast> {
        static constexpr bool is_embedded = true;
        static constexpr bool is_big_endian = true;
        static constexpr bool has_simd = false;
        static constexpr bool has_fpu = true;
        static constexpr bool has_efficient_64bit = false;
        static constexpr bool prefer_lookup_tables = true;  // Good cache
        static constexpr size_t max_memory = 16 * 1024 * 1024; // 16MB
        static constexpr size_t stack_size = 64 * 1024; // 64KB
        static constexpr size_t cache_line_size = 32;
        static constexpr size_t preferred_alignment = 32;
        static constexpr const char* cpu_arch = "sh4";
        static constexpr int cpu_frequency_mhz = 200;
        using preferred_size_type = uint16_t;
    };

    template<>
    struct PlatformTraits<Platform::PSP> {
        static constexpr bool is_embedded = true;
        static constexpr bool is_big_endian = false;
        static constexpr bool has_simd = false;
        static constexpr bool has_fpu = true;
        static constexpr bool has_efficient_64bit = false;
        static constexpr bool prefer_lookup_tables = false; // Limited cache
        static constexpr size_t max_memory = 32 * 1024 * 1024; // 32MB
        static constexpr size_t stack_size = 256 * 1024; // 256KB
        static constexpr size_t cache_line_size = 64;
        static constexpr size_t preferred_alignment = 16;
        static constexpr const char* cpu_arch = "mips";
        static constexpr int cpu_frequency_mhz = 333;
        using preferred_size_type = uint16_t;
    };

    template<>
    struct PlatformTraits<Platform::PSVita> {
        static constexpr bool is_embedded = false; // More capable than other embedded
        static constexpr bool is_big_endian = false;
        static constexpr bool has_simd = true; // NEON
        static constexpr bool has_fpu = true;
        static constexpr bool has_efficient_64bit = true;
        static constexpr bool prefer_lookup_tables = true; // Good cache
        static constexpr size_t max_memory = 512 * 1024 * 1024; // 512MB
        static constexpr size_t stack_size = 1024 * 1024; // 1MB
        static constexpr size_t cache_line_size = 64;
        static constexpr size_t preferred_alignment = 16;
        static constexpr const char* cpu_arch = "arm";
        static constexpr int cpu_frequency_mhz = 444;
        using preferred_size_type = uint32_t;
    };

    template<>
    struct PlatformTraits<Platform::Emscripten> {
        static constexpr bool is_embedded = false;
        static constexpr bool is_big_endian = false;
        static constexpr bool has_simd = true; // SIMD128
        static constexpr bool has_fpu = true;
        static constexpr bool has_efficient_64bit = true;
        static constexpr bool prefer_lookup_tables = true;
        static constexpr size_t max_memory = SIZE_MAX;
        static constexpr size_t stack_size = 1024 * 1024; // 1MB
        static constexpr size_t cache_line_size = 64;
        static constexpr size_t preferred_alignment = 16;
        static constexpr const char* cpu_arch = "wasm32";
        static constexpr int cpu_frequency_mhz = 0; // Variable
        using preferred_size_type = uint32_t;
    };

    // Desktop platforms use similar traits
    template<>
    struct PlatformTraits<Platform::MacOS> {
        static constexpr bool is_embedded = false;
        static constexpr bool is_big_endian = false;
        static constexpr bool has_simd = true;
        static constexpr bool has_fpu = true;
        static constexpr bool has_efficient_64bit = true;
        static constexpr bool prefer_lookup_tables = true;
        static constexpr size_t max_memory = SIZE_MAX;
        static constexpr size_t stack_size = 8 * 1024 * 1024; // 8MB
        static constexpr size_t cache_line_size = 64;
        static constexpr size_t preferred_alignment = 16;
        static constexpr const char* cpu_arch = "x86_64";
        static constexpr int cpu_frequency_mhz = 0; // Variable
        using preferred_size_type = size_t;
    };

    template<>
    struct PlatformTraits<Platform::Linux> {
        static constexpr bool is_embedded = false;
        static constexpr bool is_big_endian = false;
        static constexpr bool has_simd = true;
        static constexpr bool has_fpu = true;
        static constexpr bool has_efficient_64bit = true;
        static constexpr bool prefer_lookup_tables = true;
        static constexpr size_t max_memory = SIZE_MAX;
        static constexpr size_t stack_size = 8 * 1024 * 1024; // 8MB
        static constexpr size_t cache_line_size = 64;
        static constexpr size_t preferred_alignment = 16;
        static constexpr const char* cpu_arch = "x86_64";
        static constexpr int cpu_frequency_mhz = 0; // Variable
        using preferred_size_type = size_t;
    };

    template<>
    struct PlatformTraits<Platform::Windows> {
        static constexpr bool is_embedded = false;
        static constexpr bool is_big_endian = false;
        static constexpr bool has_simd = true;
        static constexpr bool has_fpu = true;
        static constexpr bool has_efficient_64bit = true;
        static constexpr bool prefer_lookup_tables = true;
        static constexpr size_t max_memory = SIZE_MAX;
        static constexpr size_t stack_size = 1024 * 1024; // 1MB default
        static constexpr size_t cache_line_size = 64;
        static constexpr size_t preferred_alignment = 16;
        static constexpr const char* cpu_arch = "x86_64";
        static constexpr int cpu_frequency_mhz = 0; // Variable
        using preferred_size_type = size_t;
    };

    // Current platform traits
    using CurrentPlatform = PlatformTraits<current_platform()>;

    // Byte swapping utilities
    template<typename T>
    constexpr T byteswap(T value) noexcept {
        if constexpr (sizeof(T) == 1) {
            return value;
        } else if constexpr (sizeof(T) == 2) {
            auto x = static_cast<uint16_t>(value);
            return static_cast<T>((x << 8) | (x >> 8));
        } else if constexpr (sizeof(T) == 4) {
            auto x = static_cast<uint32_t>(value);
            return static_cast<T>(
                ((x << 24) & 0xFF000000U) |
                ((x << 8)  & 0x00FF0000U) |
                ((x >> 8)  & 0x0000FF00U) |
                ((x >> 24) & 0x000000FFU)
            );
        } else if constexpr (sizeof(T) == 8) {
            auto x = static_cast<uint64_t>(value);
            return static_cast<T>(
                ((x << 56) & 0xFF00000000000000ULL) |
                ((x << 40) & 0x00FF000000000000ULL) |
                ((x << 24) & 0x0000FF0000000000ULL) |
                ((x << 8)  & 0x000000FF00000000ULL) |
                ((x >> 8)  & 0x00000000FF000000ULL) |
                ((x >> 24) & 0x0000000000FF0000ULL) |
                ((x >> 40) & 0x000000000000FF00ULL) |
                ((x >> 56) & 0x00000000000000FFULL)
            );
        } else {
            static_assert(sizeof(T) <= 8, "Unsupported type size for byteswap");
        }
    }

    // Endianness detection using compile-time macros
    constexpr bool is_big_endian() noexcept {
        #if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && defined(__ORDER_LITTLE_ENDIAN__)
            return __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__;
        #elif defined(__BIG_ENDIAN__) || defined(__ARMEB__) || defined(__THUMBEB__) || defined(__AARCH64EB__) || defined(_BIG_ENDIAN)
            return true;
        #elif defined(__LITTLE_ENDIAN__) || defined(__ARMEL__) || defined(__THUMBEL__) || defined(__AARCH64EL__) || defined(_LITTLE_ENDIAN)
            return false;
        #else
            // Default to little endian for most modern platforms
            return false;
        #endif
    }

    constexpr bool is_little_endian() noexcept {
        return !is_big_endian();
    }

    // Platform-specific feature detection
    #ifdef __DREAMCAST__
        constexpr bool has_neon_simd = false;
        constexpr bool has_limited_memory = true;
        constexpr bool is_big_endian_platform = true;
        constexpr size_t max_memory = 16 * 1024 * 1024; // 16MB
        constexpr size_t cache_line_size = 32;
        using preferred_size_type = uint16_t; // Conserve memory
        #define FLIGHT_WASM_PLATFORM_DREAMCAST 1
        #define FLIGHT_WASM_BIG_ENDIAN 1
        #define FLIGHT_WASM_SMALL_STACK 1
    #elif defined(__PSP__)
        constexpr bool has_neon_simd = false;
        constexpr bool has_limited_memory = true;
        constexpr bool is_big_endian_platform = false;
        constexpr size_t max_memory = 32 * 1024 * 1024; // 32MB
        constexpr size_t cache_line_size = 64;
        using preferred_size_type = uint16_t; // Conserve memory
        #define FLIGHT_WASM_PLATFORM_PSP 1
        #define FLIGHT_WASM_SMALL_STACK 1
    #elif defined(__vita__) || defined(__VITA__)
        constexpr bool has_neon_simd = true;
        constexpr bool has_limited_memory = false;
        constexpr bool is_big_endian_platform = false;
        constexpr size_t max_memory = 512 * 1024 * 1024; // 512MB
        constexpr size_t cache_line_size = 64;
        using preferred_size_type = uint32_t;
        #define FLIGHT_WASM_PLATFORM_VITA 1
        #define FLIGHT_WASM_NEON_AVAILABLE 1
    #elif defined(__EMSCRIPTEN__)
        constexpr bool has_neon_simd = false;
        constexpr bool has_limited_memory = false;
        constexpr bool is_big_endian_platform = false;
        constexpr size_t max_memory = SIZE_MAX;
        constexpr size_t cache_line_size = 64;
        using preferred_size_type = size_t;
        #define FLIGHT_WASM_PLATFORM_EMSCRIPTEN 1
    #else
        // Default desktop/server platform
        constexpr bool has_neon_simd = false;
        constexpr bool has_limited_memory = false;
        constexpr bool is_big_endian_platform = false;
        constexpr size_t max_memory = SIZE_MAX;
        constexpr size_t cache_line_size = 64;
        using preferred_size_type = size_t;
        #define FLIGHT_WASM_PLATFORM_DESKTOP 1
    #endif

    // Template recursion limits for embedded platforms
    #if defined(FLIGHT_WASM_SMALL_STACK) || defined(FLIGHT_WASM_PLATFORM_PSP) || defined(FLIGHT_WASM_PLATFORM_DREAMCAST)
        constexpr int max_template_depth = 64;
        constexpr size_t max_stack_size = 8192; // 8KB stack limit
    #else
        constexpr int max_template_depth = 1024;
        constexpr size_t max_stack_size = 1048576; // 1MB stack
    #endif

    // Memory alignment utilities
    template<typename T>
    constexpr size_t alignment_of_v = alignof(T);

    template<size_t N>
    constexpr size_t align_up(size_t value) noexcept {
        static_assert((N & (N - 1)) == 0, "Alignment must be power of 2");
        return (value + N - 1) & ~(N - 1);
    }

    template<size_t N>
    constexpr size_t align_down(size_t value) noexcept {
        static_assert((N & (N - 1)) == 0, "Alignment must be power of 2");
        return value & ~(N - 1);
    }

    // Cache-friendly allocation sizes
    constexpr size_t cache_aligned_size(size_t size) noexcept {
        return align_up<cache_line_size>(size);
    }

} // namespace flight::wasm::platform

// Performance optimization macros (only define if not already defined by build system)
#ifndef FLIGHT_WASM_INLINE
#define FLIGHT_WASM_INLINE inline constexpr
#endif

#ifndef FLIGHT_WASM_FORCE_INLINE
#define FLIGHT_WASM_FORCE_INLINE [[gnu::always_inline]] inline constexpr
#endif

// Likely/unlikely hints for branch prediction
#ifdef __has_builtin
    #if __has_builtin(__builtin_expect)
        #define FLIGHT_WASM_LIKELY(x) __builtin_expect(!!(x), 1)
        #define FLIGHT_WASM_UNLIKELY(x) __builtin_expect(!!(x), 0)
    #else
        #define FLIGHT_WASM_LIKELY(x) (x)
        #define FLIGHT_WASM_UNLIKELY(x) (x)
    #endif
#else
    #define FLIGHT_WASM_LIKELY(x) (x)
    #define FLIGHT_WASM_UNLIKELY(x) (x)
#endif

// Compiler-specific optimizations
#ifdef __GNUC__
    #define FLIGHT_WASM_PURE __attribute__((pure))
    #define FLIGHT_WASM_CONST __attribute__((const))
    #define FLIGHT_WASM_HOT __attribute__((hot))
    #define FLIGHT_WASM_COLD __attribute__((cold))
#else
    #define FLIGHT_WASM_PURE
    #define FLIGHT_WASM_CONST
    #define FLIGHT_WASM_HOT
    #define FLIGHT_WASM_COLD
#endif

// Debug/release mode detection
#if defined(NDEBUG)
    #define FLIGHT_WASM_DEBUG 0
    #define FLIGHT_WASM_RELEASE 1
#else
    #define FLIGHT_WASM_DEBUG 1
    #define FLIGHT_WASM_RELEASE 0
#endif

#endif // FLIGHT_WASM_UTILITIES_PLATFORM_HPP
