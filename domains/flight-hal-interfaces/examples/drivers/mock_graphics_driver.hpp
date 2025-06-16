/**
 * @file mock_graphics_driver.hpp
 * @brief Mock Graphics Driver for Flight HAL
 * 
 * Complete reference implementation of the graphics interface for testing,
 * development, and demonstration purposes. Simulates both PowerVR2-style
 * fixed-function and modern programmable pipeline capabilities.
 */

#pragma once

#include "../../include/flight/hal/interfaces/graphics.hpp"
#include "../../include/flight/hal/core/driver_auto_registration.hpp"
#include <unordered_map>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <chrono>
#include <thread>

namespace flight::hal::examples {

/**
 * @brief Mock graphics resource implementation
 */
class MockGraphicsResource : public IGraphicsResource {
public:
    MockGraphicsResource(GraphicsResourceHandle handle, size_t size, std::string_view debug_name)
        : handle_(handle), size_(size), debug_name_(debug_name), valid_(true) {}
    
    ~MockGraphicsResource() override = default;
    
    GraphicsResourceType get_type() const override { return handle_.type; }
    GraphicsResourceHandle get_handle() const override { return handle_; }
    size_t get_size() const override { return size_; }
    bool is_valid() const override { return valid_; }
    std::string_view get_debug_name() const override { return debug_name_; }
    
    void set_debug_name(std::string_view name) override {
        debug_name_ = std::string(name);
    }
    
    void invalidate() { valid_ = false; }
    
private:
    GraphicsResourceHandle handle_;
    size_t size_;
    std::string debug_name_;
    bool valid_;
};

/**
 * @brief Mock command buffer implementation
 */
class MockCommandBuffer : public ICommandBuffer {
public:
    MockCommandBuffer(uint32_t id) : id_(id), recording_(false) {
        reset();
    }
    
    ~MockCommandBuffer() override = default;
    
    // Command Recording
    HALResult<void> begin() override {
        if (recording_) {
            return HALResult<void>::error(errors::invalid_state(1, "Command buffer is already recording"));
        }
        recording_ = true;
        commands_.clear();
        stats_.command_count = 0;
        stats_.draw_call_count = 0;
        stats_.state_change_count = 0;
        stats_.resource_bind_count = 0;
        return HALResult<void>::success();
    }
    
    HALResult<void> end() override {
        if (!recording_) {
            return HALResult<void>::error(errors::invalid_state(2, "Command buffer is not recording"));
        }
        recording_ = false;
        return HALResult<void>::success();
    }
    
    HALResult<void> reset() override {
        recording_ = false;
        commands_.clear();
        stats_ = {};
        return HALResult<void>::success();
    }
    
    // Render State Commands
    HALResult<void> set_render_state(const RenderState& state) override {
        if (!recording_) return HALResult<void>::error(errors::invalid_state(3, "Not recording"));
        
        commands_.push_back("SetRenderState");
        stats_.command_count++;
        stats_.state_change_count++;
        current_render_state_ = state;
        return HALResult<void>::success();
    }
    
    HALResult<void> set_viewport(const Viewport& viewport) override {
        if (!recording_) return HALResult<void>::error(errors::invalid_state(4, "Not recording"));
        
        commands_.push_back("SetViewport");
        stats_.command_count++;
        current_viewport_ = viewport;
        return HALResult<void>::success();
    }
    
    HALResult<void> set_scissor(const ScissorRect& scissor) override {
        if (!recording_) return HALResult<void>::error(errors::invalid_state(5, "Not recording"));
        
        commands_.push_back("SetScissor");
        stats_.command_count++;
        current_scissor_ = scissor;
        return HALResult<void>::success();
    }
    
    // Resource Binding
    HALResult<void> bind_texture(uint32_t slot, GraphicsResourceHandle texture) override {
        if (!recording_) return HALResult<void>::error(errors::invalid_state(6, "Not recording"));
        
        commands_.push_back("BindTexture[" + std::to_string(slot) + "]");
        stats_.command_count++;
        stats_.resource_bind_count++;
        bound_textures_[slot] = texture;
        return HALResult<void>::success();
    }
    
    HALResult<void> bind_vertex_buffer(uint32_t slot, GraphicsResourceHandle buffer, 
                                      uint32_t stride, uint32_t offset) override {
        if (!recording_) return HALResult<void>::error(errors::invalid_state(7, "Not recording"));
        
        commands_.push_back("BindVertexBuffer[" + std::to_string(slot) + "]");
        stats_.command_count++;
        stats_.resource_bind_count++;
        bound_vertex_buffers_[slot] = {buffer, stride, offset};
        return HALResult<void>::success();
    }
    
