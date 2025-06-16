# Graphics Interface

**Unified Graphics Rendering from PowerVR2 to Modern GPUs**

The Graphics interface provides a unified command buffer abstraction spanning from Dreamcast's PowerVR2 fixed-function GPU to modern programmable pipelines (Vulkan, Metal, D3D12). It supports both legacy and cutting-edge graphics features with platform-specific optimizations.

## Overview

The Graphics interface is built around four core concepts:

1. **Command Buffers** - Cross-platform command recording and submission
2. **Resource Management** - Unified handling of textures, buffers, and shaders
3. **Pipeline Abstraction** - Support for both fixed-function and programmable pipelines
4. **Platform Optimization** - Leveraging platform-specific features (store queues, display lists, etc.)

```cpp
#include <flight/hal/interfaces/graphics.hpp>
using namespace flight::hal;

// Initialize graphics
auto graphics = platform->get_graphics();
HAL_TRY(graphics->initialize());

// Create command buffer
auto cmd_buffer = graphics->create_command_buffer();
HAL_TRY(cmd_buffer.value()->begin());

// Record commands
cmd_buffer.value()->clear_render_target({0.0f, 0.0f, 0.0f, 1.0f});
// ... draw calls ...

HAL_TRY(cmd_buffer.value()->end());
HAL_TRY(graphics->submit_command_buffer(cmd_buffer.value().get()));
```

## Quick Reference

| Operation | Method | Purpose |
|-----------|--------|---------|
| **Initialization** | `initialize()` | Initialize graphics device |
| **Command Buffer** | `create_command_buffer()` | Create command recording buffer |
| **Resource Creation** | `create_texture()`, `create_buffer()` | Create graphics resources |
| **Submission** | `submit_command_buffer()` | Submit commands for execution |
| **Present** | `present()` | Display frame to screen |
| **Synchronization** | `wait_for_gpu()` | Wait for GPU completion |

## Core Concepts

### Graphics Capabilities

The interface provides runtime feature detection across diverse graphics hardware:

```cpp
enum class GraphicsCapability : uint32_t {
    // Pipeline capabilities
    FixedFunctionPipeline = 1 << 0,    // Fixed-function T&L (PowerVR2)
    ProgrammableShaders = 1 << 1,      // Vertex/Fragment shader support
    ComputeShaders = 1 << 2,           // Compute shader capabilities
    GeometryShaders = 1 << 3,          // Geometry shader support
    TessellationShaders = 1 << 4,      // Tessellation shader support
    
    // Rendering capabilities
    TileBasedRendering = 1 << 5,       // Tile-based deferred rendering
    ImmediateModeRendering = 1 << 6,   // Traditional immediate mode
    MultipleRenderTargets = 1 << 7,    // Multiple render target support
    DepthBuffer = 1 << 8,              // Hardware depth buffering
    StencilBuffer = 1 << 9,            // Hardware stencil buffering
    
    // Advanced features
    MultiThreadedCommands = 1 << 15,   // Multi-threaded command recording
    ExplicitSynchronization = 1 << 16, // Explicit GPU synchronization
    UnifiedMemory = 1 << 17,           // Unified CPU/GPU memory
    RayTracing = 1 << 18,              // Hardware ray tracing
    VariableRateShading = 1 << 19,     // Variable rate shading
    
    // Platform-specific optimizations
    StoreQueues = 1 << 20,             // Dreamcast store queue optimization
    DisplayLists = 1 << 21,            // PowerVR2 display list support
};
```

### Resource Types

Graphics resources are managed through a unified handle system:

```cpp
enum class GraphicsResourceType : uint8_t {
    Texture2D = 0,      // 2D texture
    TextureCube,        // Cube map texture
    Texture3D,          // 3D volume texture
    VertexBuffer,       // Vertex buffer object
    IndexBuffer,        // Index buffer object
    UniformBuffer,      // Uniform/constant buffer
    StorageBuffer,      // Storage buffer (compute)
    VertexShader,       // Vertex shader program
    FragmentShader,     // Fragment/pixel shader program
    ComputeShader,      // Compute shader program
    GeometryShader,     // Geometry shader program
    RenderTarget,       // Render target texture
    Framebuffer,        // Framebuffer object
    Pipeline            // Graphics pipeline state
};
```

### Texture Formats

Support for diverse texture formats across hardware generations:

```cpp
enum class TextureFormat : uint16_t {
    // Basic formats (PowerVR2 compatible)
    RGB565 = 0,         // 16-bit RGB (5:6:5)
    RGBA4444,           // 16-bit RGBA (4:4:4:4)
    RGBA1555,           // 16-bit RGBA (1:5:5:5)
    RGB888,             // 24-bit RGB
    RGBA8888,           // 32-bit RGBA
    
    // Compressed formats
    DXT1,               // S3TC DXT1 compression
    DXT3,               // S3TC DXT3 compression
    DXT5,               // S3TC DXT5 compression
    PVRTC_2BPP,         // PowerVR texture compression 2bpp
    PVRTC_4BPP,         // PowerVR texture compression 4bpp
    
    // Floating-point formats
    R16F,               // 16-bit float red
    RG16F,              // 16-bit float RG
    RGBA16F,            // 16-bit float RGBA
    R32F,               // 32-bit float red
    RG32F,              // 32-bit float RG
    RGBA32F,            // 32-bit float RGBA
    
    // Depth/stencil formats
    Depth16,            // 16-bit depth
    Depth24,            // 24-bit depth
    Depth32F,           // 32-bit float depth
    Depth24Stencil8,    // 24-bit depth + 8-bit stencil
    Depth32FStencil8    // 32-bit float depth + 8-bit stencil
};
```

## Basic Usage

### Graphics Initialization

```cpp
#include <flight/hal/interfaces/graphics.hpp>
using namespace flight::hal;

void initialize_graphics_example(IGraphicsInterface* graphics) {
    // Initialize graphics device
    auto init_result = graphics->initialize();
    if (!init_result) {
        std::cerr << "Graphics initialization failed: " << init_result.error() << std::endl;
        return;
    }
    
    // Get device information
    const GraphicsDeviceInfo& device_info = graphics->get_device_info();
    
    std::cout << "Graphics Device Initialized:\n";
    std::cout << "  Device: " << device_info.device_name << "\n";
    std::cout << "  Vendor: " << device_info.vendor_name << "\n";
    std::cout << "  Driver: " << device_info.driver_version << "\n";
    std::cout << "  Max Texture Size: " << device_info.max_texture_size << "\n";
    std::cout << "  Max Render Targets: " << device_info.max_render_targets << "\n";
    std::cout << "  Graphics Memory: " << device_info.total_graphics_memory / (1024*1024) << " MB\n";
    std::cout << "  Unified Memory: " << (device_info.unified_memory ? "Yes" : "No") << "\n";
    
    // Check specific capabilities
    if (graphics->supports_graphics_capability(GraphicsCapability::ProgrammableShaders)) {
        std::cout << "  Supports programmable shaders\n";
    }
    if (graphics->supports_graphics_capability(GraphicsCapability::TileBasedRendering)) {
        std::cout << "  Uses tile-based rendering\n";
    }
    if (graphics->supports_graphics_capability(GraphicsCapability::StoreQueues)) {
        std::cout << "  Supports Dreamcast store queues\n";
    }
}
```

### Resource Creation

```cpp
void resource_creation_example(IGraphicsInterface* graphics) {
    // Create a 2D texture
    TextureDescriptor tex_desc = graphics::make_texture_descriptor(
        512, 512,                   // 512x512 resolution
        TextureFormat::RGBA8888     // 32-bit RGBA
    );
    tex_desc.mip_levels = 0;        // Generate all mip levels
    tex_desc.generate_mipmaps = true;
    tex_desc.debug_name = "PlayerTexture";
    
    auto texture_result = graphics->create_texture(tex_desc);
    if (texture_result) {
        GraphicsResourceHandle texture = texture_result.value();
        std::cout << "Created texture: " << texture.id << std::endl;
        
        // Update texture with pixel data
        std::vector<uint32_t> pixels(512 * 512, 0xFF0000FF); // Red pixels
        auto update_result = graphics->update_texture(
            texture,
            pixels.data(),
            512, 512    // Full texture update
        );
        
        if (update_result) {
            std::cout << "Texture updated successfully\n";
        }
    }
    
    // Create vertex buffer
    struct Vertex {
        float x, y, z;
        float u, v;
    };
    
    std::vector<Vertex> vertices = {
        {-1.0f, -1.0f, 0.0f, 0.0f, 1.0f},  // Bottom-left
        { 1.0f, -1.0f, 0.0f, 1.0f, 1.0f},  // Bottom-right
        { 0.0f,  1.0f, 0.0f, 0.5f, 0.0f}   // Top-center
    };
    
    BufferDescriptor vb_desc = graphics::make_buffer_descriptor(
        vertices.size() * sizeof(Vertex),
        GraphicsResourceType::VertexBuffer,
        BufferUsage::Static
    );
    vb_desc.stride = sizeof(Vertex);
    vb_desc.debug_name = "TriangleVertices";
    
    auto vertex_buffer_result = graphics->create_buffer(vb_desc, vertices.data());
    if (vertex_buffer_result) {
        GraphicsResourceHandle vertex_buffer = vertex_buffer_result.value();
        std::cout << "Created vertex buffer: " << vertex_buffer.id << std::endl;
    }
    
    // Create index buffer
    std::vector<uint16_t> indices = {0, 1, 2};
    
    BufferDescriptor ib_desc = graphics::make_buffer_descriptor(
        indices.size() * sizeof(uint16_t),
        GraphicsResourceType::IndexBuffer,
        BufferUsage::Static
    );
    ib_desc.debug_name = "TriangleIndices";
    
    auto index_buffer_result = graphics->create_buffer(ib_desc, indices.data());
    if (index_buffer_result) {
        GraphicsResourceHandle index_buffer = index_buffer_result.value();
        std::cout << "Created index buffer: " << index_buffer.id << std::endl;
    }
}
```

