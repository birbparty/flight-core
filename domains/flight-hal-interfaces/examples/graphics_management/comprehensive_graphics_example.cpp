/**
 * @file comprehensive_graphics_example.cpp
 * @brief Comprehensive Graphics Interface Usage Example
 * 
 * Demonstrates the complete Flight HAL graphics interface capabilities,
 * including resource management, command buffer usage, and cross-platform
 * adaptation between PowerVR2-style and modern graphics APIs.
 */

#include "../../include/flight/hal/interfaces/graphics.hpp"
#include "../../include/flight/hal/core/driver_registry.hpp"
#include "../../include/flight/hal/core/platform_detection.hpp"
#include "../drivers/mock_graphics_driver.hpp"
#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include <chrono>

using namespace flight::hal;
using namespace flight::hal::graphics;

/**
 * @brief Simple vertex structure for demonstration
 */
struct Vertex {
    float position[3];
    float color[4];
    float uv[2];
};

/**
 * @brief Graphics example demonstrating various interface features
 */
class GraphicsExample {
private:
    std::unique_ptr<IGraphicsInterface> graphics_;
    GraphicsResourceHandle vertex_buffer_;
    GraphicsResourceHandle index_buffer_;
    GraphicsResourceHandle texture_;
    GraphicsResourceHandle vertex_shader_;
    GraphicsResourceHandle fragment_shader_;
    GraphicsResourceHandle render_target_;
    std::unique_ptr<ICommandBuffer> command_buffer_;

public:
    bool initialize() {
        std::cout << "\n=== Flight HAL Graphics Interface Example ===\n";
        
        // Get graphics driver from registry
        auto& registry = DriverRegistry::instance();
        auto graphics_shared = registry.get_interface<IGraphicsInterface>();
        if (!graphics_shared) {
            std::cerr << "Failed to get graphics driver from registry" << std::endl;
            return false;
        }
        
        graphics_ = std::unique_ptr<IGraphicsInterface>(graphics_shared.get());
        
        // Initialize graphics system
        std::cout << "\n--- Initializing Graphics System ---\n";
        auto init_result = graphics_->initialize();
        if (!init_result) {
            std::cerr << "Failed to initialize graphics: " << init_result.error().message() << std::endl;
            return false;
        }
        
        // Display device information
        display_device_info();
        
        // Create resources
        if (!create_resources()) {
            return false;
        }
        
        // Create command buffer
        auto cmd_result = graphics_->create_command_buffer();
        if (!cmd_result) {
            std::cerr << "Failed to create command buffer: " << cmd_result.error().message() << std::endl;
            return false;
        }
        command_buffer_ = std::move(cmd_result.value());
        
        std::cout << "Graphics system initialized successfully!\n";
        return true;
    }
    