    HALResult<void> bind_index_buffer(GraphicsResourceHandle buffer, uint32_t offset) override {
        if (!recording_) return HALResult<void>::error(errors::invalid_state(8, "Not recording"));
        
        commands_.push_back("BindIndexBuffer");
        stats_.command_count++;
        stats_.resource_bind_count++;
        bound_index_buffer_ = {buffer, 0, offset};
        return HALResult<void>::success();
    }
    
    HALResult<void> bind_uniform_buffer(uint32_t slot, GraphicsResourceHandle buffer) override {
        if (!recording_) return HALResult<void>::error(errors::invalid_state(9, "Not recording"));
        
        commands_.push_back("BindUniformBuffer[" + std::to_string(slot) + "]");
        stats_.command_count++;
        stats_.resource_bind_count++;
        bound_uniform_buffers_[slot] = buffer;
        return HALResult<void>::success();
    }
    
    HALResult<void> bind_shaders(GraphicsResourceHandle vertex_shader,
                                GraphicsResourceHandle fragment_shader) override {
        if (!recording_) return HALResult<void>::error(errors::invalid_state(10, "Not recording"));
        
        commands_.push_back("BindShaders");
        stats_.command_count++;
        stats_.resource_bind_count++;
        bound_vertex_shader_ = vertex_shader;
        bound_fragment_shader_ = fragment_shader;
        return HALResult<void>::success();
    }
    
    HALResult<void> bind_pipeline(GraphicsResourceHandle pipeline) override {
        if (!recording_) return HALResult<void>::error(errors::invalid_state(11, "Not recording"));
        
        commands_.push_back("BindPipeline");
        stats_.command_count++;
        stats_.resource_bind_count++;
        bound_pipeline_ = pipeline;
        return HALResult<void>::success();
    }
    
    // Drawing Commands
    HALResult<void> draw(const DrawCommand& command) override {
        if (!recording_) return HALResult<void>::error(errors::invalid_state(12, "Not recording"));
        
        commands_.push_back("Draw(" + std::to_string(command.vertex_count) + " vertices)");
        stats_.command_count++;
        stats_.draw_call_count++;
        
        // Estimate triangle count based on topology
        uint32_t triangles = 0;
        switch (command.topology) {
            case PrimitiveTopology::TriangleList:
                triangles = command.vertex_count / 3;
                break;
            case PrimitiveTopology::TriangleStrip:
            case PrimitiveTopology::TriangleFan:
                triangles = command.vertex_count >= 3 ? command.vertex_count - 2 : 0;
                break;
            default:
                triangles = 0;
                break;
        }
        triangle_count_ += triangles;
        
        return HALResult<void>::success();
    }
    
    HALResult<void> draw_indexed(const DrawIndexedCommand& command) override {
        if (!recording_) return HALResult<void>::error(errors::invalid_state(13, "Not recording"));
        
        commands_.push_back("DrawIndexed(" + std::to_string(command.index_count) + " indices)");
        stats_.command_count++;
        stats_.draw_call_count++;
        
        // Estimate triangle count based on topology
        uint32_t triangles = 0;
        switch (command.topology) {
            case PrimitiveTopology::TriangleList:
                triangles = command.index_count / 3;
                break;
            case PrimitiveTopology::TriangleStrip:
            case PrimitiveTopology::TriangleFan:
                triangles = command.index_count >= 3 ? command.index_count - 2 : 0;
                break;
            default:
                triangles = 0;
                break;
        }
        triangle_count_ += triangles;
        
        return HALResult<void>::success();
    }
    
    // Clear Commands
    HALResult<void> clear_render_target(const std::array<float, 4>& color) override {
        if (!recording_) return HALResult<void>::error(errors::invalid_state(14, "Not recording"));
        
        commands_.push_back("ClearRenderTarget");
        stats_.command_count++;
        return HALResult<void>::success();
    }
    
    HALResult<void> clear_depth_stencil(float depth, uint8_t stencil) override {
        if (!recording_) return HALResult<void>::error(errors::invalid_state(15, "Not recording"));
        
        commands_.push_back("ClearDepthStencil");
        stats_.command_count++;
        return HALResult<void>::success();
    }
    
    // Resource Operations
    HALResult<void> copy_resource(GraphicsResourceHandle src, GraphicsResourceHandle dst) override {
        if (!recording_) return HALResult<void>::error(errors::invalid_state(16, "Not recording"));
        
        commands_.push_back("CopyResource");
        stats_.command_count++;
        return HALResult<void>::success();
    }
    