### Shader Creation

```cpp
void shader_creation_example(IGraphicsInterface* graphics) {
    // Check if programmable shaders are supported
    if (!graphics->supports_graphics_capability(GraphicsCapability::ProgrammableShaders)) {
        std::cout << "Using fixed-function pipeline\n";
        return;
    }
    
    // Vertex shader source (HLSL-style for cross-platform)
    const char* vertex_shader_source = R"(
        struct VSInput {
            float3 position : POSITION;
            float2 texcoord : TEXCOORD0;
        };
        
        struct VSOutput {
            float4 position : SV_POSITION;
            float2 texcoord : TEXCOORD0;
        };
        
        VSOutput main(VSInput input) {
            VSOutput output;
            output.position = float4(input.position, 1.0);
            output.texcoord = input.texcoord;
            return output;
        }
    )";
    
    // Fragment shader source
    const char* fragment_shader_source = R"(
        Texture2D diffuseTexture : register(t0);
        SamplerState diffuseSampler : register(s0);
        
        struct PSInput {
            float4 position : SV_POSITION;
            float2 texcoord : TEXCOORD0;
        };
        
        float4 main(PSInput input) : SV_TARGET {
            return diffuseTexture.Sample(diffuseSampler, input.texcoord);
        }
    )";
    
    // Create vertex shader
    ShaderDescriptor vs_desc{};
    vs_desc.type = GraphicsResourceType::VertexShader;
    vs_desc.source = vertex_shader_source;
    vs_desc.entry_point = "main";
    vs_desc.debug_name = "BasicVertexShader";
    
    auto vertex_shader_result = graphics->create_shader(vs_desc);
    if (vertex_shader_result) {
        std::cout << "Created vertex shader: " << vertex_shader_result.value().id << std::endl;
    }
    
    // Create fragment shader
    ShaderDescriptor fs_desc{};
    fs_desc.type = GraphicsResourceType::FragmentShader;
    fs_desc.source = fragment_shader_source;
    fs_desc.entry_point = "main";
    fs_desc.debug_name = "BasicFragmentShader";
    
    auto fragment_shader_result = graphics->create_shader(fs_desc);
    if (fragment_shader_result) {
        std::cout << "Created fragment shader: " << fragment_shader_result.value().id << std::endl;
    }
}
```

## Command Buffer Usage

### Basic Command Recording

```cpp
void basic_command_recording_example(IGraphicsInterface* graphics) {
    // Create command buffer
    auto cmd_buffer_result = graphics->create_command_buffer();
    if (!cmd_buffer_result) {
        std::cerr << "Failed to create command buffer\n";
        return;
    }
    
    auto cmd_buffer = std::move(cmd_buffer_result.value());
    
    // Begin recording
    HAL_TRY(cmd_buffer->begin());
    
    // Set viewport
    Viewport viewport = graphics::make_viewport(800.0f, 600.0f);
    HAL_TRY(cmd_buffer->set_viewport(viewport));
    
    // Set render state
    RenderState render_state = graphics::make_default_render_state();
    render_state.blend_enable = true;
    render_state.src_blend = BlendFactor::SrcAlpha;
    render_state.dst_blend = BlendFactor::InvSrcAlpha;
    render_state.depth_test_enable = true;
    render_state.depth_func = CompareFunc::Less;
    
    HAL_TRY(cmd_buffer->set_render_state(render_state));
    
    // Clear render target
    HAL_TRY(cmd_buffer->clear_render_target({0.2f, 0.3f, 0.5f, 1.0f})); // Sky blue
    HAL_TRY(cmd_buffer->clear_depth_stencil(1.0f, 0));
    
    // Bind resources (assuming they exist)
    GraphicsResourceHandle vertex_buffer{1, GraphicsResourceType::VertexBuffer, 1};
    GraphicsResourceHandle index_buffer{2, GraphicsResourceType::IndexBuffer, 1};
    GraphicsResourceHandle texture{3, GraphicsResourceType::Texture2D, 1};
    
    HAL_TRY(cmd_buffer->bind_vertex_buffer(0, vertex_buffer, sizeof(Vertex)));
    HAL_TRY(cmd_buffer->bind_index_buffer(index_buffer));
    HAL_TRY(cmd_buffer->bind_texture(0, texture));
    
    // Draw indexed primitives
    DrawIndexedCommand draw_cmd{};
    draw_cmd.topology = PrimitiveTopology::TriangleList;
    draw_cmd.index_count = 3;
    draw_cmd.index_offset = 0;
    draw_cmd.vertex_offset = 0;
    draw_cmd.instance_count = 1;
    
    HAL_TRY(cmd_buffer->draw_indexed(draw_cmd));
    
    // End recording
    HAL_TRY(cmd_buffer->end());
    
    // Submit for execution
    HAL_TRY(graphics->submit_command_buffer(cmd_buffer.get()));
    
    // Get command buffer statistics
    CommandBufferStats stats = cmd_buffer->get_stats();
    std::cout << "Command Buffer Stats:\n";
    std::cout << "  Commands: " << stats.command_count << "\n";
    std::cout << "  Draw calls: " << stats.draw_call_count << "\n";
    std::cout << "  State changes: " << stats.state_change_count << "\n";
    std::cout << "  Memory used: " << stats.memory_used << " bytes\n";
}
```

