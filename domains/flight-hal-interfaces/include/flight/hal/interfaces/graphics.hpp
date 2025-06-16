/**
 * @file graphics.hpp
 * @brief Flight HAL Elite Graphics Interface
 * 
 * Unified graphics interface spanning from Dreamcast's PowerVR2 fixed-function GPU
 * to modern programmable pipelines (Vulkan, Metal, D3D12). Provides command buffer
 * abstraction, resource management, and performance optimization across 25+ years
 * of graphics evolution.
 */

#pragma once

#include "../core/driver_registry.hpp"
#include "../core/hal_result.hpp"
#include "../core/hal_capabilities.hpp"
#include "memory.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>
#include <string_view>
#include <memory>
#include <array>

namespace flight::hal {

/**
 * @brief Graphics-specific capability enumeration
 * 
 * Extends the base HALCapability system with graphics-specific capabilities
 * for runtime feature detection and graceful degradation.
 */
enum class GraphicsCapability : uint32_t {
    // Pipeline capabilities
    FixedFunctionPipeline = 1 << 0,    ///< Fixed-function transform & lighting (PowerVR2)
    ProgrammableShaders = 1 << 1,      ///< Vertex/Fragment shader support
    ComputeShaders = 1 << 2,           ///< Compute shader capabilities
    GeometryShaders = 1 << 3,          ///< Geometry shader support
    TessellationShaders = 1 << 4,      ///< Tessellation shader support
    
    // Rendering capabilities
    TileBasedRendering = 1 << 5,       ///< Tile-based deferred rendering (PowerVR, mobile)
    ImmediateModeRendering = 1 << 6,   ///< Traditional immediate mode rendering
    MultipleRenderTargets = 1 << 7,    ///< Multiple render target support
    DepthBuffer = 1 << 8,              ///< Hardware depth buffering
    StencilBuffer = 1 << 9,            ///< Hardware stencil buffering
    
    // Texture capabilities
    TextureCompression = 1 << 10,      ///< Hardware texture compression
    MipmapGeneration = 1 << 11,        ///< Automatic mipmap generation
    CubeMapping = 1 << 12,             ///< Cube map texture support
    VolumeTextures = 1 << 13,          ///< 3D texture support
    FloatingPointTextures = 1 << 14,   ///< Floating-point texture formats
    
    // Advanced features
    MultiThreadedCommands = 1 << 15,   ///< Multi-threaded command recording
    ExplicitSynchronization = 1 << 16, ///< Explicit GPU synchronization
    UnifiedMemory = 1 << 17,           ///< Unified CPU/GPU memory (Metal-style)
    RayTracing = 1 << 18,              ///< Hardware ray tracing support
    VariableRateShading = 1 << 19,     ///< Variable rate shading
    
    // Platform-specific optimizations
    StoreQueues = 1 << 20,             ///< Dreamcast store queue optimization
    DisplayLists = 1 << 21,            ///< PowerVR2 display list support
    CoherentMemory = 1 << 22,          ///< Coherent GPU memory access
    
    // Buffer capabilities
    VertexBufferObjects = 1 << 23,     ///< VBO support
    IndexBufferObjects = 1 << 24,      ///< IBO support
    UniformBufferObjects = 1 << 25,    ///< UBO support
    StorageBufferObjects = 1 << 26     ///< SSBO support
};

/**
 * @brief Graphics resource types
 */
enum class GraphicsResourceType : uint8_t {
    Texture2D = 0,      ///< 2D texture
    TextureCube,        ///< Cube map texture
    Texture3D,          ///< 3D volume texture
    VertexBuffer,       ///< Vertex buffer object
    IndexBuffer,        ///< Index buffer object
    UniformBuffer,      ///< Uniform/constant buffer
    StorageBuffer,      ///< Storage buffer (compute)
    VertexShader,       ///< Vertex shader program
    FragmentShader,     ///< Fragment/pixel shader program
    ComputeShader,      ///< Compute shader program
    GeometryShader,     ///< Geometry shader program
    RenderTarget,       ///< Render target texture
    Framebuffer,        ///< Framebuffer object
    Pipeline            ///< Graphics pipeline state
};

/**
 * @brief Texture formats supporting various hardware capabilities
 */
enum class TextureFormat : uint16_t {
    // Basic formats (PowerVR2 compatible)
    RGB565 = 0,         ///< 16-bit RGB (5:6:5)
    RGBA4444,           ///< 16-bit RGBA (4:4:4:4)
    RGBA1555,           ///< 16-bit RGBA (1:5:5:5)
    RGB888,             ///< 24-bit RGB
    RGBA8888,           ///< 32-bit RGBA
    