    void display_device_info() {
        const auto& device_info = graphics_->get_device_info();
        // Use mock driver's platform info directly
        auto* mock_driver = dynamic_cast<examples::MockGraphicsDriver*>(graphics_.get());
        const PlatformInfo& platform_info = mock_driver ? mock_driver->get_platform_info() : PlatformInfo{};
        
        std::cout << "\n--- Device Information ---\n";
        std::cout << "Device: " << device_info.device_name << std::endl;
        std::cout << "Vendor: " << device_info.vendor_name << std::endl;
        std::cout << "Driver: " << device_info.driver_version << std::endl;
        std::cout << "Platform: " << platform_info.platform_name << " (" << platform_info.architecture << ")" << std::endl;
        std::cout << "Performance Tier: " << static_cast<int>(device_info.performance_tier) << std::endl;
        
        std::cout << "\n--- Graphics Capabilities ---\n";
        std::cout << "Max Texture Size: " << device_info.max_texture_size << "x" << device_info.max_texture_size << std::endl;
        std::cout << "Max Render Targets: " << device_info.max_render_targets << std::endl;
        std::cout << "Max Vertex Attributes: " << device_info.max_vertex_attributes << std::endl;
        std::cout << "Graphics Memory: " << (device_info.total_graphics_memory / (1024*1024)) << " MB" << std::endl;
        std::cout << "Unified Memory: " << (device_info.unified_memory ? "Yes" : "No") << std::endl;
        std::cout << "Coherent Memory: " << (device_info.coherent_memory ? "Yes" : "No") << std::endl;
        
        // Display supported capabilities
        std::cout << "\n--- Supported Graphics Features ---\n";
        const std::vector<std::pair<GraphicsCapability, const char*>> capabilities = {
            {GraphicsCapability::FixedFunctionPipeline, "Fixed Function Pipeline"},
            {GraphicsCapability::ProgrammableShaders, "Programmable Shaders"},
            {GraphicsCapability::ComputeShaders, "Compute Shaders"},
            {GraphicsCapability::TileBasedRendering, "Tile-Based Rendering"},
            {GraphicsCapability::ImmediateModeRendering, "Immediate Mode Rendering"},
            {GraphicsCapability::MultipleRenderTargets, "Multiple Render Targets"},
            {GraphicsCapability::DepthBuffer, "Depth Buffer"},
            {GraphicsCapability::StencilBuffer, "Stencil Buffer"},
            {GraphicsCapability::TextureCompression, "Texture Compression"},
            {GraphicsCapability::MipmapGeneration, "Mipmap Generation"},
            {GraphicsCapability::CubeMapping, "Cube Mapping"},
            {GraphicsCapability::VertexBufferObjects, "Vertex Buffer Objects"},
            {GraphicsCapability::IndexBufferObjects, "Index Buffer Objects"},
            {GraphicsCapability::UniformBufferObjects, "Uniform Buffer Objects"},
            {GraphicsCapability::StoreQueues, "Store Queues (PowerVR2)"},
            {GraphicsCapability::DisplayLists, "Display Lists (PowerVR2)"}
        };
        
        for (const auto& [cap, name] : capabilities) {
            if (graphics_->supports_graphics_capability(cap)) {
                std::cout << "  ✓ " << name << std::endl;
            }
        }
        
        // Display supported texture formats
        std::cout << "\n--- Supported Texture Formats ---\n";
        const std::vector<std::pair<TextureFormat, const char*>> formats = {
            {TextureFormat::RGB565, "RGB565"},
            {TextureFormat::RGBA4444, "RGBA4444"},
            {TextureFormat::RGBA8888, "RGBA8888"},
            {TextureFormat::DXT1, "DXT1"},
            {TextureFormat::DXT5, "DXT5"},
            {TextureFormat::PVRTC_4BPP, "PVRTC 4BPP"},
            {TextureFormat::Depth24Stencil8, "Depth24Stencil8"}
        };
        
        for (const auto& [format, name] : formats) {
            if (graphics_->supports_texture_format(format)) {
                std::cout << "  ✓ " << name << std::endl;
            }
        }
    }
    