### Advanced Command Recording with Modern APIs

```cpp
void modern_command_recording_example(IGraphicsInterface* graphics) {
    auto cmd_buffer = graphics->create_command_buffer().value();
    
    HAL_TRY(cmd_buffer->begin());
    
    // Begin debug group for profiling
    HAL_TRY(cmd_buffer->begin_debug_group("SceneRendering"));
    
    // Begin render pass (modern APIs)
    std::vector<GraphicsResourceHandle> render_targets = {
        graphics->get_backbuffer().value()
    };
    GraphicsResourceHandle depth_buffer{100, GraphicsResourceType::RenderTarget, 1};
    
    HAL_TRY(cmd_buffer->begin_render_pass(render_targets, depth_buffer));
    
    // Set debug marker
    HAL_TRY(cmd_buffer->set_debug_marker("GeometryPass"));
    
    // Render geometry
    // ... binding and drawing ...
    
    // End render pass
    HAL_TRY(cmd_buffer->end_render_pass());
    
    // Compute pass (if supported)
    if (graphics->supports_graphics_capability(GraphicsCapability::ComputeShaders)) {
        HAL_TRY(cmd_buffer->set_debug_marker("PostProcessCompute"));
        
        // Bind compute shader and dispatch
        HAL_TRY(cmd_buffer->dispatch_compute(
            (800 + 15) / 16,  // Workgroups in X (16x16 threads per group)
            (600 + 15) / 16,  // Workgroups in Y
            1                 // Workgroups in Z
        ));
    }
    
    // End debug group
    HAL_TRY(cmd_buffer->end_debug_group());
    
    HAL_TRY(cmd_buffer->end());
    HAL_TRY(graphics->submit_command_buffer(cmd_buffer.get()));
}
```

## Platform-Specific Optimizations

### Dreamcast PowerVR2 Optimization

```cpp
void dreamcast_optimization_example(IGraphicsInterface* graphics) {
    // Check for Dreamcast-specific capabilities
    if (!graphics->supports_graphics_capability(GraphicsCapability::TileBasedRendering) ||
        !graphics->supports_graphics_capability(GraphicsCapability::StoreQueues)) {
        std::cout << "Not running on Dreamcast hardware\n";
        return;
    }
    
    std::cout << "Applying Dreamcast PowerVR2 optimizations\n";
    
    // Get store queue interface for optimal data transfer
    void* store_queue_interface = graphics->get_store_queue_interface();
    if (store_queue_interface) {
        std::cout << "Store queue interface available for fast data transfer\n";
    }
    
    // Get display list interface for efficient command submission
    void* display_list_interface = graphics->get_display_list_interface();
    if (display_list_interface) {
        std::cout << "Display list interface available for optimized rendering\n";
    }
    
    // Use triangle strips and fans (PowerVR2 optimized)
    auto cmd_buffer = graphics->create_command_buffer().value();
    HAL_TRY(cmd_buffer->begin());
    
    // PowerVR2 prefers triangle strips for better tile utilization
    DrawCommand strip_draw{};
    strip_draw.topology = PrimitiveTopology::TriangleStrip;
    strip_draw.vertex_count = 6;  // 4 triangles in strip
    strip_draw.vertex_offset = 0;
    strip_draw.instance_count = 1;
    
    HAL_TRY(cmd_buffer->draw(strip_draw));
    
    // Triangle fans are also efficient on PowerVR2
    DrawCommand fan_draw{};
    fan_draw.topology = PrimitiveTopology::TriangleFan;
    fan_draw.vertex_count = 8;   // 6 triangles in fan
    fan_draw.vertex_offset = 6;
    fan_draw.instance_count = 1;
    
    HAL_TRY(cmd_buffer->draw(fan_draw));
    
    HAL_TRY(cmd_buffer->end());
    HAL_TRY(graphics->submit_command_buffer(cmd_buffer.get()));
    
    std::cout << "Dreamcast-optimized rendering submitted\n";
}
```

