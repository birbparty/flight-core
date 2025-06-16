#ifndef FLIGHT_HAL_HAL_HPP
#define FLIGHT_HAL_HAL_HPP

#include "platform.hpp"
#include "result.hpp"
#include "driver.hpp"
#include "memory_manager.hpp"
#include "video_driver.hpp"
#include "audio_driver.hpp"
#include "input_driver.hpp"
#include "file_driver.hpp"
#include "thread_driver.hpp"
#include "time_driver.hpp"

#include <memory>
#include <string>
#include <vector>

namespace flight
{
    namespace hal
    {

        // Platform capabilities structure
        struct PlatformCapabilities
        {
            // Platform identification
            const char *name;         // "macOS", "Dreamcast", etc.
            const char *architecture; // "x86_64", "sh4", "mips", etc.
            const char *vendor;       // "Apple", "Sega", "Sony", etc.

            // Hardware capabilities
            uint32_t cpu_count;
            uint64_t total_memory;
            uint32_t cache_line_size;
            bool has_simd;
            bool has_fpu;
            bool is_big_endian;

            // Feature support
            bool has_threading;
            bool has_memory_mapping;
            bool has_networking;
            bool has_filesystem;
            bool has_graphics;
            bool has_audio;
            bool has_input;
        };

        // Platform configuration
        struct PlatformConfig
        {
            // Memory configuration
            MemoryConfig memory_config;

            // Video configuration (optional)
            VideoConfig video_config;
            bool enable_video;

            // Audio configuration (optional)
            AudioConfig audio_config;
            bool enable_audio;

            // Threading configuration
            bool enable_threading;

            // File system configuration
            bool enable_filesystem;

            // Input configuration
            bool enable_input;

            // Driver preferences
            const char *preferred_video_driver;
            const char *preferred_audio_driver;
            const char *preferred_input_driver;
        };

        // Main platform interface (RetroArch frontend pattern)
        class Platform
        {
        public:
            virtual ~Platform() = default;

            // Platform information
            virtual const char *name() const = 0;
            virtual const PlatformCapabilities &capabilities() const = 0;
            virtual PlatformType type() const = 0;

            // Platform lifecycle
            virtual Result<void> initialize(const PlatformConfig &config) = 0;
            virtual void shutdown() = 0;
            virtual bool is_initialized() const = 0;

            // Driver management (RetroArch pattern)
            virtual Result<void> set_video_driver(const char *driver_name) = 0;
            virtual Result<void> set_audio_driver(const char *driver_name) = 0;
            virtual Result<void> set_input_driver(const char *driver_name) = 0;
            virtual Result<void> set_file_driver(const char *driver_name) = 0;
            virtual Result<void> set_thread_driver(const char *driver_name) = 0;
            virtual Result<void> set_time_driver(const char *driver_name) = 0;

            // Get active drivers
            virtual MemoryManager *memory() = 0;
            virtual VideoDriver *video() = 0;
            virtual AudioDriver *audio() = 0;
            virtual InputDriver *input() = 0;
            virtual FileDriver *file() = 0;
            virtual ThreadDriver *thread() = 0;
            virtual TimeDriver *time() = 0;

            // Driver enumeration
            virtual std::vector<const char *> get_available_drivers(DriverType type) const = 0;
            virtual const char *get_current_driver_name(DriverType type) const = 0;

            // Platform-specific paths
            virtual const char *get_executable_path() const = 0;
            virtual const char *get_data_path() const = 0;
            virtual const char *get_config_path() const = 0;
            virtual const char *get_cache_path() const = 0;
            virtual const char *get_temp_path() const = 0;

            // System operations
            virtual void request_quit() = 0;
            virtual bool should_quit() const = 0;
            virtual void poll_events() = 0;

            // Power management
            virtual void prevent_screen_saver(bool prevent) = 0;
            virtual void set_low_power_mode(bool enable) = 0;

            // Debug/diagnostic
            virtual void log_message(const char *message) = 0;
            virtual void panic(const char *message) = 0;
        };

        // Platform factory functions

        // Auto-detect platform and create instance
        std::unique_ptr<Platform> create_platform();

        // Create specific platform instance
        std::unique_ptr<Platform> create_platform(PlatformType type);

        // Get default configuration for platform
        PlatformConfig get_default_config(PlatformType type);

        // Global platform instance management

        // Set the global platform instance
        void set_global_platform(Platform *platform);

        // Get the global platform instance
        Platform *get_global_platform();

        // Convenience functions that use the global platform
        namespace global
        {
            // Memory operations
            inline void *alloc(PoolType pool, size_t size)
            {
                auto *platform = get_global_platform();
                if (!platform || !platform->memory())
                    return nullptr;
                return platform->memory()->pool_alloc(pool, size);
            }

            inline void free(PoolType pool, void *ptr)
            {
                auto *platform = get_global_platform();
                if (!platform || !platform->memory())
                    return;
                platform->memory()->pool_free(pool, ptr);
            }