    // Compressed formats
    DXT1,               ///< S3TC DXT1 compression
    DXT3,               ///< S3TC DXT3 compression
    DXT5,               ///< S3TC DXT5 compression
    PVRTC_2BPP,         ///< PowerVR texture compression 2bpp
    PVRTC_4BPP,         ///< PowerVR texture compression 4bpp
    
    // Floating-point formats
    R16F,               ///< 16-bit float red
    RG16F,              ///< 16-bit float RG
    RGBA16F,            ///< 16-bit float RGBA
    R32F,               ///< 32-bit float red
    RG32F,              ///< 32-bit float RG
    RGBA32F,            ///< 32-bit float RGBA
    
    // Depth/stencil formats
    Depth16,            ///< 16-bit depth
    Depth24,            ///< 24-bit depth
    Depth32F,           ///< 32-bit float depth
    Depth24Stencil8,    ///< 24-bit depth + 8-bit stencil
    Depth32FStencil8    ///< 32-bit float depth + 8-bit stencil
};

/**
 * @brief Buffer usage patterns for optimization
 */
enum class BufferUsage : uint8_t {
    Static = 0,         ///< Set once, used many times
    Dynamic,            ///< Updated occasionally
    Stream,             ///< Updated every frame
    Staging             ///< CPU-GPU transfer staging
};

/**
 * @brief Primitive topology types
 */
enum class PrimitiveTopology : uint8_t {
    PointList = 0,      ///< Individual points
    LineList,           ///< Individual lines
    LineStrip,          ///< Connected line strip
    TriangleList,       ///< Individual triangles
    TriangleStrip,      ///< Connected triangle strip
    TriangleFan         ///< Triangle fan (PowerVR2 optimized)
};

/**
 * @brief Graphics command types for command buffer abstraction
 */
enum class GraphicsCommandType : uint8_t {
    SetRenderState = 0, ///< Set rendering state (blend, depth, etc.)
    BindResource,       ///< Bind texture, buffer, or shader
    DrawPrimitive,      ///< Issue draw call
    DrawIndexed,        ///< Issue indexed draw call
    BeginRenderPass,    ///< Begin render pass (modern APIs)
    EndRenderPass,      ///< End render pass
    CopyResource,       ///< Copy between resources
    ComputeDispatch,    ///< Dispatch compute shader
    SetViewport,        ///< Set viewport transformation
    SetScissor,         ///< Set scissor rectangle
    ClearRenderTarget,  ///< Clear render target
    ClearDepthStencil,  ///< Clear depth/stencil buffer
    Present             ///< Present frame to display
};

/**
 * @brief Blend operation types
 */
enum class BlendOp : uint8_t {
    Add = 0,            ///< source + destination
    Subtract,           ///< source - destination
    ReverseSubtract,    ///< destination - source
    Min,                ///< min(source, destination)
    Max                 ///< max(source, destination)
};

/**
 * @brief Blend factor types
 */
enum class BlendFactor : uint8_t {
    Zero = 0,           ///< (0, 0, 0, 0)
    One,                ///< (1, 1, 1, 1)
    SrcColor,           ///< source color
    InvSrcColor,        ///< 1 - source color
    SrcAlpha,           ///< source alpha
    InvSrcAlpha,        ///< 1 - source alpha
    DstColor,           ///< destination color
    InvDstColor,        ///< 1 - destination color
    DstAlpha,           ///< destination alpha
    InvDstAlpha         ///< 1 - destination alpha
};

/**
 * @brief Comparison function types
 */
enum class CompareFunc : uint8_t {
    Never = 0,          ///< Never pass
    Less,               ///< Pass if less
    Equal,              ///< Pass if equal
    LessEqual,          ///< Pass if less or equal
    Greater,            ///< Pass if greater
    NotEqual,           ///< Pass if not equal
    GreaterEqual,       ///< Pass if greater or equal
    Always              ///< Always pass
};

/**
 * @brief Cull mode types
 */
enum class CullMode : uint8_t {
    None = 0,           ///< No culling
    Front,              ///< Cull front faces
    Back                ///< Cull back faces
};

/**
 * @brief Fill mode types
 */
enum class FillMode : uint8_t {
    Solid = 0,          ///< Solid fill
    Wireframe           ///< Wireframe fill
};

// Forward declarations
class IGraphicsResource;
class ICommandBuffer;
class IGraphicsDevice;
class IPipelineState;

/**
 * @brief Graphics resource handle
 * 
 * Lightweight handle for graphics resources with automatic lifetime management.
 */
struct GraphicsResourceHandle {
    uint32_t id;                    ///< Unique resource identifier
    GraphicsResourceType type;      ///< Resource type
    uint32_t generation;            ///< Generation counter for validation
    