### Modern GPU with Vulkan/Metal Features

```cpp
void modern_gpu_optimization_example(IGraphicsInterface* graphics) {
    // Check for modern GPU capabilities
    if (!graphics->supports_graphics_capability(GraphicsCapability::ExplicitSynchronization) ||
        !graphics->supports_graphics_capability(GraphicsCapability::MultiThreadedCommands)) {
        std::cout << "Modern GPU features not available\n";
        return;
    }
    
    std::cout << "Applying modern GPU optimizations\n";
    
    // Multi-threaded command buffer recording
    std::vector<std::unique_ptr<ICommandBuffer>> cmd_buffers;
    const int num_threads = 4;
    
    for (int i = 0; i < num_threads; ++i) {
        auto cmd_buffer = graphics->create_command_buffer().value();
        HAL_TRY(cmd_buffer->begin());
        
        // Record thread-specific commands
        HAL_TRY(cmd_buffer->set_debug_marker(("Thread" + std::to_string(i)).c_str()));
        
        // Each thread renders different objects
        // ... thread-specific rendering ...
        
        HAL_TRY(cmd_buffer->end());
        cmd_buffers.push_back(std::move(cmd_buffer));
    }
    
    // Submit all command buffers in batch
    std::vector<ICommandBuffer*> cmd_buffer_ptrs;
    for (auto& cmd_buffer : cmd_buffers) {
        cmd_buffer_ptrs.push_back(cmd_buffer.get());
    }
    
    HAL_TRY(graphics->submit_command_buffers(cmd_buffer_ptrs, false)); // Don't wait
    
    std::cout << "Multi-threaded command buffers submitted\n";
}
```

### Web Platform Constraints

```cpp
void web_optimization_example(IGraphicsInterface* graphics) {
    const GraphicsDeviceInfo& device_info = graphics->get_device_info();
    
    // Check for web platform constraints
    if (device_info.total_graphics_memory > 0 && 
        device_info.total_graphics_memory < 256 * 1024 * 1024) {  // < 256MB
        
        std::cout << "Web platform detected - applying constraints\n";
        
        // Use smaller textures
        TextureDescriptor web_texture = graphics::make_texture_descriptor(
            256, 256,                 // Smaller resolution
            TextureFormat::RGB565     // Lower bit depth
        );
        web_texture.mip_levels = 1;   // No mipmaps to save memory
        
        auto texture_result = graphics->create_texture(web_texture);
        if (texture_result) {
            std::cout << "Created web-optimized texture\n";
        }
        
        // Batch draw calls to reduce WebGL overhead
        auto cmd_buffer = graphics->create_command_buffer().value();
        HAL_TRY(cmd_buffer->begin());
        
        // Minimize state changes
        RenderState web_state = graphics::make_default_render_state();
        HAL_TRY(cmd_buffer->set_render_state(web_state));
        
        // Draw multiple objects with same state
        for (int i = 0; i < 10; ++i) {
            DrawCommand draw{};
            draw.topology = PrimitiveTopology::TriangleList;
            draw.vertex_count = 3;
            draw.vertex_offset = i * 3;
            draw.instance_count = 1;
            
            HAL_TRY(cmd_buffer->draw(draw));
        }
        
        HAL_TRY(cmd_buffer->end());
        HAL_TRY(graphics->submit_command_buffer(cmd_buffer.get()));
    }
}
```

## Performance Monitoring

### Graphics Statistics