    // Modern API Features
    HALResult<void> begin_render_pass(const std::vector<GraphicsResourceHandle>& render_targets,
                                     GraphicsResourceHandle depth_stencil) override {
        if (!recording_) return HALResult<void>::error(errors::invalid_state(17, "Not recording"));
        
        commands_.push_back("BeginRenderPass");
        stats_.command_count++;
        return HALResult<void>::success();
    }
    
    HALResult<void> end_render_pass() override {
        if (!recording_) return HALResult<void>::error(errors::invalid_state(18, "Not recording"));
        
        commands_.push_back("EndRenderPass");
        stats_.command_count++;
        return HALResult<void>::success();
    }
    
    HALResult<void> dispatch_compute(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z) override {
        if (!recording_) return HALResult<void>::error(errors::invalid_state(19, "Not recording"));
        
        commands_.push_back("DispatchCompute");
        stats_.command_count++;
        return HALResult<void>::success();
    }
    
    // Statistics and Debug
    CommandBufferStats get_stats() const override {
        auto stats = stats_;
        stats.memory_used = commands_.size() * 64; // Estimate 64 bytes per command
        return stats;
    }
    
    bool is_recording() const override { return recording_; }
    
    HALResult<void> set_debug_marker(std::string_view name) override {
        if (!recording_) return HALResult<void>::error(errors::invalid_state(20, "Not recording"));
        
        commands_.push_back("DebugMarker: " + std::string(name));
        stats_.command_count++;
        return HALResult<void>::success();
    }
    
    HALResult<void> begin_debug_group(std::string_view name) override {
        if (!recording_) return HALResult<void>::error(errors::invalid_state(21, "Not recording"));
        
        commands_.push_back("BeginDebugGroup: " + std::string(name));
        stats_.command_count++;
        return HALResult<void>::success();
    }
    
    HALResult<void> end_debug_group() override {
        if (!recording_) return HALResult<void>::error(errors::invalid_state(22, "Not recording"));
        
        commands_.push_back("EndDebugGroup");
        stats_.command_count++;
        return HALResult<void>::success();
    }
    
    // Access to recorded commands for testing
    const std::vector<std::string>& get_commands() const { return commands_; }
    uint32_t get_triangle_count() const { return triangle_count_; }
    
private:
    uint32_t id_;
    bool recording_;
    std::vector<std::string> commands_;
    CommandBufferStats stats_;
    uint32_t triangle_count_ = 0;
    
    // Current state
    RenderState current_render_state_;
    Viewport current_viewport_;
    ScissorRect current_scissor_;
    
    // Bound resources
    std::unordered_map<uint32_t, GraphicsResourceHandle> bound_textures_;
    std::unordered_map<uint32_t, GraphicsResourceHandle> bound_uniform_buffers_;
    
    struct BufferBinding {
        GraphicsResourceHandle handle;
        uint32_t stride;
        uint32_t offset;
    };
    std::unordered_map<uint32_t, BufferBinding> bound_vertex_buffers_;
    BufferBinding bound_index_buffer_ = {};
    
    GraphicsResourceHandle bound_vertex_shader_ = {};
    GraphicsResourceHandle bound_fragment_shader_ = {};
    GraphicsResourceHandle bound_pipeline_ = {};
};

/**
 * @brief Mock graphics driver implementation
 */
class MockGraphicsDriver : public IGraphicsInterface {
private:
    // Core state
    bool initialized_;
    std::atomic<uint32_t> next_resource_id_;
    std::atomic<uint32_t> next_command_buffer_id_;
    
    // Device information
    GraphicsDeviceInfo device_info_;
    PlatformInfo platform_info_;
    
    // Resource management
    std::mutex resources_mutex_;
    std::unordered_map<uint32_t, std::unique_ptr<MockGraphicsResource>> resources_;
    std::mutex command_buffers_mutex_;
    std::unordered_map<ICommandBuffer*, std::unique_ptr<MockCommandBuffer>> command_buffers_;
    
    // Memory tracking
    std::atomic<size_t> texture_memory_used_;
    std::atomic<size_t> buffer_memory_used_;
    
    // Statistics
    GraphicsStats stats_;
    std::chrono::high_resolution_clock::time_point frame_start_time_;
    