    bool is_valid() const { return id != 0; }
    void invalidate() { id = 0; generation = 0; }
};

/**
 * @brief Texture creation parameters
 */
struct TextureDescriptor {
    uint32_t width;                 ///< Texture width in pixels
    uint32_t height;                ///< Texture height in pixels
    uint32_t depth;                 ///< Texture depth (1 for 2D textures)
    uint32_t mip_levels;            ///< Number of mipmap levels (1 for no mipmaps)
    TextureFormat format;           ///< Pixel format
    BufferUsage usage;              ///< Usage pattern
    bool generate_mipmaps;          ///< Automatically generate mipmaps
    std::string_view debug_name;    ///< Debug name for graphics debugging
};

/**
 * @brief Buffer creation parameters
 */
struct BufferDescriptor {
    size_t size;                    ///< Buffer size in bytes
    BufferUsage usage;              ///< Usage pattern
    GraphicsResourceType type;      ///< Buffer type (vertex, index, uniform, etc.)
    size_t stride;                  ///< Element stride for structured buffers
    std::string_view debug_name;    ///< Debug name for graphics debugging
};

/**
 * @brief Shader creation parameters
 */
struct ShaderDescriptor {
    GraphicsResourceType type;      ///< Shader type
    const void* bytecode;           ///< Compiled shader bytecode
    size_t bytecode_size;           ///< Bytecode size in bytes
    std::string_view source;        ///< Optional source code (for runtime compilation)
    std::string_view entry_point;   ///< Entry point function name
    std::string_view debug_name;    ///< Debug name for graphics debugging
};

/**
 * @brief Viewport parameters
 */
struct Viewport {
    float x, y;                     ///< Top-left corner
    float width, height;            ///< Viewport dimensions
    float min_depth, max_depth;     ///< Depth range [0,1]
};

/**
 * @brief Scissor rectangle
 */
struct ScissorRect {
    int32_t x, y;                   ///< Top-left corner
    uint32_t width, height;         ///< Rectangle dimensions
};

/**
 * @brief Render state for fixed-function and programmable pipelines
 */
struct RenderState {
    // Blend state
    bool blend_enable;              ///< Enable alpha blending
    BlendFactor src_blend;          ///< Source blend factor
    BlendFactor dst_blend;          ///< Destination blend factor
    BlendOp blend_op;               ///< Blend operation
    BlendFactor src_blend_alpha;    ///< Source alpha blend factor
    BlendFactor dst_blend_alpha;    ///< Destination alpha blend factor
    BlendOp blend_op_alpha;         ///< Alpha blend operation
    
    // Depth/stencil state
    bool depth_test_enable;         ///< Enable depth testing
    bool depth_write_enable;        ///< Enable depth writing
    CompareFunc depth_func;         ///< Depth comparison function
    bool stencil_enable;            ///< Enable stencil testing
    
    // Rasterizer state
    CullMode cull_mode;             ///< Face culling mode
    FillMode fill_mode;             ///< Fill mode (solid/wireframe)
    bool scissor_enable;            ///< Enable scissor testing
    