```cpp
void graphics_statistics_example(IGraphicsInterface* graphics) {
    // Get current graphics statistics
    GraphicsStats stats = graphics->get_stats();
    
    std::cout << "Graphics Performance Statistics:\n";
    std::cout << "  Frames rendered: " << stats.frame_count << "\n";
    std::cout << "  Draw calls: " << stats.draw_call_count << "\n";
    std::cout << "  Triangles: " << stats.triangle_count << "\n";
    std::cout << "  Texture switches: " << stats.texture_switches << "\n";
    std::cout << "  Shader switches: " << stats.shader_switches << "\n";
    std::cout << "  State changes: " << stats.render_state_changes << "\n";
    
    std::cout << "\nMemory Usage:\n";
    std::cout << "  Texture memory: " << stats.texture_memory_used / (1024*1024) << " MB\n";
    std::cout << "  Buffer memory: " << stats.buffer_memory_used / (1024*1024) << " MB\n";
    std::cout << "  Total graphics memory: " << stats.total_graphics_memory_used / (1024*1024) << " MB\n";
    
    std::cout << "\nPerformance Metrics:\n";
    std::cout << "  Average frame time: " << stats.average_frame_time_ms << " ms\n";
    std::cout << "  GPU utilization: " << stats.gpu_utilization << "%\n";
    
    // Get memory usage
    auto memory_usage = graphics->get_gpu_memory_usage();
    auto available_memory = graphics->get_available_gpu_memory();
    
    if (memory_usage && available_memory) {
        std::cout << "\nCurrent Memory Status:\n";
        std::cout << "  Used: " << memory_usage.value() / (1024*1024) << " MB\n";
        std::cout << "  Available: " << available_memory.value() / (1024*1024) << " MB\n";
    }
}
```

### GPU Timing

```cpp
void gpu_timing_example(IGraphicsInterface* graphics) {
    // Begin GPU timing for geometry pass
    auto geometry_query = graphics->begin_gpu_timing("GeometryPass");
    if (!geometry_query) {
        std::cout << "GPU timing not supported\n";
        return;
    }
    
    // Render geometry
    auto cmd_buffer = graphics->create_command_buffer().value();
    HAL_TRY(cmd_buffer->begin());
    
    // ... geometry rendering commands ...
    
    HAL_TRY(cmd_buffer->end());
    HAL_TRY(graphics->submit_command_buffer(cmd_buffer.get()));
    
    // End timing
    HAL_TRY(graphics->end_gpu_timing(geometry_query.value()));
    
    // Begin timing for post-processing
    auto postprocess_query = graphics->begin_gpu_timing("PostProcess");
    if (postprocess_query) {
        // ... post-processing commands ...
        HAL_TRY(graphics->end_gpu_timing(postprocess_query.value()));
    }
    
    // Wait a frame then get results
    HAL_TRY(graphics->present());
    
    // Get timing results (may need to wait several frames)
    auto geometry_time = graphics->get_gpu_timing_result(geometry_query.value());
    auto postprocess_time = graphics->get_gpu_timing_result(postprocess_query.value());
    
    if (geometry_time) {
        std::cout << "Geometry pass: " << geometry_time.value() << " μs\n";
    }
    if (postprocess_time) {
        std::cout << "Post-process: " << postprocess_time.value() << " μs\n";
    }
}
```

## Error Handling

### Comprehensive Graphics Error Handling

```cpp
void robust_graphics_usage(IGraphicsInterface* graphics) {
    // Initialize with error handling
    auto init_result = graphics->initialize();
    if (!init_result) {
        HALError error = init_result.error();
        
        switch (error.category) {
            case HALErrorCategory::DeviceError:
                std::cerr << "Graphics device error: " << error.message << std::endl;
                // Try fallback graphics settings
                break;
                
            case HALErrorCategory::OutOfMemory:
                std::cerr << "Insufficient GPU memory\n";
                // Reduce texture quality, disable features
                break;
                
            case HALErrorCategory::PlatformError:
                std::cerr << "Platform graphics error: " << error.message << std::endl;
                break;
                
            default:
                std::cerr << "Unknown graphics error: " << error.message << std::endl;
                break;
        }
        return;
    }
    
    // Resource creation with fallback
    TextureDescriptor high_quality_desc = graphics::make_texture_descriptor(
        2048, 2048, TextureFormat::RGBA8888
    );
    
    auto texture_result = graphics->create_texture(high_quality_desc);
    if (!texture_result) {
        std::cout << "High quality texture failed, trying lower quality\n";
        
        // Fallback to lower quality
        TextureDescriptor low_quality_desc = graphics::make_texture_descriptor(
            1024, 1024, TextureFormat::RGB565
        );
        
        texture_result = graphics->create_texture(low_quality_desc);
        if (!texture_result) {
            std::cerr << "Even low quality texture failed!\n";
            return;
        }
    }
    
    // Command buffer with error recovery
    auto cmd_buffer_result = graphics->create_command_buffer();
    if (!cmd_buffer_result) {
        std::cerr << "Failed to create command buffer\n";
        return;
    }
    
    auto cmd_buffer = std::move(cmd_buffer_result.value());
    
    // Safe command recording with error checks
    if (!cmd_buffer->begin()) {
        std::cerr << "Failed to begin command recording\n";
        return;
    }
    
    // Each command should be checked
    auto viewport_result = cmd_buffer->set_viewport(graphics::make_viewport(800, 600));
    if (!viewport_result) {
        std::cerr << "Failed to set viewport\n";
        // Continue with default viewport or abort
    }
    
    // Always end command buffer even on errors
    cmd_buffer->end();
}
```