            // Time operations
            inline TimePoint now()
            {
                auto *platform = get_global_platform();
                if (!platform || !platform->time())
                    return TimePoint{};
                return platform->time()->now();
            }

            inline void sleep_ms(uint32_t ms)
            {
                auto *platform = get_global_platform();
                if (!platform || !platform->time())
                    return;
                platform->time()->sleep_ms(ms);
            }

            // File operations
            inline Result<FileHandle> open_file(const char *path, FileMode mode)
            {
                auto *platform = get_global_platform();
                if (!platform || !platform->file())
                    return ErrorCode::NotImplemented;
                return platform->file()->open(path, mode);
            }

            // Logging
            inline void log(const char *message)
            {
                auto *platform = get_global_platform();
                if (!platform)
                    return;
                platform->log_message(message);
            }
        }

        // Platform-specific default configurations
        namespace platform_configs
        {
            // Desktop (macOS, Linux, Windows)
            constexpr PlatformConfig DESKTOP_CONFIG = {
                .memory_config = memory_configs::DESKTOP_CONFIG,
                .video_config = {
                    .width = 1280,
                    .height = 720,
                    .fullscreen = false,
                    .vsync = true,
                    .msaa_samples = 4,
                    .double_buffer = true,
                    .triple_buffer = false,
                    .preferred_api = GraphicsAPI::OpenGL},
                .enable_video = true,
                .audio_config = audio_defaults::DESKTOP_CONFIG,
                .enable_audio = true,
                .enable_threading = true,
                .enable_filesystem = true,
                .enable_input = true,
                .preferred_video_driver = nullptr, // Auto-select
                .preferred_audio_driver = nullptr, // Auto-select
                .preferred_input_driver = nullptr  // Auto-select
            };

            // Dreamcast configuration
            constexpr PlatformConfig DREAMCAST_CONFIG = {
                .memory_config = memory_configs::DREAMCAST_CONFIG,
                .video_config = {
                    .width = 640,
                    .height = 480,
                    .fullscreen = true,
                    .vsync = true,
                    .msaa_samples = 0,
                    .double_buffer = true,
                    .triple_buffer = false,
                    .preferred_api = GraphicsAPI::PowerVR2},
                .enable_video = true,
                .audio_config = audio_defaults::DREAMCAST_CONFIG,
                .enable_audio = true,
                .enable_threading = false, // No threading
                .enable_filesystem = true,
                .enable_input = true,
                .preferred_video_driver = "pvr",
                .preferred_audio_driver = "aica",
                .preferred_input_driver = "maple"};

            // PSP configuration
            constexpr PlatformConfig PSP_CONFIG = {
                .memory_config = memory_configs::PSP_CONFIG,
                .video_config = {
                    .width = 480,
                    .height = 272,
                    .fullscreen = true,
                    .vsync = true,
                    .msaa_samples = 0,
                    .double_buffer = true,
                    .triple_buffer = false,
                    .preferred_api = GraphicsAPI::PSPGu},
                .enable_video = true,
                .audio_config = audio_defaults::PSP_CONFIG,
                .enable_audio = true,
                .enable_threading = true, // Cooperative threading
                .enable_filesystem = true,
                .enable_input = true,
                .preferred_video_driver = "gu",
                .preferred_audio_driver = "audio",
                .preferred_input_driver = "ctrl"};

            // Web/Emscripten configuration
            constexpr PlatformConfig WEB_CONFIG = {
                .memory_config = memory_configs::WEB_CONFIG,
                .video_config = {
                    .width = 1280,
                    .height = 720,
                    .fullscreen = false,
                    .vsync = true,
                    .msaa_samples = 2,
                    .double_buffer = true,
                    .triple_buffer = false,
                    .preferred_api = GraphicsAPI::WebGL2},
                .enable_video = true,
                .audio_config = audio_defaults::WEB_CONFIG,
                .enable_audio = true,
                .enable_threading = true,  // Web Workers
                .enable_filesystem = true, // Virtual FS
                .enable_input = true,
                .preferred_video_driver = "webgl",
                .preferred_audio_driver = "webaudio",
                .preferred_input_driver = "web"};
        }

        // Helper to get configuration for current platform
        inline PlatformConfig get_current_platform_config()
        {
#if defined(FLIGHT_PLATFORM_MACOS) || defined(FLIGHT_PLATFORM_LINUX) || defined(FLIGHT_PLATFORM_WINDOWS)
            return platform_configs::DESKTOP_CONFIG;
#elif defined(FLIGHT_PLATFORM_DREAMCAST)
            return platform_configs::DREAMCAST_CONFIG;
#elif defined(FLIGHT_PLATFORM_PSP)
            return platform_configs::PSP_CONFIG;
#elif defined(FLIGHT_PLATFORM_EMSCRIPTEN)
            return platform_configs::WEB_CONFIG;
#else
#error "Unknown platform"
#endif
        }

    } // namespace hal
} // namespace flight

#endif // FLIGHT_HAL_HAL_HPP