    // PowerVR2 specific states
    bool fog_enable;                ///< Enable fog (PowerVR2)
    uint32_t fog_color;             ///< Fog color (RGBA)
    float fog_density;              ///< Fog density
};

/**
 * @brief Draw command parameters
 */
struct DrawCommand {
    PrimitiveTopology topology;     ///< Primitive topology
    uint32_t vertex_count;          ///< Number of vertices
    uint32_t vertex_offset;         ///< First vertex offset
    uint32_t instance_count;        ///< Number of instances (1 for non-instanced)
    uint32_t instance_offset;       ///< First instance offset
};

/**
 * @brief Indexed draw command parameters
 */
struct DrawIndexedCommand {
    PrimitiveTopology topology;     ///< Primitive topology
    uint32_t index_count;           ///< Number of indices
    uint32_t index_offset;          ///< First index offset
    int32_t vertex_offset;          ///< Vertex buffer offset
    uint32_t instance_count;        ///< Number of instances (1 for non-instanced)
    uint32_t instance_offset;       ///< First instance offset
};

/**
 * @brief Graphics device capabilities and information
 */
struct GraphicsDeviceInfo {
    std::string device_name;        ///< GPU/device name
    std::string vendor_name;        ///< Vendor name
    std::string driver_version;     ///< Driver version string
    uint32_t capability_mask;       ///< Supported GraphicsCapability bitmask
    
    // Resource limits
    uint32_t max_texture_size;      ///< Maximum texture dimension
    uint32_t max_texture_array_size; ///< Maximum texture array size
    uint32_t max_render_targets;    ///< Maximum simultaneous render targets
    uint32_t max_vertex_attributes; ///< Maximum vertex attributes
    uint32_t max_uniform_buffers;   ///< Maximum uniform buffers per stage
    
    // Memory information
    size_t total_graphics_memory;   ///< Total graphics memory (0 if unknown)
    size_t available_graphics_memory; ///< Available graphics memory (0 if unknown)
    
    // Performance characteristics
    PerformanceTier performance_tier; ///< Performance tier classification
    bool unified_memory;            ///< CPU/GPU memory is unified
    bool coherent_memory;           ///< Memory is coherent between CPU/GPU
};

/**
 * @brief Command buffer recording statistics
 */
struct CommandBufferStats {
    uint32_t command_count;         ///< Total commands recorded
    uint32_t draw_call_count;       ///< Number of draw calls
    uint32_t state_change_count;    ///< Number of state changes
    uint32_t resource_bind_count;   ///< Number of resource bindings
    size_t memory_used;             ///< Memory used by command buffer
};

/**
 * @brief Graphics performance statistics
 */
struct GraphicsStats {
    uint64_t frame_count;           ///< Total frames rendered
    uint64_t draw_call_count;       ///< Total draw calls
    uint64_t triangle_count;        ///< Total triangles rendered
    uint64_t texture_switches;      ///< Texture binding changes
    uint64_t shader_switches;       ///< Shader program changes
    uint64_t render_state_changes;  ///< Render state changes
    
    // Memory statistics
    size_t texture_memory_used;     ///< Texture memory usage
    size_t buffer_memory_used;      ///< Buffer memory usage
    size_t total_graphics_memory_used; ///< Total graphics memory usage
    
    // Performance metrics
    double average_frame_time_ms;   ///< Average frame time in milliseconds
    double gpu_utilization;         ///< GPU utilization percentage (0-100)
};

/**
 * @brief Graphics resource interface
 * 
 * Base interface for all graphics resources (textures, buffers, shaders).
 */
class IGraphicsResource {
public:
    virtual ~IGraphicsResource() = default;
    
    /**
     * @brief Get resource type
     * @return Type of this resource
     */
    virtual GraphicsResourceType get_type() const = 0;
    
    /**
     * @brief Get resource handle
     * @return Handle for this resource
     */
    virtual GraphicsResourceHandle get_handle() const = 0;
    
    /**
     * @brief Get resource size in bytes
     * @return Size of resource in bytes
     */
    virtual size_t get_size() const = 0;
    
    /**
     * @brief Check if resource is valid
     * @return true if resource is valid and ready to use
     */
    virtual bool is_valid() const = 0;
    
    /**
     * @brief Get debug name
     * @return Debug name for this resource
     */
    virtual std::string_view get_debug_name() const = 0;
    
    /**
     * @brief Set debug name
     * @param name Debug name to set
     */
    virtual void set_debug_name(std::string_view name) = 0;
};

/**
 * @brief Command buffer interface
 * 
 * Abstracts command recording for both modern APIs (Vulkan, Metal, D3D12) 
 * and legacy APIs (PowerVR2 display lists, OpenGL immediate mode).
 */
class ICommandBuffer {
public:
    virtual ~ICommandBuffer() = default;
    