    bool create_resources() {
        std::cout << "\n--- Creating Graphics Resources ---\n";
        
        // Create vertex buffer
        std::vector<Vertex> vertices = {
            {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
            {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
            {{ 0.0f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {0.5f, 0.0f}}
        };
        
        BufferDescriptor vertex_desc = make_buffer_descriptor(
            vertices.size() * sizeof(Vertex),
            GraphicsResourceType::VertexBuffer,
            BufferUsage::Static
        );
        vertex_desc.stride = sizeof(Vertex);
        vertex_desc.debug_name = "Triangle Vertices";
        
        auto vertex_result = graphics_->create_buffer(vertex_desc, vertices.data());
        if (!vertex_result) {
            std::cerr << "Failed to create vertex buffer: " << vertex_result.error().message() << std::endl;
            return false;
        }
        vertex_buffer_ = vertex_result.value();
        std::cout << "Created vertex buffer (" << vertices.size() << " vertices)" << std::endl;
        
        // Create index buffer
        std::vector<uint16_t> indices = {0, 1, 2};
        
        BufferDescriptor index_desc = make_buffer_descriptor(
            indices.size() * sizeof(uint16_t),
            GraphicsResourceType::IndexBuffer,
            BufferUsage::Static
        );
        index_desc.debug_name = "Triangle Indices";
        
        auto index_result = graphics_->create_buffer(index_desc, indices.data());
        if (!index_result) {
            std::cerr << "Failed to create index buffer: " << index_result.error().message() << std::endl;
            return false;
        }
        index_buffer_ = index_result.value();
        std::cout << "Created index buffer (" << indices.size() << " indices)" << std::endl;
        
        // Create texture
        const uint32_t texture_width = 64;
        const uint32_t texture_height = 64;
        std::vector<uint32_t> texture_data(texture_width * texture_height);
        
        // Generate checkerboard pattern
        for (uint32_t y = 0; y < texture_height; ++y) {
            for (uint32_t x = 0; x < texture_width; ++x) {
                bool checker = ((x / 8) + (y / 8)) % 2;
                texture_data[y * texture_width + x] = checker ? 0xFFFFFFFF : 0xFF000000;
            }
        }
        
        TextureDescriptor texture_desc = make_texture_descriptor(texture_width, texture_height);
        texture_desc.format = TextureFormat::RGBA8888;
        texture_desc.generate_mipmaps = graphics_->supports_graphics_capability(GraphicsCapability::MipmapGeneration);
        texture_desc.debug_name = "Checkerboard Texture";
        
        auto texture_result = graphics_->create_texture(texture_desc, texture_data.data());
        if (!texture_result) {
            std::cerr << "Failed to create texture: " << texture_result.error().message() << std::endl;
            return false;
        }
        texture_ = texture_result.value();
        std::cout << "Created texture (" << texture_width << "x" << texture_height << ")" << std::endl;
        
        // Generate mipmaps if supported
        if (texture_desc.generate_mipmaps) {
            auto mipmap_result = graphics_->generate_mipmaps(texture_);
            if (mipmap_result) {
                std::cout << "Generated mipmaps for texture" << std::endl;
            }
        }
        
        // Create shaders (mock bytecode)
        if (graphics_->supports_graphics_capability(GraphicsCapability::ProgrammableShaders)) {
            std::cout << "Creating programmable shaders..." << std::endl;
            
            // Vertex shader
            const char* vertex_shader_code = "vertex_shader_mock_bytecode";
            ShaderDescriptor vs_desc = {};
            vs_desc.type = GraphicsResourceType::VertexShader;
            vs_desc.bytecode = vertex_shader_code;
            vs_desc.bytecode_size = strlen(vertex_shader_code);
            vs_desc.debug_name = "Triangle Vertex Shader";
            
            auto vs_result = graphics_->create_shader(vs_desc);
            if (!vs_result) {
                std::cerr << "Failed to create vertex shader: " << vs_result.error().message() << std::endl;
                return false;
            }
            vertex_shader_ = vs_result.value();
            
            // Fragment shader
            const char* fragment_shader_code = "fragment_shader_mock_bytecode";
            ShaderDescriptor fs_desc = {};
            fs_desc.type = GraphicsResourceType::FragmentShader;
            fs_desc.bytecode = fragment_shader_code;
            fs_desc.bytecode_size = strlen(fragment_shader_code);
            fs_desc.debug_name = "Triangle Fragment Shader";
            
            auto fs_result = graphics_->create_shader(fs_desc);
            if (!fs_result) {
                std::cerr << "Failed to create fragment shader: " << fs_result.error().message() << std::endl;
                return false;
            }
            fragment_shader_ = fs_result.value();
            
            std::cout << "Created vertex and fragment shaders" << std::endl;
        }
        
        // Create render target
        auto rt_result = graphics_->create_render_target(512, 512, TextureFormat::RGBA8888, "Off-screen Target");
        if (!rt_result) {
            std::cerr << "Failed to create render target: " << rt_result.error().message() << std::endl;
            return false;
        }
        render_target_ = rt_result.value();
        std::cout << "Created render target (512x512)" << std::endl;
        
        return true;
    }
    
    void demonstrate_rendering() {
        std::cout << "\n--- Demonstrating Rendering Operations ---\n";
        
        // Begin command buffer recording
        auto begin_result = command_buffer_->begin();
        if (!begin_result) {
            std::cerr << "Failed to begin command buffer: " << begin_result.error().message() << std::endl;
            return;
        }
        
        // Set debug marker
        command_buffer_->set_debug_marker("Triangle Rendering");
        command_buffer_->begin_debug_group("Main Rendering Pass");
        
        // Set render state
        RenderState render_state = make_default_render_state();
        render_state.depth_test_enable = true;
        render_state.blend_enable = false;
        command_buffer_->set_render_state(render_state);
        
        // Set viewport
        Viewport viewport = make_viewport(512.0f, 512.0f);
        command_buffer_->set_viewport(viewport);
        
        // Begin render pass
        std::vector<GraphicsResourceHandle> render_targets = {render_target_};
        command_buffer_->begin_render_pass(render_targets, {});
        
        // Clear render target
        std::array<float, 4> clear_color = {0.2f, 0.3f, 0.4f, 1.0f};
        command_buffer_->clear_render_target(clear_color);
        
        // Bind resources
        command_buffer_->bind_vertex_buffer(0, vertex_buffer_, sizeof(Vertex), 0);
        command_buffer_->bind_index_buffer(index_buffer_, 0);
        command_buffer_->bind_texture(0, texture_);
        
        // Bind shaders if available
        if (graphics_->supports_graphics_capability(GraphicsCapability::ProgrammableShaders)) {
            command_buffer_->bind_shaders(vertex_shader_, fragment_shader_);
        }
        
        // Draw triangle
        DrawIndexedCommand draw_cmd = {};
        draw_cmd.index_count = 3;
        draw_cmd.instance_count = 1;
        draw_cmd.vertex_offset = 0;
        draw_cmd.topology = PrimitiveTopology::TriangleList;
        
        command_buffer_->draw_indexed(draw_cmd);
        
        // End render pass
        command_buffer_->end_render_pass();
        command_buffer_->end_debug_group();
        
        // End command buffer recording
        auto end_result = command_buffer_->end();
        if (!end_result) {
            std::cerr << "Failed to end command buffer: " << end_result.error().message() << std::endl;
            return;
        }
        
        // Submit command buffer
        auto submit_result = graphics_->submit_command_buffer(command_buffer_.get(), true);
        if (!submit_result) {
            std::cerr << "Failed to submit command buffer: " << submit_result.error().message() << std::endl;
            return;
        }
        
        // Get command buffer stats
        auto stats = command_buffer_->get_stats();
        std::cout << "Command buffer executed successfully!" << std::endl;
        std::cout << "  Commands: " << stats.command_count << std::endl;
        std::cout << "  Draw calls: " << stats.draw_call_count << std::endl;
        std::cout << "  State changes: " << stats.state_change_count << std::endl;
        std::cout << "  Resource binds: " << stats.resource_bind_count << std::endl;
        std::cout << "  Memory used: " << stats.memory_used << " bytes" << std::endl;
    }
    
    void demonstrate_performance_monitoring() {
        std::cout << "\n--- Performance Monitoring ---\n";
        
        // Start GPU timing
        auto timing_result = graphics_->begin_gpu_timing("Frame Rendering");
        if (!timing_result) {
            std::cerr << "Failed to begin GPU timing: " << timing_result.error().message() << std::endl;
            return;
        }
        uint32_t timing_query = timing_result.value();
        
        // Simulate some work
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        
        // End GPU timing
        auto end_timing_result = graphics_->end_gpu_timing(timing_query);
        if (!end_timing_result) {
            std::cerr << "Failed to end GPU timing: " << end_timing_result.error().message() << std::endl;
            return;
        }
        
        // Get timing result
        auto timing_time_result = graphics_->get_gpu_timing_result(timing_query);
        if (timing_time_result) {
            std::cout << "GPU frame time: " << timing_time_result.value() << " microseconds" << std::endl;
        }
        
        // Get graphics statistics
        auto graphics_stats = graphics_->get_stats();
        std::cout << "\n--- Graphics Statistics ---\n";
        std::cout << "Frame count: " << graphics_stats.frame_count << std::endl;
        std::cout << "Draw call count: " << graphics_stats.draw_call_count << std::endl;
        std::cout << "Triangle count: " << graphics_stats.triangle_count << std::endl;
        std::cout << "Average frame time: " << std::fixed << std::setprecision(2) 
                  << graphics_stats.average_frame_time_ms << " ms" << std::endl;
        std::cout << "GPU utilization: " << std::fixed << std::setprecision(1) 
                  << graphics_stats.gpu_utilization << "%" << std::endl;
        std::cout << "Texture memory: " << (graphics_stats.texture_memory_used / 1024) << " KB" << std::endl;
        std::cout << "Buffer memory: " << (graphics_stats.buffer_memory_used / 1024) << " KB" << std::endl;
        std::cout << "Total graphics memory: " << (graphics_stats.total_graphics_memory_used / 1024) << " KB" << std::endl;
        
        // Get memory usage
        auto memory_result = graphics_->get_gpu_memory_usage();
        auto available_result = graphics_->get_available_gpu_memory();
        
        if (memory_result && available_result) {
            std::cout << "GPU memory used: " << (memory_result.value() / 1024) << " KB" << std::endl;
            std::cout << "GPU memory available: " << (available_result.value() / (1024*1024)) << " MB" << std::endl;
        }
    }
    
    void demonstrate_adaptive_rendering() {
        std::cout << "\n--- Adaptive Rendering Based on Capabilities ---\n";
        
        if (graphics_->supports_graphics_capability(GraphicsCapability::TileBasedRendering)) {
            std::cout << "Using tile-based rendering optimizations..." << std::endl;
            std::cout << "  - Optimizing for bandwidth efficiency" << std::endl;
            std::cout << "  - Reducing memory traffic" << std::endl;
        }
        
        if (graphics_->supports_graphics_capability(GraphicsCapability::StoreQueues)) {
            std::cout << "PowerVR2 Store Queues detected!" << std::endl;
            auto* sq_interface = graphics_->get_store_queue_interface();
            if (sq_interface) {
                std::cout << "  - Using store queue direct memory access" << std::endl;
                std::cout << "  - Bypassing CPU cache for optimal performance" << std::endl;
            }
        }
        
        if (graphics_->supports_graphics_capability(GraphicsCapability::DisplayLists)) {
            std::cout << "Display Lists supported!" << std::endl;
            auto* dl_interface = graphics_->get_display_list_interface();
            if (dl_interface) {
                std::cout << "  - Pre-compiling geometry commands" << std::endl;
                std::cout << "  - Reducing CPU overhead" << std::endl;
            }
        }
        
        if (graphics_->supports_graphics_capability(GraphicsCapability::ComputeShaders)) {
            std::cout << "Compute shaders available!" << std::endl;
            std::cout << "  - Could offload particle systems to GPU" << std::endl;
            std::cout << "  - Could perform GPU-based culling" << std::endl;
        }
        
        // Adapt rendering strategy based on performance tier
        auto* mock_driver = dynamic_cast<examples::MockGraphicsDriver*>(graphics_.get());
        const PlatformInfo& platform_info2 = mock_driver ? mock_driver->get_platform_info() : PlatformInfo{};
        switch (platform_info2.performance_tier) {
            case PerformanceTier::Minimal:
                std::cout << "Minimal performance tier detected - using ultra-conservative settings" << std::endl;
                break;
            case PerformanceTier::Limited:
                std::cout << "Limited performance tier - balancing quality and performance" << std::endl;
                break;
            case PerformanceTier::Standard:
                std::cout << "Standard performance tier - enabling enhanced features" << std::endl;
                break;
            case PerformanceTier::High:
                std::cout << "High performance tier - enabling all visual effects" << std::endl;
                break;
        }
    }
    
    void demonstrate_resource_management() {
        std::cout << "\n--- Resource Management ---\n";
        
        // Create temporary resources to demonstrate management
        std::cout << "Creating temporary resources..." << std::endl;
        
        std::vector<GraphicsResourceHandle> temp_textures;
        for (int i = 0; i < 5; ++i) {
            auto rt_result = graphics_->create_render_target(
                128, 128, TextureFormat::RGBA8888, 
                "Temp RT " + std::to_string(i)
            );
            if (rt_result) {
                temp_textures.push_back(rt_result.value());
            }
        }
        
        // Check memory usage
        auto memory_before = graphics_->get_gpu_memory_usage();
        std::cout << "Memory before cleanup: " << (memory_before.value() / 1024) << " KB" << std::endl;
        
        // Clean up temporary resources
        std::cout << "Cleaning up temporary resources..." << std::endl;
        for (auto& texture : temp_textures) {
            graphics_->destroy_resource(texture);
        }
        
        auto memory_after = graphics_->get_gpu_memory_usage();
        std::cout << "Memory after cleanup: " << (memory_after.value() / 1024) << " KB" << std::endl;
        
        size_t freed = memory_before.value() - memory_after.value();
        std::cout << "Freed " << (freed / 1024) << " KB of GPU memory" << std::endl;
    }
    
    void cleanup() {
        std::cout << "\n--- Cleaning Up ---\n";
        
        // Destroy all resources
        if (vertex_buffer_.id != 0) graphics_->destroy_resource(vertex_buffer_);
        if (index_buffer_.id != 0) graphics_->destroy_resource(index_buffer_);
        if (texture_.id != 0) graphics_->destroy_resource(texture_);
        if (vertex_shader_.id != 0) graphics_->destroy_resource(vertex_shader_);
        if (fragment_shader_.id != 0) graphics_->destroy_resource(fragment_shader_);
        if (render_target_.id != 0) graphics_->destroy_resource(render_target_);
        
        // Reset command buffer
        command_buffer_.reset();
        
        // Shutdown graphics
        auto shutdown_result = graphics_->shutdown();
        if (!shutdown_result) {
            std::cerr << "Failed to shutdown graphics: " << shutdown_result.error().message() << std::endl;
        } else {
            std::cout << "Graphics system shut down successfully" << std::endl;
        }
    }
    
    void run() {
        if (!initialize()) {
            return;
        }
        
        demonstrate_rendering();
        demonstrate_performance_monitoring();
        demonstrate_adaptive_rendering();
        demonstrate_resource_management();
        
        cleanup();
        
        std::cout << "\n=== Graphics Example Complete ===\n";
    }
};

int main() {
    try {
        GraphicsExample example;
        example.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}
