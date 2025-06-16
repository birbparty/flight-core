#ifndef FLIGHT_HAL_AUDIO_DRIVER_HPP
#define FLIGHT_HAL_AUDIO_DRIVER_HPP

#include "driver.hpp"
#include "result.hpp"
#include <cstdint>
#include <functional>
#include <vector>

namespace flight
{
    namespace hal
    {

        // Audio format types
        enum class AudioFormat : uint32_t
        {
            None,
            U8,    // Unsigned 8-bit
            S8,    // Signed 8-bit
            U16LE, // Unsigned 16-bit little-endian
            S16LE, // Signed 16-bit little-endian
            U16BE, // Unsigned 16-bit big-endian
            S16BE, // Signed 16-bit big-endian
            S32LE, // Signed 32-bit little-endian
            S32BE, // Signed 32-bit big-endian
            F32LE, // 32-bit float little-endian
            F32BE, // 32-bit float big-endian
            ADPCM, // ADPCM compressed (Dreamcast)
            ATRAC3 // ATRAC3 compressed (PSP)
        };

        // Audio channel layouts
        enum class ChannelLayout : uint32_t
        {
            Mono,       // 1 channel
            Stereo,     // 2 channels (L, R)
            Surround21, // 2.1 channels (L, R, LFE)
            Surround40, // 4.0 channels (FL, FR, BL, BR)
            Surround51, // 5.1 channels
            Surround71  // 7.1 channels
        };

        // Audio stream handle
        struct AudioStreamHandle
        {
            uint32_t id;
            bool operator==(const AudioStreamHandle &other) const { return id == other.id; }
            bool operator!=(const AudioStreamHandle &other) const { return id != other.id; }
        };

        constexpr AudioStreamHandle INVALID_AUDIO_STREAM = {0};

        // Audio configuration
        struct AudioConfig
        {
            uint32_t sample_rate;   // 44100, 48000, etc.
            AudioFormat format;     // Sample format
            ChannelLayout layout;   // Channel configuration
            uint32_t buffer_frames; // Buffer size in frames
            uint32_t period_count;  // Number of periods (2 = double buffer)
            bool low_latency_mode;  // Prioritize latency over stability
        };

        // Audio capabilities
        struct AudioCapabilities
        {
            // Supported formats
            std::vector<AudioFormat> supported_formats;
            std::vector<uint32_t> supported_sample_rates;
            std::vector<ChannelLayout> supported_layouts;

            // Hardware capabilities
            uint32_t max_simultaneous_streams;
            uint32_t hardware_channels;
            bool has_hardware_mixing;
            bool has_hardware_3d;
            bool has_hardware_effects;
            bool has_dedicated_processor; // AICA on Dreamcast

            // Latency characteristics
            uint32_t min_latency_samples;
            uint32_t typical_latency_samples;
            uint32_t max_latency_samples;

            // Memory
            size_t audio_memory;     // Dedicated audio RAM
            bool uses_system_memory; // Or has dedicated memory
        };

        // Audio stream descriptor
        struct AudioStreamDesc
        {
            AudioFormat format;
            uint32_t sample_rate;
            ChannelLayout layout;
            bool is_looping;
            bool is_3d;
            uint32_t priority; // Higher priority streams get resources first
        };

        // 3D audio position (for platforms that support it)
        struct AudioPosition3D
        {
            float x, y, z;
            float velocity_x, velocity_y, velocity_z;
        };

        // Audio callback function type (for low-latency audio)
        using AudioCallback = std::function<void(void *output_buffer, uint32_t frame_count, void *userdata)>;

        // Audio driver interface (RetroArch pattern)
        class AudioDriver : public Driver
        {
        public:
            // Driver interface
            DriverType type() const override { return DriverType::Audio; }

            // Initialize audio system
            virtual Result<void> initialize_audio(const AudioConfig &config) = 0;

            // Capability queries
            virtual AudioCapabilities capabilities() const = 0;
            virtual std::vector<AudioFormat> supported_formats() const = 0;
            virtual std::vector<uint32_t> supported_sample_rates() const = 0;
            virtual bool supports_format(AudioFormat format, uint32_t sample_rate) const = 0;

            // Stream management
            virtual Result<AudioStreamHandle> create_stream(const AudioStreamDesc &desc) = 0;
            virtual void destroy_stream(AudioStreamHandle handle) = 0;

            // Playback control
            virtual Result<void> play_stream(AudioStreamHandle handle) = 0;
            virtual Result<void> pause_stream(AudioStreamHandle handle) = 0;
            virtual Result<void> stop_stream(AudioStreamHandle handle) = 0;
            virtual bool is_playing(AudioStreamHandle handle) const = 0;