    // === Command Recording ===
    
    /**
     * @brief Begin command recording
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> begin() = 0;
    
    /**
     * @brief End command recording
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> end() = 0;
    
    /**
     * @brief Reset command buffer for reuse
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> reset() = 0;
    
    // === Render State Commands ===
    
    /**
     * @brief Set render state
     * @param state Render state to set
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_render_state(const RenderState& state) = 0;
    
    /**
     * @brief Set viewport
     * @param viewport Viewport parameters
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_viewport(const Viewport& viewport) = 0;
    
    /**
     * @brief Set scissor rectangle
     * @param scissor Scissor rectangle
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_scissor(const ScissorRect& scissor) = 0;
    
    // === Resource Binding ===
    
    /**
     * @brief Bind texture to slot
     * @param slot Texture slot index
     * @param texture Texture resource handle
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> bind_texture(uint32_t slot, GraphicsResourceHandle texture) = 0;
    
    /**
     * @brief Bind vertex buffer
     * @param slot Vertex buffer slot
     * @param buffer Vertex buffer handle
     * @param stride Vertex stride in bytes
     * @param offset Offset in buffer
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> bind_vertex_buffer(uint32_t slot, GraphicsResourceHandle buffer, 
                                              uint32_t stride, uint32_t offset = 0) = 0;
    
    /**
     * @brief Bind index buffer
     * @param buffer Index buffer handle
     * @param offset Offset in buffer
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> bind_index_buffer(GraphicsResourceHandle buffer, uint32_t offset = 0) = 0;
    
    /**
     * @brief Bind uniform buffer
     * @param slot Uniform buffer slot
     * @param buffer Uniform buffer handle
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> bind_uniform_buffer(uint32_t slot, GraphicsResourceHandle buffer) = 0;
    
    /**
     * @brief Bind shader program
     * @param vertex_shader Vertex shader handle
     * @param fragment_shader Fragment shader handle (optional for fixed-function)
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> bind_shaders(GraphicsResourceHandle vertex_shader,
                                        GraphicsResourceHandle fragment_shader = {}) = 0;
    
    /**
     * @brief Bind pipeline state (for modern APIs)
     * @param pipeline Pipeline state handle
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> bind_pipeline(GraphicsResourceHandle pipeline) = 0;
    
    // === Drawing Commands ===
    
    /**
     * @brief Draw primitives
     * @param command Draw command parameters
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> draw(const DrawCommand& command) = 0;
    
    /**
     * @brief Draw indexed primitives
     * @param command Indexed draw command parameters
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> draw_indexed(const DrawIndexedCommand& command) = 0;
    
    // === Clear Commands ===
    
    /**
     * @brief Clear render target
     * @param color Clear color (RGBA)
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> clear_render_target(const std::array<float, 4>& color) = 0;
    
    /**
     * @brief Clear depth/stencil buffer
     * @param depth Depth clear value
     * @param stencil Stencil clear value
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> clear_depth_stencil(float depth, uint8_t stencil) = 0;
    
    // === Resource Operations ===
    
    /**
     * @brief Copy between resources
     * @param src Source resource handle
     * @param dst Destination resource handle
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> copy_resource(GraphicsResourceHandle src, GraphicsResourceHandle dst) = 0;
    
    // === Modern API Features ===
    
    /**
     * @brief Begin render pass (modern APIs)
     * @param render_targets Array of render target handles
     * @param depth_stencil Depth/stencil target handle (optional)
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> begin_render_pass(const std::vector<GraphicsResourceHandle>& render_targets,
                                             GraphicsResourceHandle depth_stencil = {}) = 0;
    
    /**
     * @brief End render pass
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> end_render_pass() = 0;
    
    /**
     * @brief Dispatch compute shader (if supported)
     * @param group_count_x Number of work groups in X dimension
     * @param group_count_y Number of work groups in Y dimension
     * @param group_count_z Number of work groups in Z dimension
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> dispatch_compute(uint32_t group_count_x, 
                                            uint32_t group_count_y, 
                                            uint32_t group_count_z) = 0;
    
    // === Statistics and Debug ===
    
    /**
     * @brief Get command buffer statistics
     * @return Statistics for this command buffer
     */
    virtual CommandBufferStats get_stats() const = 0;
    