## Performance Characteristics

### Graphics Interface Performance

| Operation | Typical Latency | Memory Overhead | Notes |
|-----------|----------------|-----------------|-------|
| **initialize()** | 10-100ms | Platform dependent | One-time setup cost |
| **create_command_buffer()** | < 100μs | 1-8KB per buffer | Pre-allocated pools |
| **create_texture()** | 1-10ms | Texture size + metadata | GPU memory allocation |
| **create_buffer()** | < 1ms | Buffer size + metadata | Fast for small buffers |
| **submit_command_buffer()** | < 100μs | Minimal | Asynchronous submission |
| **present()** | 0-16ms | 0 bytes | Depends on VSync |

### Platform Performance Characteristics

#### Dreamcast PowerVR2
- **Initialization**: < 200ms (PowerVR2 setup)
- **Texture creation**: < 5ms (PVR compression)
- **Command submission**: < 50μs (display lists)
- **Draw call overhead**: < 10μs per call
- **Memory bandwidth**: 800MB/s peak

#### PSP (Graphics Synthesizer)
- **Initialization**: < 100ms
- **Texture creation**: < 2ms (small textures)
- **Command submission**: < 30μs
- **Draw call overhead**: < 5μs per call
- **Memory bandwidth**: 2.6GB/s

#### Web (WebGL/WebGPU)
- **Initialization**: 50-500ms (context creation)
- **Texture creation**: 1-20ms (varies by browser)
- **Command submission**: 100-1000μs (JavaScript overhead)
- **Draw call overhead**: 10-100μs per call
- **Memory constraints**: Browser dependent

#### Modern Desktop (Vulkan/D3D12/Metal)
- **Initialization**: 10-50ms
- **Texture creation**: < 1ms (fast allocation)
- **Command submission**: < 10μs (direct GPU queues)
- **Draw call overhead**: < 1μs per call
- **Memory bandwidth**: 100-1000GB/s

## Best Practices

### 1. Choose Appropriate Primitives

```cpp
// PowerVR2 optimization - use strips and fans
if (graphics->supports_graphics_capability(GraphicsCapability::TileBasedRendering)) {
    // Triangle strips are optimal for tile-based renderers
    draw_cmd.topology = PrimitiveTopology::TriangleStrip;
} else {
    // Triangle lists work well on immediate mode renderers
    draw_cmd.topology = PrimitiveTopology::TriangleList;
}
```

### 2. Minimize State Changes

```cpp
// Batch objects by material/shader to reduce state changes
void render_batch(const std::vector<RenderObject>& objects) {
    auto cmd_buffer = graphics->create_command_buffer().value();
    HAL_TRY(cmd_buffer->begin());
    
    // Sort objects by material to minimize state changes
    // Set render state once per material batch
    RenderState current_state;
    GraphicsResourceHandle current_shader;
    
    for (const auto& obj : objects) {
        if (obj.shader != current_shader) {
            cmd_buffer->bind_shaders(obj.vertex_shader, obj.fragment_shader);
            current_shader = obj.shader;
        }
        
        if (obj.render_state != current_state) {
            cmd_buffer->set_render_state(obj.render_state);
            current_state = obj.render_state;
        }
        
        // Draw object
        cmd_buffer->draw_indexed(obj.draw_command);
    }
    
    HAL_TRY(cmd_buffer->end());
    HAL_TRY(graphics->submit_command_buffer(cmd_buffer.get()));
}
```

### 3. Use Appropriate Texture Formats

```cpp
// Choose format based on content and platform
TextureFormat choose_optimal_format(const ImageData& image, IGraphicsInterface* graphics) {
    // For platforms with limited memory (Dreamcast, PSP)
    if (graphics->get_device_info().total_graphics_memory < 32 * 1024 * 1024) {
        if (image.has_alpha) {
            return TextureFormat::RGBA4444;  // 16-bit with alpha
        } else {
            return TextureFormat::RGB565;    // 16-bit without alpha
        }
    }
    
    // For modern platforms
    if (image.is_hdr) {
        return TextureFormat::RGBA16F;       // HDR content
    } else if (image.needs_compression) {
        return TextureFormat::DXT5;          // Compressed
    } else {
        return TextureFormat::RGBA8888;      // Full quality
    }
}
```

### 4. Handle GPU Memory Wisely

