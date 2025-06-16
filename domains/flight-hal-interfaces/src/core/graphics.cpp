/**
 * @file graphics.cpp
 * @brief Flight HAL Elite Graphics Interface Implementation
 * 
 * Implementation of graphics utility functions and supporting code for the
 * unified graphics interface spanning PowerVR2 to modern programmable pipelines.
 */

#include "../../include/flight/hal/interfaces/graphics.hpp"
#include <cstring>
#include <algorithm>

namespace flight::hal::graphics {

const char* to_string(GraphicsCapability capability) {
    switch (capability) {
        case GraphicsCapability::FixedFunctionPipeline: return "FixedFunctionPipeline";
        case GraphicsCapability::ProgrammableShaders: return "ProgrammableShaders";
        case GraphicsCapability::ComputeShaders: return "ComputeShaders";
        case GraphicsCapability::GeometryShaders: return "GeometryShaders";
        case GraphicsCapability::TessellationShaders: return "TessellationShaders";
        case GraphicsCapability::TileBasedRendering: return "TileBasedRendering";
        case GraphicsCapability::ImmediateModeRendering: return "ImmediateModeRendering";
        case GraphicsCapability::MultipleRenderTargets: return "MultipleRenderTargets";
        case GraphicsCapability::DepthBuffer: return "DepthBuffer";
        case GraphicsCapability::StencilBuffer: return "StencilBuffer";
        case GraphicsCapability::TextureCompression: return "TextureCompression";
        case GraphicsCapability::MipmapGeneration: return "MipmapGeneration";
        case GraphicsCapability::CubeMapping: return "CubeMapping";
        case GraphicsCapability::VolumeTextures: return "VolumeTextures";
        case GraphicsCapability::FloatingPointTextures: return "FloatingPointTextures";
        case GraphicsCapability::MultiThreadedCommands: return "MultiThreadedCommands";
        case GraphicsCapability::ExplicitSynchronization: return "ExplicitSynchronization";
        case GraphicsCapability::UnifiedMemory: return "UnifiedMemory";
        case GraphicsCapability::RayTracing: return "RayTracing";
        case GraphicsCapability::VariableRateShading: return "VariableRateShading";
        case GraphicsCapability::StoreQueues: return "StoreQueues";
        case GraphicsCapability::DisplayLists: return "DisplayLists";
        case GraphicsCapability::CoherentMemory: return "CoherentMemory";
        case GraphicsCapability::VertexBufferObjects: return "VertexBufferObjects";
        case GraphicsCapability::IndexBufferObjects: return "IndexBufferObjects";
        case GraphicsCapability::UniformBufferObjects: return "UniformBufferObjects";
        case GraphicsCapability::StorageBufferObjects: return "StorageBufferObjects";
        default: return "Unknown";
    }
}

const char* to_string(TextureFormat format) {
    switch (format) {
        case TextureFormat::RGB565: return "RGB565";
        case TextureFormat::RGBA4444: return "RGBA4444";
        case TextureFormat::RGBA1555: return "RGBA1555";
        case TextureFormat::RGB888: return "RGB888";
        case TextureFormat::RGBA8888: return "RGBA8888";
        case TextureFormat::DXT1: return "DXT1";
        case TextureFormat::DXT3: return "DXT3";
        case TextureFormat::DXT5: return "DXT5";
        case TextureFormat::PVRTC_2BPP: return "PVRTC_2BPP";
        case TextureFormat::PVRTC_4BPP: return "PVRTC_4BPP";
        case TextureFormat::R16F: return "R16F";
        case TextureFormat::RG16F: return "RG16F";
        case TextureFormat::RGBA16F: return "RGBA16F";
        case TextureFormat::R32F: return "R32F";
        case TextureFormat::RG32F: return "RG32F";
        case TextureFormat::RGBA32F: return "RGBA32F";
        case TextureFormat::Depth16: return "Depth16";
        case TextureFormat::Depth24: return "Depth24";
        case TextureFormat::Depth32F: return "Depth32F";
        case TextureFormat::Depth24Stencil8: return "Depth24Stencil8";
        case TextureFormat::Depth32FStencil8: return "Depth32FStencil8";
        default: return "Unknown";
    }
}

const char* to_string(PrimitiveTopology topology) {
    switch (topology) {
        case PrimitiveTopology::PointList: return "PointList";
        case PrimitiveTopology::LineList: return "LineList";
        case PrimitiveTopology::LineStrip: return "LineStrip";
        case PrimitiveTopology::TriangleList: return "TriangleList";
        case PrimitiveTopology::TriangleStrip: return "TriangleStrip";
        case PrimitiveTopology::TriangleFan: return "TriangleFan";
        default: return "Unknown";
    }
}

const char* to_string(BlendOp op) {
    switch (op) {
        case BlendOp::Add: return "Add";
        case BlendOp::Subtract: return "Subtract";
        case BlendOp::ReverseSubtract: return "ReverseSubtract";
        case BlendOp::Min: return "Min";
        case BlendOp::Max: return "Max";
        default: return "Unknown";
    }
}

const char* to_string(BlendFactor factor) {
    switch (factor) {
        case BlendFactor::Zero: return "Zero";
        case BlendFactor::One: return "One";
        case BlendFactor::SrcColor: return "SrcColor";
        case BlendFactor::InvSrcColor: return "InvSrcColor";
        case BlendFactor::SrcAlpha: return "SrcAlpha";
        case BlendFactor::InvSrcAlpha: return "InvSrcAlpha";
        case BlendFactor::DstColor: return "DstColor";
        case BlendFactor::InvDstColor: return "InvDstColor";
        case BlendFactor::DstAlpha: return "DstAlpha";
        case BlendFactor::InvDstAlpha: return "InvDstAlpha";
        default: return "Unknown";
    }
}

uint32_t get_bytes_per_pixel(TextureFormat format) {
    switch (format) {
        case TextureFormat::RGB565:
        case TextureFormat::RGBA4444:
        case TextureFormat::RGBA1555:
        case TextureFormat::R16F:
        case TextureFormat::Depth16:
            return 2;
            
        case TextureFormat::RGB888:
            return 3;
            
        case TextureFormat::RGBA8888:
        case TextureFormat::RG16F:
        case TextureFormat::R32F:
        case TextureFormat::Depth24:
        case TextureFormat::Depth32F:
        case TextureFormat::Depth24Stencil8:
            return 4;
            
        case TextureFormat::RGBA16F:
        case TextureFormat::RG32F:
        case TextureFormat::Depth32FStencil8:
            return 8;
            
        case TextureFormat::RGBA32F:
            return 16;
            
        // Compressed formats - approximate bytes per pixel
        case TextureFormat::DXT1:
        case TextureFormat::PVRTC_2BPP:
            return 1; // Approximate - actual compression varies
            
        case TextureFormat::DXT3:
        case TextureFormat::DXT5:
        case TextureFormat::PVRTC_4BPP:
            return 1; // Approximate - actual compression varies
            
        default:
            return 4; // Default to 32-bit RGBA
    }
}

bool is_compressed_format(TextureFormat format) {
    switch (format) {
        case TextureFormat::DXT1:
        case TextureFormat::DXT3:
        case TextureFormat::DXT5:
        case TextureFormat::PVRTC_2BPP:
        case TextureFormat::PVRTC_4BPP:
            return true;
        default:
            return false;
    }
}

bool is_depth_stencil_format(TextureFormat format) {
    switch (format) {
        case TextureFormat::Depth16:
        case TextureFormat::Depth24:
        case TextureFormat::Depth32F:
        case TextureFormat::Depth24Stencil8:
        case TextureFormat::Depth32FStencil8:
            return true;
        default:
            return false;
    }
}

size_t calculate_texture_size(uint32_t width, uint32_t height, 
                             TextureFormat format, uint32_t mip_levels) {
    if (is_compressed_format(format)) {
        // Compressed formats are more complex to calculate exactly
        // This is a simplified approximation
        size_t block_size = 0;
        size_t blocks_per_row = 0;
        size_t blocks_per_col = 0;
        
        switch (format) {
            case TextureFormat::DXT1:
            case TextureFormat::PVRTC_2BPP:
                block_size = 8;
                blocks_per_row = std::max(1u, (width + 3) / 4);
                blocks_per_col = std::max(1u, (height + 3) / 4);
                break;
                
            case TextureFormat::DXT3:
            case TextureFormat::DXT5:
            case TextureFormat::PVRTC_4BPP:
                block_size = 16;
                blocks_per_row = std::max(1u, (width + 3) / 4);
                blocks_per_col = std::max(1u, (height + 3) / 4);
                break;
                
            default:
                break;
        }
        
        size_t total_size = 0;
        uint32_t mip_width = width;
        uint32_t mip_height = height;
        
        for (uint32_t level = 0; level < mip_levels; ++level) {
            size_t mip_blocks_per_row = std::max(1u, (mip_width + 3) / 4);
            size_t mip_blocks_per_col = std::max(1u, (mip_height + 3) / 4);
            total_size += mip_blocks_per_row * mip_blocks_per_col * block_size;
            
            mip_width = std::max(1u, mip_width / 2);
            mip_height = std::max(1u, mip_height / 2);
        }
        
        return total_size;
    } else {
        // Uncompressed formats
        uint32_t bytes_per_pixel = get_bytes_per_pixel(format);
        size_t total_size = 0;
        uint32_t mip_width = width;
        uint32_t mip_height = height;
        
        for (uint32_t level = 0; level < mip_levels; ++level) {
            total_size += mip_width * mip_height * bytes_per_pixel;
            mip_width = std::max(1u, mip_width / 2);
            mip_height = std::max(1u, mip_height / 2);
        }
        
        return total_size;
    }
}

TextureDescriptor make_texture_descriptor(uint32_t width, uint32_t height, 
                                         TextureFormat format) {
    TextureDescriptor desc = {};
    desc.width = width;
    desc.height = height;
    desc.depth = 1;
    desc.mip_levels = 1;
    desc.format = format;
    desc.usage = BufferUsage::Static;
    desc.generate_mipmaps = false;
    desc.debug_name = {};
    return desc;
}

BufferDescriptor make_buffer_descriptor(size_t size, 
                                       GraphicsResourceType type,
                                       BufferUsage usage) {
    BufferDescriptor desc = {};
    desc.size = size;
    desc.usage = usage;
    desc.type = type;
    desc.stride = 0; // Will be set by application based on vertex format
    desc.debug_name = {};
    return desc;
}

RenderState make_default_render_state() {
    RenderState state = {};
    
    // Blend state - disabled by default
    state.blend_enable = false;
    state.src_blend = BlendFactor::One;
    state.dst_blend = BlendFactor::Zero;
    state.blend_op = BlendOp::Add;
    state.src_blend_alpha = BlendFactor::One;
    state.dst_blend_alpha = BlendFactor::Zero;
    state.blend_op_alpha = BlendOp::Add;
    
    // Depth/stencil state - depth testing enabled
    state.depth_test_enable = true;
    state.depth_write_enable = true;
    state.depth_func = CompareFunc::Less;
    state.stencil_enable = false;
    
    // Rasterizer state
    state.cull_mode = CullMode::Back;
    state.fill_mode = FillMode::Solid;
    state.scissor_enable = false;
    
    // PowerVR2 specific states - disabled by default
    state.fog_enable = false;
    state.fog_color = 0x00000000; // Black fog
    state.fog_density = 1.0f;
    
    return state;
}

Viewport make_viewport(float width, float height) {
    Viewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = width;
    viewport.height = height;
    viewport.min_depth = 0.0f;
    viewport.max_depth = 1.0f;
    return viewport;
}

ScissorRect make_scissor_rect(int32_t x, int32_t y, uint32_t width, uint32_t height) {
    ScissorRect rect = {};
    rect.x = x;
    rect.y = y;
    rect.width = width;
    rect.height = height;
    return rect;
}

} // namespace flight::hal::graphics