    /**
     * @brief Check if command buffer is recording
     * @return true if currently recording commands
     */
    virtual bool is_recording() const = 0;
    
    /**
     * @brief Set debug marker
     * @param name Debug marker name
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_debug_marker(std::string_view name) = 0;
    
    /**
     * @brief Begin debug group
     * @param name Debug group name
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> begin_debug_group(std::string_view name) = 0;
    
    /**
     * @brief End debug group
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> end_debug_group() = 0;
};

/**
 * @brief Enhanced graphics interface
 * 
 * Comprehensive graphics system supporting both fixed-function (PowerVR2) and
 * modern programmable pipelines (Vulkan, Metal, D3D12). Provides unified command
 * buffer abstraction and resource management across diverse graphics architectures.
 */
class IGraphicsInterface : public IHALInterface, public ICapabilityProvider {
public:
    virtual ~IGraphicsInterface() = default;
    
    // === Device Management ===
    
    /**
     * @brief Initialize graphics device
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> initialize() = 0;
    
    /**
     * @brief Shutdown graphics device
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> shutdown() = 0;
    
    /**
     * @brief Get device information
     * @return Device capabilities and information
     */
    virtual const GraphicsDeviceInfo& get_device_info() const = 0;
    
    /**
     * @brief Check if graphics system is initialized
     * @return true if initialized and ready
     */
    virtual bool is_initialized() const = 0;
    
    // === Command Buffer Management ===
    
    /**
     * @brief Create command buffer
     * @return HALResult containing command buffer pointer on success
     */
    virtual HALResult<std::unique_ptr<ICommandBuffer>> create_command_buffer() = 0;
    
    /**
     * @brief Submit command buffer for execution
     * @param command_buffer Command buffer to submit
     * @param wait_for_completion Wait for GPU completion before returning
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> submit_command_buffer(ICommandBuffer* command_buffer,
                                                 bool wait_for_completion = false) = 0;
    
    /**
     * @brief Submit multiple command buffers
     * @param command_buffers Array of command buffers to submit
     * @param wait_for_completion Wait for GPU completion before returning
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> submit_command_buffers(const std::vector<ICommandBuffer*>& command_buffers,
                                                  bool wait_for_completion = false) = 0;
    
    // === Resource Creation ===
    
    /**
     * @brief Create texture resource
     * @param descriptor Texture creation parameters
     * @param initial_data Initial texture data (optional)
     * @return HALResult containing resource handle on success
     */
    virtual HALResult<GraphicsResourceHandle> create_texture(const TextureDescriptor& descriptor,
                                                            const void* initial_data = nullptr) = 0;
    
    /**
     * @brief Create buffer resource
     * @param descriptor Buffer creation parameters  
     * @param initial_data Initial buffer data (optional)
     * @return HALResult containing resource handle on success
     */
    virtual HALResult<GraphicsResourceHandle> create_buffer(const BufferDescriptor& descriptor,
                                                           const void* initial_data = nullptr) = 0;
    
    /**
     * @brief Create shader resource
     * @param descriptor Shader creation parameters
     * @return HALResult containing resource handle on success
     */
    virtual HALResult<GraphicsResourceHandle> create_shader(const ShaderDescriptor& descriptor) = 0;
    
    /**
     * @brief Create render target
     * @param width Render target width
     * @param height Render target height
     * @param format Pixel format
     * @param debug_name Debug name (optional)
     * @return HALResult containing resource handle on success
     */
    virtual HALResult<GraphicsResourceHandle> create_render_target(uint32_t width, uint32_t height,
                                                                  TextureFormat format,
                                                                  std::string_view debug_name = {}) = 0;
    
    /**
     * @brief Create framebuffer with render targets
     * @param render_targets Array of render target handles
     * @param depth_stencil Depth/stencil target (optional)
     * @param debug_name Debug name (optional)
     * @return HALResult containing resource handle on success
     */
    virtual HALResult<GraphicsResourceHandle> create_framebuffer(const std::vector<GraphicsResourceHandle>& render_targets,
                                                                GraphicsResourceHandle depth_stencil = {},
                                                                std::string_view debug_name = {}) = 0;
    
    // === Resource Management ===
    