    // Backbuffer
    GraphicsResourceHandle backbuffer_;

public:
    MockGraphicsDriver() : initialized_(false), next_resource_id_(1), next_command_buffer_id_(1),
                          texture_memory_used_(0), buffer_memory_used_(0) {
        // Initialize device info with mixed capabilities (simulating adaptive hardware)
        device_info_.device_name = "Mock Graphics Device";
        device_info_.vendor_name = "Flight HAL";
        device_info_.driver_version = "1.0.0";
        
        // Support both fixed-function and modern capabilities for demonstration
        device_info_.capability_mask = 
            static_cast<uint32_t>(GraphicsCapability::FixedFunctionPipeline) |
            static_cast<uint32_t>(GraphicsCapability::ProgrammableShaders) |
            static_cast<uint32_t>(GraphicsCapability::TileBasedRendering) |
            static_cast<uint32_t>(GraphicsCapability::ImmediateModeRendering) |
            static_cast<uint32_t>(GraphicsCapability::MultipleRenderTargets) |
            static_cast<uint32_t>(GraphicsCapability::DepthBuffer) |
            static_cast<uint32_t>(GraphicsCapability::StencilBuffer) |
            static_cast<uint32_t>(GraphicsCapability::TextureCompression) |
            static_cast<uint32_t>(GraphicsCapability::MipmapGeneration) |
            static_cast<uint32_t>(GraphicsCapability::CubeMapping) |
            static_cast<uint32_t>(GraphicsCapability::VertexBufferObjects) |
            static_cast<uint32_t>(GraphicsCapability::IndexBufferObjects) |
            static_cast<uint32_t>(GraphicsCapability::UniformBufferObjects);
        
        device_info_.max_texture_size = 2048;
        device_info_.max_texture_array_size = 256;
        device_info_.max_render_targets = 4;
        device_info_.max_vertex_attributes = 16;
        device_info_.max_uniform_buffers = 8;
        device_info_.total_graphics_memory = 64 * 1024 * 1024; // 64MB (Dreamcast-like)
        device_info_.available_graphics_memory = 48 * 1024 * 1024;
        device_info_.performance_tier = PerformanceTier::Limited;
        device_info_.unified_memory = false;
        device_info_.coherent_memory = true;
        
        // Initialize platform info
        platform_info_.platform_name = "Mock Platform";
        platform_info_.architecture = "Mock-64";
        platform_info_.performance_tier = PerformanceTier::Limited;
        platform_info_.total_memory = 64 * 1024 * 1024;
        platform_info_.cpu_cores = 1;
        platform_info_.has_fpu = true;
        platform_info_.has_simd = false;
        
        // Reset statistics
        stats_ = {};
        frame_start_time_ = std::chrono::high_resolution_clock::now();
    }
    
    ~MockGraphicsDriver() override {
        if (initialized_) {
            shutdown();
        }
    }
    
    // Device Management
    HALResult<void> initialize() override {
        if (initialized_) {
            return HALResult<void>::error(errors::initialization_failed(1, "Graphics driver already initialized"));
        }
        
        initialized_ = true;
        
        // Create default backbuffer
        TextureDescriptor backbuffer_desc = graphics::make_texture_descriptor(640, 480);
        backbuffer_desc.format = TextureFormat::RGBA8888;
        backbuffer_desc.debug_name = "Backbuffer";
        auto backbuffer_result = create_texture(backbuffer_desc);
        if (!backbuffer_result) {
            return HALResult<void>::error(errors::initialization_failed(2, "Failed to create backbuffer"));
        }
        backbuffer_ = backbuffer_result.value();
        
        return HALResult<void>::success();
    }
    
    HALResult<void> shutdown() override {
        if (!initialized_) {
            return HALResult<void>::error(errors::invalid_state(23, "Graphics driver not initialized"));
        }
        
        // Clean up all resources
        std::lock_guard<std::mutex> lock(resources_mutex_);
        resources_.clear();
        command_buffers_.clear();
        
        initialized_ = false;
        return HALResult<void>::success();
    }
    
    const GraphicsDeviceInfo& get_device_info() const override { return device_info_; }
    bool is_initialized() const override { return initialized_; }
    
    // Command Buffer Management
    HALResult<std::unique_ptr<ICommandBuffer>> create_command_buffer() override {
        if (!initialized_) {
            return HALResult<std::unique_ptr<ICommandBuffer>>::error(
                errors::invalid_state(24, "Not initialized"));
        }
        
        auto cmd_buffer = std::make_unique<MockCommandBuffer>(next_command_buffer_id_.fetch_add(1));
        ICommandBuffer* ptr = cmd_buffer.get();
        
        std::lock_guard<std::mutex> lock(command_buffers_mutex_);
        command_buffers_[ptr] = std::move(cmd_buffer);
        
        // Return a new unique_ptr - the mock manages its own lifetime
        uint32_t cb_id = next_command_buffer_id_.fetch_add(1);
        return HALResult<std::unique_ptr<ICommandBuffer>>::success(
            std::make_unique<MockCommandBuffer>(cb_id));
    }
    