```cpp
// Monitor GPU memory usage
void manage_gpu_memory(IGraphicsInterface* graphics) {
    auto used = graphics->get_gpu_memory_usage();
    auto available = graphics->get_available_gpu_memory();
    
    if (used && available) {
        float usage_ratio = static_cast<float>(used.value()) / 
                           (used.value() + available.value());
        
        if (usage_ratio > 0.8f) {  // 80% memory usage
            // Free unused textures
            free_unused_textures();
            
            // Reduce texture quality
            reduce_texture_lod();
            
            // Use compressed formats
            convert_to_compressed_formats();
        }
    }
}
```

### 5. Platform-Specific Optimizations

```cpp
void apply_platform_optimizations(IGraphicsInterface* graphics) {
    const auto& device_info = graphics->get_device_info();
    
    // Dreamcast optimizations
    if (graphics->supports_graphics_capability(GraphicsCapability::StoreQueues)) {
        // Use store queues for fast vertex data transfer
        setup_store_queue_rendering();
    }
    
    // Modern GPU optimizations
    if (graphics->supports_graphics_capability(GraphicsCapability::MultiThreadedCommands)) {
        // Enable multi-threaded command recording
        setup_multithreaded_rendering();
    }
    
    // Web platform optimizations
    if (device_info.total_graphics_memory < 256 * 1024 * 1024) {
        // Reduce quality for memory-constrained web platforms
        setup_web_optimizations();
    }
    
    // Unified memory optimizations (Metal, some Vulkan drivers)
    if (device_info.unified_memory) {
        // Optimize for unified memory architecture
        setup_unified_memory_rendering();
    }
}
```

## Graphics Pipeline Patterns

### Fixed-Function Pipeline (PowerVR2, Legacy)

```cpp
void render_with_fixed_function(IGraphicsInterface* graphics) {
    if (!graphics->supports_graphics_capability(GraphicsCapability::FixedFunctionPipeline)) {
        std::cout << "Fixed-function pipeline not supported\n";
        return;
    }
    
    auto cmd_buffer = graphics->create_command_buffer().value();
    HAL_TRY(cmd_buffer->begin());
    
    // Set transformation matrices (typically through uniform buffers or constants)
    // Set material properties
    // Enable/disable fixed-function features
    
    RenderState ff_state = graphics::make_default_render_state();
    ff_state.fog_enable = true;  // PowerVR2 supports hardware fog
    ff_state.fog_color = 0x808080FF;  // Gray fog
    ff_state.fog_density = 0.1f;
    
    HAL_TRY(cmd_buffer->set_render_state(ff_state));
    
    // Bind vertex data (position, normal, texcoords)
    // Fixed-function will handle T&L automatically
    
    HAL_TRY(cmd_buffer->end());
    HAL_TRY(graphics->submit_command_buffer(cmd_buffer.get()));
}
```

### Modern Programmable Pipeline

```cpp
void render_with_programmable_pipeline(IGraphicsInterface* graphics) {
    if (!graphics->supports_graphics_capability(GraphicsCapability::ProgrammableShaders)) {
        std::cout << "Programmable shaders not supported\n";
        return;
    }
    
    auto cmd_buffer = graphics->create_command_buffer().value();
    HAL_TRY(cmd_buffer->begin());
    
    // Bind shader programs
    HAL_TRY(cmd_buffer->bind_shaders(vertex_shader, fragment_shader));
    
    // Bind uniform buffers
    HAL_TRY(cmd_buffer->bind_uniform_buffer(0, per_frame_uniforms));
    HAL_TRY(cmd_buffer->bind_uniform_buffer(1, per_object_uniforms));
    
    // Bind textures
    HAL_TRY(cmd_buffer->bind_texture(0, diffuse_texture));
    HAL_TRY(cmd_buffer->bind_texture(1, normal_texture));
    
    // Set render state
    RenderState modern_state = graphics::make_default_render_state();
    modern_state.depth_test_enable = true;
    modern_state.blend_enable = false;
    
    HAL_TRY(cmd_buffer->set_render_state(modern_state));
    
    // Draw
    HAL_TRY(cmd_buffer->draw_indexed(draw_command));
    
    HAL_TRY(cmd_buffer->end());
    HAL_TRY(graphics->submit_command_buffer(cmd_buffer.get()));
}
```

---

## API Reference

For complete method signatures and detailed parameter information, see [Graphics API Reference](api-reference.md).

## Related Documentation

- [Graphics Interface Design](../../graphics_interface_design.md) - Design rationale and architecture
- [Command Buffer Patterns](command-buffers.md) - Command recording best practices
- [Resource Management](resource-management.md) - Texture and buffer management
- [Platform Graphics Guides](../../platform-guides/) - Platform-specific graphics optimization
- [Performance Benchmarking](../../performance_benchmarking_system.md) - Graphics performance testing

---

*The Graphics interface enables unified rendering from Dreamcast's PowerVR2 to cutting-edge GPUs with Vulkan, Metal, and D3D12.*