    /**
     * @brief Destroy resource
     * @param resource Resource handle to destroy
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> destroy_resource(GraphicsResourceHandle resource) = 0;
    
    /**
     * @brief Get resource object
     * @param resource Resource handle
     * @return HALResult containing resource pointer on success
     */
    virtual HALResult<IGraphicsResource*> get_resource(GraphicsResourceHandle resource) = 0;
    
    /**
     * @brief Update buffer data
     * @param buffer Buffer resource handle
     * @param data New data to upload
     * @param size Size of data in bytes
     * @param offset Offset in buffer to start update
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> update_buffer(GraphicsResourceHandle buffer, 
                                         const void* data, 
                                         size_t size, 
                                         size_t offset = 0) = 0;
    
    /**
     * @brief Update texture data
     * @param texture Texture resource handle
     * @param data New pixel data
     * @param width Width of update region
     * @param height Height of update region
     * @param x X offset in texture
     * @param y Y offset in texture
     * @param mip_level Mipmap level to update
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> update_texture(GraphicsResourceHandle texture,
                                          const void* data,
                                          uint32_t width, uint32_t height,
                                          uint32_t x = 0, uint32_t y = 0,
                                          uint32_t mip_level = 0) = 0;
    
    /**
     * @brief Generate mipmaps for texture
     * @param texture Texture resource handle
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> generate_mipmaps(GraphicsResourceHandle texture) = 0;
    
    // === GPU Synchronization ===
    
    /**
     * @brief Wait for GPU to complete all submitted work
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> wait_for_gpu() = 0;
    
    /**
     * @brief Check if GPU is idle
     * @return true if GPU has no pending work
     */
    virtual bool is_gpu_idle() const = 0;
    
    /**
     * @brief Flush pending GPU commands
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> flush() = 0;
    
    // === Present and Display ===
    
    /**
     * @brief Present current frame to display
     * @param vsync Enable vertical synchronization
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> present(bool vsync = true) = 0;
    
    /**
     * @brief Get current backbuffer
     * @return HALResult containing backbuffer handle on success
     */
    virtual HALResult<GraphicsResourceHandle> get_backbuffer() = 0;
    
    /**
     * @brief Resize backbuffer
     * @param width New width
     * @param height New height
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> resize_backbuffer(uint32_t width, uint32_t height) = 0;
    
    // === Capability Queries ===
    
    /**
     * @brief Check if texture format is supported
     * @param format Texture format to check
     * @return true if format is supported
     */
    virtual bool supports_texture_format(TextureFormat format) const = 0;
    
    /**
     * @brief Check if graphics capability is supported
     * @param capability Graphics capability to check
     * @return true if capability is supported
     */
    virtual bool supports_graphics_capability(GraphicsCapability capability) const = 0;
    
    /**
     * @brief Get maximum texture size
     * @return Maximum texture dimension
     */
    virtual uint32_t get_max_texture_size() const = 0;
    
    /**
     * @brief Get maximum render targets
     * @return Maximum simultaneous render targets
     */
    virtual uint32_t get_max_render_targets() const = 0;
    
    /**
     * @brief Get maximum vertex attributes
     * @return Maximum vertex attributes
     */
    virtual uint32_t get_max_vertex_attributes() const = 0;
    
    // === Statistics and Performance ===
    
    /**
     * @brief Get graphics performance statistics
     * @return Performance statistics structure
     */
    virtual GraphicsStats get_stats() const = 0;
    
    /**
     * @brief Reset performance statistics
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> reset_stats() = 0;
    
    /**
     * @brief Get GPU memory usage
     * @return HALResult containing memory usage in bytes
     */
    virtual HALResult<size_t> get_gpu_memory_usage() const = 0;
    
    /**
     * @brief Get available GPU memory
     * @return HALResult containing available memory in bytes
     */
    virtual HALResult<size_t> get_available_gpu_memory() const = 0;
    
    // === Debug and Profiling ===
    
    /**
     * @brief Begin GPU timing query
     * @param name Query name for identification
     * @return HALResult with query ID on success
     */
    virtual HALResult<uint32_t> begin_gpu_timing(std::string_view name) = 0;
    
