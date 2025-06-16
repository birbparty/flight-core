#ifndef FLIGHT_HAL_VIDEO_DRIVER_HPP
#define FLIGHT_HAL_VIDEO_DRIVER_HPP

#include "driver.hpp"
#include "result.hpp"
#include <cstdint>
#include <vector>
#include <functional>

namespace flight
{
    namespace hal
    {

        // Graphics API types
        enum class GraphicsAPI : uint32_t
        {
            None,
            OpenGL,
            OpenGLES2,
            OpenGLES3,
            Vulkan,
            Metal,
            DirectX11,
            DirectX12,
            WebGL,
            WebGL2,
            PowerVR2, // Dreamcast
            PSPGu,    // PSP
            Software  // Software renderer fallback
        };

        // Pixel formats
        enum class PixelFormat : uint32_t
        {
            Unknown,
            // 8-bit formats
            R8,
            // 16-bit formats
            R5G6B5,
            R5G5B5A1,
            R4G4B4A4,
            // 24-bit formats
            R8G8B8,
            B8G8R8,
            // 32-bit formats
            R8G8B8A8,
            B8G8R8A8,
            A8R8G8B8,
            A8B8G8R8,
            // Special formats
            YUV422, // Dreamcast
            DXT1,   // Compressed
            DXT3,   // Compressed
            DXT5,   // Compressed
            PVRTC2, // PowerVR compressed
            PVRTC4, // PowerVR compressed
            ETC1,   // Android/embedded
            ETC2    // Android/embedded
        };

        // Primitive types
        enum class PrimitiveType : uint32_t
        {
            Points,
            Lines,
            LineStrip,
            Triangles,
            TriangleStrip,
            TriangleFan
        };

        // Texture handle
        struct TextureHandle
        {
            uint32_t id;
            bool operator==(const TextureHandle &other) const { return id == other.id; }
            bool operator!=(const TextureHandle &other) const { return id != other.id; }
        };

        // Buffer handle
        struct BufferHandle
        {
            uint32_t id;
            bool operator==(const BufferHandle &other) const { return id == other.id; }
            bool operator!=(const BufferHandle &other) const { return id != other.id; }
        };

        // Shader handle
        struct ShaderHandle
        {
            uint32_t id;
            bool operator==(const ShaderHandle &other) const { return id == other.id; }
            bool operator!=(const ShaderHandle &other) const { return id != other.id; }
        };

        // Invalid handles
        constexpr TextureHandle INVALID_TEXTURE = {0};
        constexpr BufferHandle INVALID_BUFFER = {0};
        constexpr ShaderHandle INVALID_SHADER = {0};

        // Display mode
        struct DisplayMode
        {
            uint32_t width;
            uint32_t height;
            uint32_t refresh_rate;
            PixelFormat format;
            bool fullscreen;
        };

        // Video configuration
        struct VideoConfig
        {
            uint32_t width;
            uint32_t height;
            bool fullscreen;
            bool vsync;
            uint32_t msaa_samples;
            bool double_buffer;
            bool triple_buffer;
            GraphicsAPI preferred_api;
        };

        // Graphics capabilities (query what the platform supports)
        struct GraphicsCapabilities
        {
            // API support
            std::vector<GraphicsAPI> supported_apis;

            // Texture capabilities
            uint32_t max_texture_size;
            uint32_t max_texture_units;
            std::vector<PixelFormat> supported_formats;
            bool supports_npot_textures;
            bool supports_compressed_textures;
            bool supports_mipmapping;

            // Rendering capabilities
            uint32_t max_render_targets;
            uint32_t max_vertex_attributes;
            uint32_t max_uniform_buffer_size;
            bool supports_hardware_transform;
            bool supports_programmable_shaders;
            bool supports_geometry_shaders;
            bool supports_compute_shaders;
            bool supports_instancing;

            // Memory
            size_t texture_memory;
            size_t vertex_buffer_memory;

            // Performance hints
            bool prefers_immediate_mode; // Older hardware
            bool supports_retained_mode; // Modern hardware
            bool benefits_from_batching;
            uint32_t optimal_batch_size;
        };

        // Texture descriptor
        struct TextureDesc
        {
            uint32_t width;
            uint32_t height;
            uint32_t depth;
            uint32_t mip_levels;
            PixelFormat format;
            bool is_render_target;
            bool is_dynamic;
            const void *initial_data;
            size_t data_size;
        };

        // Buffer descriptor
        struct BufferDesc
        {
            size_t size;
            bool is_dynamic;
            bool is_index_buffer;
            const void *initial_data;
        };

        // Shader descriptor
        struct ShaderDesc
        {
            const char *vertex_source;
            const char *fragment_source;
            const char *geometry_source; // Optional
        };