    HALResult<void> submit_command_buffer(ICommandBuffer* command_buffer, bool wait_for_completion) override {
        if (!initialized_) {
            return HALResult<void>::error(errors::invalid_state(25, "Not initialized"));
        }
        
        auto* mock_cb = dynamic_cast<MockCommandBuffer*>(command_buffer);
        if (!mock_cb) {
            return HALResult<void>::error(errors::invalid_parameter(1, "Invalid command buffer"));
        }
        
        // Simulate command execution
        auto cb_stats = mock_cb->get_stats();
        stats_.draw_call_count += cb_stats.draw_call_count;
        stats_.triangle_count += mock_cb->get_triangle_count();
        stats_.render_state_changes += cb_stats.state_change_count;
        
        if (wait_for_completion) {
            // Simulate GPU work
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        return HALResult<void>::success();
    }
    
    HALResult<void> submit_command_buffers(const std::vector<ICommandBuffer*>& command_buffers,
                                          bool wait_for_completion) override {
        for (auto* cb : command_buffers) {
            auto result = submit_command_buffer(cb, false);
            if (!result) {
                return result;
            }
        }
        
        if (wait_for_completion) {
            return wait_for_gpu();
        }
        
        return HALResult<void>::success();
    }
    
    // Resource Creation
    HALResult<GraphicsResourceHandle> create_texture(const TextureDescriptor& descriptor,
                                                    const void* initial_data) override {
        if (!initialized_) {
            return HALResult<GraphicsResourceHandle>::error(
                errors::invalid_state(26, "Not initialized"));
        }
        
        // Calculate texture size
        size_t texture_size = graphics::calculate_texture_size(
            descriptor.width, descriptor.height, descriptor.format, descriptor.mip_levels);
        
        // Create resource handle
        GraphicsResourceHandle handle = {};
        handle.id = next_resource_id_.fetch_add(1);
        handle.type = GraphicsResourceType::Texture2D;
        handle.generation = 1;
        
        // Create resource
        auto resource = std::make_unique<MockGraphicsResource>(handle, texture_size, descriptor.debug_name);
        
        std::lock_guard<std::mutex> lock(resources_mutex_);
        resources_[handle.id] = std::move(resource);
        
        // Update memory usage
        texture_memory_used_.fetch_add(texture_size);
        
        return HALResult<GraphicsResourceHandle>::success(handle);
    }
    
    HALResult<GraphicsResourceHandle> create_buffer(const BufferDescriptor& descriptor,
                                                   const void* initial_data) override {
        if (!initialized_) {
            return HALResult<GraphicsResourceHandle>::error(
                errors::invalid_state(27, "Not initialized"));
        }
        
        // Create resource handle
        GraphicsResourceHandle handle = {};
        handle.id = next_resource_id_.fetch_add(1);
        handle.type = descriptor.type;
        handle.generation = 1;
        
        // Create resource
        auto resource = std::make_unique<MockGraphicsResource>(handle, descriptor.size, descriptor.debug_name);
        
        std::lock_guard<std::mutex> lock(resources_mutex_);
        resources_[handle.id] = std::move(resource);
        
        // Update memory usage
        buffer_memory_used_.fetch_add(descriptor.size);
        
        return HALResult<GraphicsResourceHandle>::success(handle);
    }
    
    HALResult<GraphicsResourceHandle> create_shader(const ShaderDescriptor& descriptor) override {
        if (!initialized_) {
            return HALResult<GraphicsResourceHandle>::error(
                errors::invalid_state(28, "Not initialized"));
        }
        
        // Create resource handle
        GraphicsResourceHandle handle = {};
        handle.id = next_resource_id_.fetch_add(1);
        handle.type = descriptor.type;
        handle.generation = 1;
        
        // Create resource
        auto resource = std::make_unique<MockGraphicsResource>(handle, descriptor.bytecode_size, descriptor.debug_name);
        
        std::lock_guard<std::mutex> lock(resources_mutex_);
        resources_[handle.id] = std::move(resource);
        
        return HALResult<GraphicsResourceHandle>::success(handle);
    }
    
    HALResult<GraphicsResourceHandle> create_render_target(uint32_t width, uint32_t height,
                                                          TextureFormat format,
                                                          std::string_view debug_name) override {
        TextureDescriptor desc = graphics::make_texture_descriptor(width, height);
        desc.format = format;
        desc.debug_name = debug_name;
        
        auto handle_result = create_texture(desc);
        if (!handle_result) {
            return handle_result;
        }
        
        auto handle = handle_result.value();
        handle.type = GraphicsResourceType::RenderTarget;
        
        // Update the resource type in storage
        std::lock_guard<std::mutex> lock(resources_mutex_);
        if (auto it = resources_.find(handle.id); it != resources_.end()) {
            // Update the handle in the resource
            auto new_resource = std::make_unique<MockGraphicsResource>(handle, it->second->get_size(), debug_name);
            resources_[handle.id] = std::move(new_resource);
        }
        
        return HALResult<GraphicsResourceHandle>::success(handle);
    }
    
    HALResult<GraphicsResourceHandle> create_framebuffer(const std::vector<GraphicsResourceHandle>& render_targets,
                                                        GraphicsResourceHandle depth_stencil,
                                                        std::string_view debug_name) override {
        if (!initialized_) {
            return HALResult<GraphicsResourceHandle>::error(
                errors::invalid_state(29, "Not initialized"));
        }
        
        // Create resource handle
        GraphicsResourceHandle handle = {};
        handle.id = next_resource_id_.fetch_add(1);
        handle.type = GraphicsResourceType::Framebuffer;
        handle.generation = 1;
        
        // Create resource (minimal size for framebuffer metadata)
        auto resource = std::make_unique<MockGraphicsResource>(handle, 256, debug_name);
        
        std::lock_guard<std::mutex> lock(resources_mutex_);
        resources_[handle.id] = std::move(resource);
        
        return HALResult<GraphicsResourceHandle>::success(handle);
    }
    
    // Resource Management
    HALResult<void> destroy_resource(GraphicsResourceHandle resource) override {
        std::lock_guard<std::mutex> lock(resources_mutex_);
        auto it = resources_.find(resource.id);
        if (it == resources_.end()) {
            return HALResult<void>::error(errors::invalid_parameter(2, "Resource not found"));
        }
        
        // Update memory usage
        size_t size = it->second->get_size();
        switch (resource.type) {
            case GraphicsResourceType::Texture2D:
            case GraphicsResourceType::TextureCube:
            case GraphicsResourceType::Texture3D:
            case GraphicsResourceType::RenderTarget:
                texture_memory_used_.fetch_sub(size);
                break;
            case GraphicsResourceType::VertexBuffer:
            case GraphicsResourceType::IndexBuffer:
            case GraphicsResourceType::UniformBuffer:
            case GraphicsResourceType::StorageBuffer:
                buffer_memory_used_.fetch_sub(size);
                break;
            default:
                break;
        }
        
        resources_.erase(it);
        return HALResult<void>::success();
    }
    
    HALResult<IGraphicsResource*> get_resource(GraphicsResourceHandle resource) override {
        std::lock_guard<std::mutex> lock(resources_mutex_);
        auto it = resources_.find(resource.id);
        if (it == resources_.end()) {
            return HALResult<IGraphicsResource*>::error(
                errors::invalid_parameter(3, "Resource not found"));
        }
        return HALResult<IGraphicsResource*>::success(it->second.get());
    }
    
    HALResult<void> update_buffer(GraphicsResourceHandle buffer, const void* data, 
                                 size_t size, size_t offset) override {
        auto resource_result = get_resource(buffer);
        if (!resource_result) {
            return HALResult<void>::error(errors::invalid_parameter(4, "Buffer not found"));
        }
        
        // Simulate buffer update
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        return HALResult<void>::success();
    }
    
    HALResult<void> update_texture(GraphicsResourceHandle texture, const void* data,
                                  uint32_t width, uint32_t height,
                                  uint32_t x, uint32_t y, uint32_t mip_level) override {
        auto resource_result = get_resource(texture);
        if (!resource_result) {
            return HALResult<void>::error(errors::invalid_parameter(5, "Texture not found"));
        }
        
        // Simulate texture update
        std::this_thread::sleep_for(std::chrono::microseconds(50));
        return HALResult<void>::success();
    }
    
    HALResult<void> generate_mipmaps(GraphicsResourceHandle texture) override {
        if (!supports_graphics_capability(GraphicsCapability::MipmapGeneration)) {
            return HALResult<void>::error(errors::feature_not_supported(1, "Mipmap generation not supported"));
        }
        
        auto resource_result = get_resource(texture);
        if (!resource_result) {
            return HALResult<void>::error(errors::invalid_parameter(6, "Texture not found"));
        }
        
        // Simulate mipmap generation
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        return HALResult<void>::success();
    }
    
    // GPU Synchronization
    HALResult<void> wait_for_gpu() override {
        // Simulate GPU wait
        std::this_thread::sleep_for(std::chrono::microseconds(50));
        return HALResult<void>::success();
    }
    
    bool is_gpu_idle() const override {
        // Simulate GPU state
        return true;
    }
    
    HALResult<void> flush() override {
        // Simulate command flush
        return HALResult<void>::success();
    }
    
    // Present and Display
    HALResult<void> present(bool vsync) override {
        if (!initialized_) {
            return HALResult<void>::error(errors::invalid_state(30, "Not initialized"));
        }
        
        // Update frame statistics
        stats_.frame_count++;
        
        auto now = std::chrono::high_resolution_clock::now();
        auto frame_time = std::chrono::duration_cast<std::chrono::microseconds>(now - frame_start_time_);
        stats_.average_frame_time_ms = frame_time.count() / 1000.0;
        frame_start_time_ = now;
        
        // Simulate present operation
        if (vsync) {
            std::this_thread::sleep_for(std::chrono::microseconds(16667)); // ~60 FPS
        }
        
        return HALResult<void>::success();
    }
    
    HALResult<GraphicsResourceHandle> get_backbuffer() override {
        if (!initialized_) {
            return HALResult<GraphicsResourceHandle>::error(errors::invalid_state(31, "Not initialized"));
        }
        return HALResult<GraphicsResourceHandle>::success(backbuffer_);
    }
    
    HALResult<void> resize_backbuffer(uint32_t width, uint32_t height) override {
        if (!initialized_) {
            return HALResult<void>::error(errors::invalid_state(32, "Not initialized"));
        }
        
        // Destroy old backbuffer and create new one
        destroy_resource(backbuffer_);
        
        TextureDescriptor desc = graphics::make_texture_descriptor(width, height);
        desc.format = TextureFormat::RGBA8888;
        desc.debug_name = "Backbuffer";
        auto result = create_texture(desc);
        if (!result) {
            return HALResult<void>::error(errors::initialization_failed(3, "Failed to create new backbuffer"));
        }
        
        backbuffer_ = result.value();
        backbuffer_.type = GraphicsResourceType::RenderTarget;
        
        return HALResult<void>::success();
    }
    
    // Capability Queries
    bool supports_texture_format(TextureFormat format) const override {
        // Mock driver supports most common formats
        switch (format) {
            case TextureFormat::RGB565:
            case TextureFormat::RGBA4444:
            case TextureFormat::RGBA1555:
            case TextureFormat::RGB888:
            case TextureFormat::RGBA8888:
            case TextureFormat::DXT1:
            case TextureFormat::DXT3:
            case TextureFormat::DXT5:
            case TextureFormat::PVRTC_2BPP:
            case TextureFormat::PVRTC_4BPP:
            case TextureFormat::Depth16:
            case TextureFormat::Depth24:
            case TextureFormat::Depth32F:
            case TextureFormat::Depth24Stencil8:
                return true;
            default:
                return false;
        }
    }
    
    bool supports_graphics_capability(GraphicsCapability capability) const override {
        return (device_info_.capability_mask & static_cast<uint32_t>(capability)) != 0;
    }
    
    uint32_t get_max_texture_size() const override {
        return device_info_.max_texture_size;
    }
    
    uint32_t get_max_render_targets() const override {
        return device_info_.max_render_targets;
    }
    
    uint32_t get_max_vertex_attributes() const override {
        return device_info_.max_vertex_attributes;
    }
    
    // Statistics and Performance
    GraphicsStats get_stats() const override {
        auto stats = stats_;
        stats.texture_memory_used = texture_memory_used_.load();
        stats.buffer_memory_used = buffer_memory_used_.load();
        stats.total_graphics_memory_used = stats.texture_memory_used + stats.buffer_memory_used;
        stats.gpu_utilization = 75.0; // Simulated GPU utilization
        return stats;
    }
    
    HALResult<void> reset_stats() override {
        stats_ = {};
        frame_start_time_ = std::chrono::high_resolution_clock::now();
        return HALResult<void>::success();
    }
    
    HALResult<size_t> get_gpu_memory_usage() const override {
        return HALResult<size_t>::success(texture_memory_used_.load() + buffer_memory_used_.load());
    }
    
    HALResult<size_t> get_available_gpu_memory() const override {
        size_t used = texture_memory_used_.load() + buffer_memory_used_.load();
        size_t available = device_info_.total_graphics_memory - used;
        return HALResult<size_t>::success(available);
    }
    
    // Debug and Profiling
    HALResult<uint32_t> begin_gpu_timing(std::string_view name) override {
        // Simulate GPU timing
        return HALResult<uint32_t>::success(1); // Return fake query ID
    }
    
    HALResult<void> end_gpu_timing(uint32_t query_id) override {
        // Simulate ending GPU timing
        return HALResult<void>::success();
    }
    
    HALResult<uint64_t> get_gpu_timing_result(uint32_t query_id) override {
        // Return simulated timing (1ms)
        return HALResult<uint64_t>::success(1000);
    }
    
    HALResult<void> capture_frame() override {
        // Simulate frame capture
        return HALResult<void>::success();
    }
    
    // Platform-Specific Extensions
    void* get_extension_interface(std::string_view extension_name) override {
        // No extensions supported in mock
        return nullptr;
    }
    
    void* get_store_queue_interface() override {
        // Only available on PowerVR2/Dreamcast
        if (supports_graphics_capability(GraphicsCapability::StoreQueues)) {
            return this; // Return mock interface
        }
        return nullptr;
    }
    
    void* get_display_list_interface() override {
        // Only available on PowerVR2/Dreamcast
        if (supports_graphics_capability(GraphicsCapability::DisplayLists)) {
            return this; // Return mock interface
        }
        return nullptr;
    }
    
    // ICapabilityProvider implementation
    bool supports_capability(HALCapability capability) const override {
        // Map HAL capabilities to graphics capabilities
        switch (capability) {
            case HALCapability::Hardware3D:
                return supports_graphics_capability(GraphicsCapability::FixedFunctionPipeline) ||
                       supports_graphics_capability(GraphicsCapability::ProgrammableShaders);
            case HALCapability::Hardware2D:
                return true; // Always supported in mock
            case HALCapability::VertexShaders:
                return supports_graphics_capability(GraphicsCapability::ProgrammableShaders);
            case HALCapability::FragmentShaders:
                return supports_graphics_capability(GraphicsCapability::ProgrammableShaders);
            case HALCapability::ComputeShaders:
                return supports_graphics_capability(GraphicsCapability::ComputeShaders);
            default:
                return false;
        }
    }
    
    uint32_t get_capability_mask() const override {
        uint32_t mask = 0;
        if (supports_capability(HALCapability::Hardware3D)) mask |= static_cast<uint32_t>(HALCapability::Hardware3D);
        if (supports_capability(HALCapability::Hardware2D)) mask |= static_cast<uint32_t>(HALCapability::Hardware2D);
        if (supports_capability(HALCapability::VertexShaders)) mask |= static_cast<uint32_t>(HALCapability::VertexShaders);
        if (supports_capability(HALCapability::FragmentShaders)) mask |= static_cast<uint32_t>(HALCapability::FragmentShaders);
        if (supports_capability(HALCapability::ComputeShaders)) mask |= static_cast<uint32_t>(HALCapability::ComputeShaders);
        return mask;
    }
    
    std::vector<HALCapability> get_capabilities() const override {
        std::vector<HALCapability> caps;
        if (supports_capability(HALCapability::Hardware3D)) caps.push_back(HALCapability::Hardware3D);
        if (supports_capability(HALCapability::Hardware2D)) caps.push_back(HALCapability::Hardware2D);
        if (supports_capability(HALCapability::VertexShaders)) caps.push_back(HALCapability::VertexShaders);
        if (supports_capability(HALCapability::FragmentShaders)) caps.push_back(HALCapability::FragmentShaders);
        if (supports_capability(HALCapability::ComputeShaders)) caps.push_back(HALCapability::ComputeShaders);
        return caps;
    }
    
    PerformanceTier get_performance_tier() const override {
        return platform_info_.performance_tier;
    }
    
    const PlatformInfo& get_platform_info() const override {
        return platform_info_;
    }
    
    bool has_fallback(HALCapability capability) const override {
        // Most graphics capabilities have software fallbacks
        switch (capability) {
            case HALCapability::Hardware3D:
            case HALCapability::Hardware2D:
            case HALCapability::VertexShaders:
            case HALCapability::FragmentShaders:
                return true;
            default:
                return false;
        }
    }
};

// Auto-register the mock graphics driver
REGISTER_HAL_DRIVER(IGraphicsInterface, MockGraphicsDriver);

} // namespace flight::hal::examples
