#ifndef FLIGHT_HAL_PLATFORM_HPP
#define FLIGHT_HAL_PLATFORM_HPP

// Platform detection macros
#if defined(__APPLE__) && defined(__MACH__)
#define FLIGHT_PLATFORM_MACOS 1
#define FLIGHT_PLATFORM_NAME "macOS"
#define FLIGHT_PLATFORM_POSIX 1
#elif defined(__EMSCRIPTEN__)
#define FLIGHT_PLATFORM_EMSCRIPTEN 1
#define FLIGHT_PLATFORM_NAME "Emscripten"
#define FLIGHT_PLATFORM_WEB 1
#elif defined(_arch_dreamcast)
#define FLIGHT_PLATFORM_DREAMCAST 1
#define FLIGHT_PLATFORM_NAME "Dreamcast"
#define FLIGHT_PLATFORM_EMBEDDED 1
#elif defined(PSP)
#define FLIGHT_PLATFORM_PSP 1
#define FLIGHT_PLATFORM_NAME "PSP"
#define FLIGHT_PLATFORM_EMBEDDED 1
#else
#error "Unsupported platform. Please add platform detection for your target."
#endif

// Feature detection based on platform
#if defined(FLIGHT_PLATFORM_MACOS)
#define FLIGHT_HAS_THREADS 1
#define FLIGHT_HAS_MMAP 1
#define FLIGHT_HAS_SIMD 1
#define FLIGHT_HAS_FILESYSTEM 1
#define FLIGHT_HAS_DYNAMIC_ALLOC 1
#define FLIGHT_ARCH "x86_64"
#elif defined(FLIGHT_PLATFORM_EMSCRIPTEN)
#define FLIGHT_HAS_THREADS 0 // Web Workers are not real threads
#define FLIGHT_HAS_MMAP 0
#define FLIGHT_HAS_SIMD 1
#define FLIGHT_HAS_FILESYSTEM 1 // Virtual FS
#define FLIGHT_HAS_DYNAMIC_ALLOC 1
#define FLIGHT_ARCH "wasm32"
#elif defined(FLIGHT_PLATFORM_DREAMCAST)
#define FLIGHT_HAS_THREADS 0 // Single core
#define FLIGHT_HAS_MMAP 0
#define FLIGHT_HAS_SIMD 0
#define FLIGHT_HAS_FILESYSTEM 1
#define FLIGHT_HAS_DYNAMIC_ALLOC 1
#define FLIGHT_ARCH "sh4"
#elif defined(FLIGHT_PLATFORM_PSP)
#define FLIGHT_HAS_THREADS 1 // Cooperative threads
#define FLIGHT_HAS_MMAP 0
#define FLIGHT_HAS_SIMD 1 // VFPU
#define FLIGHT_HAS_FILESYSTEM 1
#define FLIGHT_HAS_DYNAMIC_ALLOC 1
#define FLIGHT_ARCH "mips"
#endif

// Compiler detection
#if defined(__clang__)
#define FLIGHT_COMPILER_CLANG 1
#elif defined(__GNUC__)
#define FLIGHT_COMPILER_GCC 1
#elif defined(_MSC_VER)
#define FLIGHT_COMPILER_MSVC 1
#endif

// C++ standard detection
#if __cplusplus >= 201402L
#define FLIGHT_CPP14_OR_GREATER 1
#else
#error "Flight Core requires C++14 or later"
#endif

// Inline and noreturn attributes
#if defined(FLIGHT_COMPILER_MSVC)
#define FLIGHT_INLINE __forceinline
#define FLIGHT_NORETURN __declspec(noreturn)
#else
#define FLIGHT_INLINE inline __attribute__((always_inline))
#define FLIGHT_NORETURN __attribute__((noreturn))
#endif

// Export/import for shared libraries (future use)
#if defined(FLIGHT_COMPILER_MSVC)
#define FLIGHT_EXPORT __declspec(dllexport)
#define FLIGHT_IMPORT __declspec(dllimport)
#else
#define FLIGHT_EXPORT __attribute__((visibility("default")))
#define FLIGHT_IMPORT
#endif

// API macro
#ifdef FLIGHT_BUILDING
#define FLIGHT_API FLIGHT_EXPORT
#else
#define FLIGHT_API FLIGHT_IMPORT
#endif

namespace flight
{
    namespace hal
    {

        // Platform enumeration for runtime queries
        enum class PlatformType
        {
            macOS,
            Emscripten,
            Dreamcast,
            PSP,
            Unknown
        };

        // Get the current platform at runtime
        inline PlatformType get_current_platform() noexcept
        {
#if defined(FLIGHT_PLATFORM_MACOS)
            return PlatformType::macOS;
#elif defined(FLIGHT_PLATFORM_EMSCRIPTEN)
            return PlatformType::Emscripten;
#elif defined(FLIGHT_PLATFORM_DREAMCAST)
            return PlatformType::Dreamcast;
#elif defined(FLIGHT_PLATFORM_PSP)
            return PlatformType::PSP;
#else
            return PlatformType::Unknown;
#endif
        }

        // Platform feature queries
        namespace features
        {
            constexpr bool has_threads() noexcept
            {
#if FLIGHT_HAS_THREADS
                return true;
#else
                return false;
#endif
            }

            constexpr bool has_memory_mapping() noexcept
            {
#if FLIGHT_HAS_MMAP
                return true;
#else
                return false;
#endif
            }

            constexpr bool has_simd() noexcept
            {
#if FLIGHT_HAS_SIMD
                return true;
#else
                return false;
#endif
            }

            constexpr bool has_filesystem() noexcept
            {
#if FLIGHT_HAS_FILESYSTEM
                return true;
#else
                return false;
#endif
            }

            constexpr bool has_dynamic_allocation() noexcept
            {
#if FLIGHT_HAS_DYNAMIC_ALLOC
                return true;
#else
                return false;
#endif
            }
        } // namespace features

    } // namespace hal
} // namespace flight

#endif // FLIGHT_HAL_PLATFORM_HPP