            // Data submission
            virtual Result<void> write_stream_data(
                AudioStreamHandle handle,
                const void *data,
                size_t size_bytes) = 0;

            // Get writeable buffer (for zero-copy)
            virtual Result<void *> get_stream_buffer(
                AudioStreamHandle handle,
                size_t *available_bytes) = 0;

            virtual Result<void> commit_stream_buffer(
                AudioStreamHandle handle,
                size_t bytes_written) = 0;

            // Stream properties
            virtual Result<void> set_stream_volume(AudioStreamHandle handle, float volume) = 0;
            virtual Result<void> set_stream_pitch(AudioStreamHandle handle, float pitch) = 0;
            virtual Result<void> set_stream_pan(AudioStreamHandle handle, float pan) = 0;

            // 3D audio (if supported)
            virtual bool supports_3d_audio() const = 0;
            virtual Result<void> set_listener_position(const AudioPosition3D &position) = 0;
            virtual Result<void> set_stream_position(
                AudioStreamHandle handle,
                const AudioPosition3D &position) = 0;

            // Low-latency callback interface (for real-time audio)
            virtual Result<void> set_callback(
                AudioCallback callback,
                void *userdata) = 0;

            virtual void remove_callback() = 0;

            // Latency control
            virtual uint32_t get_latency_samples() const = 0;
            virtual Result<void> set_latency_mode(bool low_latency) = 0;

            // Master volume control
            virtual Result<void> set_master_volume(float volume) = 0;
            virtual float get_master_volume() const = 0;

            // Platform-specific optimizations
            virtual void optimize_for_latency() = 0;
            virtual void optimize_for_quality() = 0;
            virtual void optimize_for_power() = 0;

            // Debug/statistics
            virtual uint32_t get_underrun_count() const = 0;
            virtual uint32_t get_active_stream_count() const = 0;
            virtual float get_cpu_usage() const = 0;
        };

        // Helper functions
        inline uint32_t get_audio_format_size(AudioFormat format)
        {
            switch (format)
            {
            case AudioFormat::U8:
            case AudioFormat::S8:
                return 1;
            case AudioFormat::U16LE:
            case AudioFormat::S16LE:
            case AudioFormat::U16BE:
            case AudioFormat::S16BE:
                return 2;
            case AudioFormat::S32LE:
            case AudioFormat::S32BE:
            case AudioFormat::F32LE:
            case AudioFormat::F32BE:
                return 4;
            default:
                return 0; // Variable for compressed formats
            }
        }

        inline uint32_t get_channel_count(ChannelLayout layout)
        {
            switch (layout)
            {
            case ChannelLayout::Mono:
                return 1;
            case ChannelLayout::Stereo:
                return 2;
            case ChannelLayout::Surround21:
                return 3;
            case ChannelLayout::Surround40:
                return 4;
            case ChannelLayout::Surround51:
                return 6;
            case ChannelLayout::Surround71:
                return 8;
            default:
                return 0;
            }
        }

        inline size_t calculate_audio_buffer_size(
            AudioFormat format,
            ChannelLayout layout,
            uint32_t frame_count)
        {
            uint32_t format_size = get_audio_format_size(format);
            uint32_t channel_count = get_channel_count(layout);
            return format_size * channel_count * frame_count;
        }

        // Platform-specific audio defaults
        namespace audio_defaults
        {
            // Desktop defaults
            constexpr AudioConfig DESKTOP_CONFIG = {
                .sample_rate = 48000,
                .format = AudioFormat::F32LE,
                .layout = ChannelLayout::Stereo,
                .buffer_frames = 512,
                .period_count = 2,
                .low_latency_mode = false};

            // Dreamcast defaults (AICA)
            constexpr AudioConfig DREAMCAST_CONFIG = {
                .sample_rate = 44100,
                .format = AudioFormat::S16LE,
                .layout = ChannelLayout::Stereo,
                .buffer_frames = 2048,
                .period_count = 2,
                .low_latency_mode = false};

            // PSP defaults
            constexpr AudioConfig PSP_CONFIG = {
                .sample_rate = 44100,
                .format = AudioFormat::S16LE,
                .layout = ChannelLayout::Stereo,
                .buffer_frames = 1024,
                .period_count = 2,
                .low_latency_mode = false};

            // Web Audio defaults
            constexpr AudioConfig WEB_CONFIG = {
                .sample_rate = 48000,
                .format = AudioFormat::F32LE,
                .layout = ChannelLayout::Stereo,
                .buffer_frames = 256,
                .period_count = 2,
                .low_latency_mode = true};
        }

    } // namespace hal
} // namespace flight

#endif // FLIGHT_HAL_AUDIO_DRIVER_HPP