        // Command buffer for bgfx-style rendering
        class CommandBuffer
        {
        public:
            virtual ~CommandBuffer() = default;

            // State management
            virtual void set_viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
            virtual void set_scissor(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
            virtual void clear(uint32_t color, float depth, uint8_t stencil) = 0;

            // Resource binding
            virtual void set_texture(uint32_t slot, TextureHandle texture) = 0;
            virtual void set_vertex_buffer(BufferHandle buffer, uint32_t stride, uint32_t offset = 0) = 0;
            virtual void set_index_buffer(BufferHandle buffer, uint32_t offset = 0) = 0;
            virtual void set_shader(ShaderHandle shader) = 0;

            // Drawing
            virtual void draw(PrimitiveType type, uint32_t first, uint32_t count) = 0;
            virtual void draw_indexed(PrimitiveType type, uint32_t count, uint32_t offset = 0) = 0;
            virtual void draw_instanced(PrimitiveType type, uint32_t first, uint32_t count, uint32_t instance_count) = 0;

            // Render target management
            virtual void set_render_target(TextureHandle target) = 0;
            virtual void set_render_targets(const TextureHandle *targets, uint32_t count) = 0;

            // Performance markers (for debugging)
            virtual void push_debug_group(const char *name) = 0;
            virtual void pop_debug_group() = 0;
        };

        // Video driver interface (RetroArch pattern)
        class VideoDriver : public Driver
        {
        public:
            // Driver interface
            DriverType type() const override { return DriverType::Video; }

            // Initialize with configuration
            virtual Result<void> initialize_display(const VideoConfig &config) = 0;

            // Capability queries
            virtual GraphicsCapabilities capabilities() const = 0;
            virtual GraphicsAPI current_api() const = 0;
            virtual bool supports_format(PixelFormat format) const = 0;
            virtual bool supports_resolution(uint32_t width, uint32_t height) const = 0;

            // Display management
            virtual std::vector<DisplayMode> get_display_modes() const = 0;
            virtual Result<void> set_display_mode(const DisplayMode &mode) = 0;
            virtual DisplayMode get_current_display_mode() const = 0;

            // Frame management (bgfx pattern)
            virtual Result<CommandBuffer *> begin_frame() = 0;
            virtual Result<void> submit_commands(CommandBuffer *commands) = 0;
            virtual Result<void> end_frame() = 0;
            virtual void present() = 0;

            // Resource creation
            virtual Result<TextureHandle> create_texture(const TextureDesc &desc) = 0;
            virtual Result<BufferHandle> create_buffer(const BufferDesc &desc) = 0;
            virtual Result<ShaderHandle> create_shader(const ShaderDesc &desc) = 0;

            // Resource destruction
            virtual void destroy_texture(TextureHandle handle) = 0;
            virtual void destroy_buffer(BufferHandle handle) = 0;
            virtual void destroy_shader(ShaderHandle handle) = 0;

            // Resource updates
            virtual Result<void> update_texture(TextureHandle handle, const void *data, size_t size) = 0;
            virtual Result<void> update_buffer(BufferHandle handle, const void *data, size_t size, size_t offset = 0) = 0;

            // Synchronization
            virtual void wait_for_idle() = 0;
            virtual void flush() = 0;

            // Platform-specific optimizations
            virtual void optimize_for_platform() = 0;
            virtual void set_performance_mode(uint32_t mode) = 0;

            // Debug/profiling
            virtual uint64_t get_frame_time_ns() const = 0;
            virtual uint32_t get_draw_call_count() const = 0;
            virtual uint32_t get_triangle_count() const = 0;
        };

        // Helper functions for common operations
        inline uint32_t get_pixel_format_size(PixelFormat format)
        {
            switch (format)
            {
            case PixelFormat::R8:
                return 1;
            case PixelFormat::R5G6B5:
            case PixelFormat::R5G5B5A1:
            case PixelFormat::R4G4B4A4:
            case PixelFormat::YUV422:
                return 2;
            case PixelFormat::R8G8B8:
            case PixelFormat::B8G8R8:
                return 3;
            case PixelFormat::R8G8B8A8:
            case PixelFormat::B8G8R8A8:
            case PixelFormat::A8R8G8B8:
            case PixelFormat::A8B8G8R8:
                return 4;
            default:
                return 0;
            }
        }

        inline bool is_compressed_format(PixelFormat format)
        {
            switch (format)
            {
            case PixelFormat::DXT1:
            case PixelFormat::DXT3:
            case PixelFormat::DXT5:
            case PixelFormat::PVRTC2:
            case PixelFormat::PVRTC4:
            case PixelFormat::ETC1:
            case PixelFormat::ETC2:
                return true;
            default:
                return false;
            }
        }

    } // namespace hal
} // namespace flight

#endif // FLIGHT_HAL_VIDEO_DRIVER_HPP