    /**
     * @brief End GPU timing query
     * @param query_id Query ID from begin_gpu_timing
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> end_gpu_timing(uint32_t query_id) = 0;
    
    /**
     * @brief Get GPU timing result
     * @param query_id Query ID
     * @return HALResult containing timing in microseconds
     */
    virtual HALResult<uint64_t> get_gpu_timing_result(uint32_t query_id) = 0;
    
    /**
     * @brief Capture graphics frame for debugging
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> capture_frame() = 0;
    
    // === Platform-Specific Extensions ===
    
    /**
     * @brief Get platform-specific extension interface
     * @param extension_name Name of extension to query
     * @return Pointer to extension interface (nullptr if not supported)
     */
    virtual void* get_extension_interface(std::string_view extension_name) = 0;
    
    /**
     * @brief PowerVR2 specific: Get store queue interface
     * @return Pointer to store queue interface (PowerVR2 only)
     */
    virtual void* get_store_queue_interface() = 0;
    
    /**
     * @brief PowerVR2 specific: Get display list interface
     * @return Pointer to display list interface (PowerVR2 only)
     */
    virtual void* get_display_list_interface() = 0;
    
    // === IHALInterface implementation ===
    std::string_view get_interface_name() const override {
        return "graphics";
    }
};

// === Utility Functions ===

/**
 * @brief Graphics utility functions
 */
namespace graphics {

/**
 * @brief Convert graphics capability to string
 * @param capability Graphics capability to convert
 * @return String representation
 */
const char* to_string(GraphicsCapability capability);

/**
 * @brief Convert texture format to string
 * @param format Texture format to convert
 * @return String representation
 */
const char* to_string(TextureFormat format);

/**
 * @brief Convert primitive topology to string
 * @param topology Primitive topology to convert
 * @return String representation
 */
const char* to_string(PrimitiveTopology topology);

/**
 * @brief Convert blend operation to string
 * @param op Blend operation to convert
 * @return String representation
 */
const char* to_string(BlendOp op);

/**
 * @brief Convert blend factor to string
 * @param factor Blend factor to convert
 * @return String representation
 */
const char* to_string(BlendFactor factor);

/**
 * @brief Get bytes per pixel for texture format
 * @param format Texture format
 * @return Number of bytes per pixel
 */
uint32_t get_bytes_per_pixel(TextureFormat format);

/**
 * @brief Check if texture format is compressed
 * @param format Texture format to check
 * @return true if format is compressed
 */
bool is_compressed_format(TextureFormat format);

/**
 * @brief Check if texture format is depth/stencil
 * @param format Texture format to check
 * @return true if format is depth/stencil
 */
bool is_depth_stencil_format(TextureFormat format);

/**
 * @brief Calculate texture memory size
 * @param width Texture width
 * @param height Texture height
 * @param format Texture format
 * @param mip_levels Number of mipmap levels
 * @return Total memory size in bytes
 */
size_t calculate_texture_size(uint32_t width, uint32_t height, 
                             TextureFormat format, uint32_t mip_levels = 1);

/**
 * @brief Create default texture descriptor
 * @param width Texture width
 * @param height Texture height
 * @param format Texture format
 * @return Configured TextureDescriptor
 */
TextureDescriptor make_texture_descriptor(uint32_t width, uint32_t height, 
                                         TextureFormat format = TextureFormat::RGBA8888);

/**
 * @brief Create default buffer descriptor
 * @param size Buffer size in bytes
 * @param type Buffer type
 * @param usage Usage pattern
 * @return Configured BufferDescriptor
 */
BufferDescriptor make_buffer_descriptor(size_t size, 
                                       GraphicsResourceType type = GraphicsResourceType::VertexBuffer,
                                       BufferUsage usage = BufferUsage::Static);

/**
 * @brief Create default render state
 * @return Default render state configuration
 */
RenderState make_default_render_state();

/**
 * @brief Create viewport from dimensions
 * @param width Viewport width
 * @param height Viewport height
 * @return Configured viewport
 */
Viewport make_viewport(float width, float height);

/**
 * @brief Create scissor rectangle
 * @param x X coordinate
 * @param y Y coordinate
 * @param width Rectangle width
 * @param height Rectangle height
 * @return Configured scissor rectangle
 */
ScissorRect make_scissor_rect(int32_t x, int32_t y, uint32_t width, uint32_t height);

} // namespace graphics

} // namespace flight::hal
