# Getting Started with Flight HAL

**Your First Flight HAL Application in 15 Minutes**

Flight HAL is designed to get you up and running quickly while providing access to sophisticated cross-platform capabilities. This guide will walk you through creating your first application using Flight HAL.

## Prerequisites

Before getting started, ensure you have:

- **C++17 or later** compiler (GCC 7+, Clang 5+, MSVC 2017+)
- **CMake 3.16+** for building
- **Git** for cloning the repository
- Platform-specific development tools:
  - **Windows**: Visual Studio 2017+ or MinGW
  - **macOS**: Xcode Command Line Tools
  - **Linux**: GCC/Clang development packages
  - **Web**: Emscripten SDK
  - **Dreamcast**: KallistiOS toolchain
  - **PSP**: PSP toolchain

## Installation

### Option 1: Using CMake FetchContent (Recommended)

Add Flight HAL to your CMakeLists.txt:

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyFlightApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch Flight HAL
include(FetchContent)
FetchContent_Declare(
    flight_hal
    GIT_REPOSITORY https://github.com/flight-engine/hal-interfaces.git
    GIT_TAG main
)
FetchContent_MakeAvailable(flight_hal)

# Your executable
add_executable(my_flight_app main.cpp)
target_link_libraries(my_flight_app flight::hal)
```

### Option 2: Git Submodule

```bash
# Add as submodule
git submodule add https://github.com/flight-engine/hal-interfaces.git third_party/flight-hal
git submodule update --init --recursive

# In your CMakeLists.txt
add_subdirectory(third_party/flight-hal)
target_link_libraries(my_flight_app flight::hal)
```

### Option 3: Manual Download

Download and extract the latest release, then add to your CMake project:

```cmake
add_subdirectory(path/to/flight-hal)
target_link_libraries(my_flight_app flight::hal)
```

## Your First Application

Create a simple application that initializes Flight HAL and displays platform information:

### main.cpp

```cpp
#include <flight/hal/platform.hpp>
#include <iostream>

int main() {
    std::cout << "🚀 Flight HAL Application Starting...\n\n";
    
    try {
        // Create platform instance
        auto platform = flight::hal::Platform::create();
        if (!platform) {
            std::cerr << "❌ Failed to create platform\n";
            return 1;
        }
        
        // Initialize the platform
        auto init_result = platform->initialize();
        if (!init_result) {
            std::cerr << "❌ Failed to initialize platform: " 
                      << init_result.error().message << std::endl;
            return 1;
        }
        
        std::cout << "✅ Platform initialized successfully!\n\n";
        
        // Get platform information
        auto platform_info = platform->get_platform_info();
        std::cout << "Platform Information:\n";
        std::cout << "  Name: " << platform_info.name << "\n";
        std::cout << "  Version: " << platform_info.version << "\n";
        std::cout << "  Architecture: " << platform_info.architecture << "\n";
        
        // Get available interfaces
        auto available_interfaces = platform->get_available_interfaces();
        std::cout << "\nAvailable Interfaces:\n";
        for (const auto& interface_name : available_interfaces) {
            std::cout << "  ✓ " << interface_name << "\n";
        }
        
        // Try to get specific interfaces
        std::cout << "\nInterface Status:\n";
        
        // Memory interface (always available)
        auto memory = platform->get_memory();
        if (memory) {
            auto memory_stats = memory->get_memory_stats();
            if (memory_stats) {
                std::cout << "  Memory: " 
                          << memory_stats.value().total_bytes / (1024*1024) 
                          << " MB total\n";
            }
        }
        
        // Graphics interface
        auto graphics = platform->get_graphics();
        if (graphics) {
            auto graphics_result = graphics->initialize();
            if (graphics_result) {
                const auto& device_info = graphics->get_device_info();
                std::cout << "  Graphics: " << device_info.device_name 
                          << " (" << device_info.vendor_name << ")\n";
            } else {
                std::cout << "  Graphics: Available but failed to initialize\n";
            }
        } else {
            std::cout << "  Graphics: Not available\n";
        }
        
        // Audio interface
        auto audio = platform->get_audio();
        if (audio) {
            std::cout << "  Audio: Available\n";
        } else {
            std::cout << "  Audio: Not available\n";
        }
        
        // Input interface
        auto input = platform->get_input();
        if (input) {
            std::cout << "  Input: Available\n";
        } else {
            std::cout << "  Input: Not available\n";
        }
        
        std::cout << "\n🎉 Flight HAL application completed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
```

### Build and Run

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
cmake --build .

# Run
./my_flight_app
```

Expected output:
```
🚀 Flight HAL Application Starting...

✅ Platform initialized successfully!

Platform Information:
  Name: Windows 10
  Version: 10.0.19042
  Architecture: x64

Available Interfaces:
  ✓ memory
  ✓ graphics
  ✓ audio
  ✓ input
  ✓ file
  ✓ network
  ✓ time
  ✓ thread
  ✓ performance

Interface Status:
  Memory: 16384 MB total
  Graphics: NVIDIA GeForce RTX 3080 (NVIDIA Corporation)
  Audio: Available
  Input: Available

🎉 Flight HAL application completed successfully!
```

## Next Steps: Adding Graphics

Let's extend the application to render a simple triangle:

### graphics_example.cpp

```cpp
#include <flight/hal/platform.hpp>
#include <flight/hal/interfaces/graphics.hpp>
#include <iostream>

using namespace flight::hal;

int main() {
    auto platform = Platform::create();
    HAL_TRY(platform->initialize());
    
    // Get graphics interface
    auto graphics = platform->get_graphics();
    if (!graphics) {
        std::cerr << "Graphics not available on this platform\n";
        return 1;
    }
    
    // Initialize graphics
    HAL_TRY(graphics->initialize());
    
    std::cout << "Graphics initialized: " 
              << graphics->get_device_info().device_name << std::endl;
    
    // Create a simple vertex buffer
    struct Vertex {
        float x, y, z;
        float r, g, b;
    };
    
    std::vector<Vertex> triangle_vertices = {
        {-0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f}, // Red bottom-left
        { 0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f}, // Green bottom-right
        { 0.0f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f}  // Blue top-center
    };
    
    // Create vertex buffer
    BufferDescriptor vb_desc = graphics::make_buffer_descriptor(
        triangle_vertices.size() * sizeof(Vertex),
        GraphicsResourceType::VertexBuffer,
        BufferUsage::Static
    );
    vb_desc.stride = sizeof(Vertex);
    vb_desc.debug_name = "TriangleVertices";
    
    auto vertex_buffer = graphics->create_buffer(vb_desc, triangle_vertices.data());
    HAL_TRY_ASSIGN(vb_handle, vertex_buffer);
    
    std::cout << "Created vertex buffer with " << triangle_vertices.size() 
              << " vertices\n";
    
    // Create command buffer for rendering
    auto cmd_buffer = graphics->create_command_buffer();
    HAL_TRY_ASSIGN(cmd, cmd_buffer);
    
    // Record rendering commands
    HAL_TRY(cmd->begin());
    
    // Set viewport to match window
    Viewport viewport = graphics::make_viewport(800.0f, 600.0f);
    HAL_TRY(cmd->set_viewport(viewport));
    
    // Clear to a nice blue color
    HAL_TRY(cmd->clear_render_target({0.2f, 0.3f, 0.8f, 1.0f}));
    
    // Bind vertex buffer
    HAL_TRY(cmd->bind_vertex_buffer(0, vb_handle, sizeof(Vertex)));
    
    // Draw the triangle
    DrawCommand draw_cmd{};
    draw_cmd.topology = PrimitiveTopology::TriangleList;
    draw_cmd.vertex_count = 3;
    draw_cmd.vertex_offset = 0;
    draw_cmd.instance_count = 1;
    
    HAL_TRY(cmd->draw(draw_cmd));
    
    HAL_TRY(cmd->end());
    
    // Submit for rendering
    HAL_TRY(graphics->submit_command_buffer(cmd.get()));
    
    // Present the frame
    HAL_TRY(graphics->present());
    
    std::cout << "Triangle rendered successfully!\n";
    
    // Get rendering statistics
    auto stats = graphics->get_stats();
    std::cout << "Draw calls: " << stats.draw_call_count << std::endl;
    std::cout << "Triangles: " << stats.triangle_count << std::endl;
    
    return 0;
}
```

## Platform-Specific Examples

### Dreamcast (16MB RAM)

```cpp
// Optimized for Dreamcast's limited memory
void dreamcast_optimized_app() {
    auto platform = Platform::create();
    HAL_TRY(platform->initialize());
    
    auto memory = platform->get_memory();
    
    // Use pool allocators for zero fragmentation
    auto sprite_request = memory::make_allocation_request(
        1000 * sizeof(Sprite),
        MemoryType::System,
        MemoryAlignment::Cache
    );
    sprite_request.preferred_allocator = AllocatorType::Pool;
    
    auto sprite_pool = memory->allocate(sprite_request);
    HAL_TRY_ASSIGN(sprites, sprite_pool);
    
    std::cout << "Dreamcast: Pool allocator for sprites created\n";
    
    // Use small textures optimized for PowerVR2
    auto graphics = platform->get_graphics();
    if (graphics && graphics->supports_graphics_capability(
        GraphicsCapability::TileBasedRendering)) {
        
        TextureDescriptor tex_desc = graphics::make_texture_descriptor(
            256, 256,                    // Small texture for 16MB system
            TextureFormat::RGBA4444      // 16-bit to save memory
        );
        
        auto texture = graphics->create_texture(tex_desc);
        if (texture) {
            std::cout << "Dreamcast: PowerVR2-optimized texture created\n";
        }
    }
}
```

### Web Platform

```cpp
// Optimized for web constraints
void web_optimized_app() {
    auto platform = Platform::create();
    HAL_TRY(platform->initialize());
    
    auto graphics = platform->get_graphics();
    const auto& device_info = graphics->get_device_info();
    
    // Check for web platform memory constraints
    if (device_info.total_graphics_memory < 256 * 1024 * 1024) {
        std::cout << "Web platform detected - using optimized settings\n";
        
        // Use compressed textures to save bandwidth
        TextureDescriptor web_tex = graphics::make_texture_descriptor(
            512, 512,
            TextureFormat::DXT1  // Compressed format
        );
        web_tex.mip_levels = 1;  // No mipmaps to save memory
        
        auto texture = graphics->create_texture(web_tex);
        if (texture) {
            std::cout << "Web: Compressed texture created\n";
        }
        
        // Batch draw calls to minimize WebGL overhead
        auto cmd_buffer = graphics->create_command_buffer();
        HAL_TRY_ASSIGN(cmd, cmd_buffer);
        
        HAL_TRY(cmd->begin());
        
        // Set render state once
        RenderState web_state = graphics::make_default_render_state();
        HAL_TRY(cmd->set_render_state(web_state));
        
        // Batch multiple draw calls
        for (int i = 0; i < 10; ++i) {
            DrawCommand draw{};
            draw.topology = PrimitiveTopology::TriangleList;
            draw.vertex_count = 3;
            draw.vertex_offset = i * 3;
            
            HAL_TRY(cmd->draw(draw));
        }
        
        HAL_TRY(cmd->end());
        HAL_TRY(graphics->submit_command_buffer(cmd.get()));
        
        std::cout << "Web: Batched rendering completed\n";
    }
}
```

## Error Handling Best Practices

Flight HAL uses a comprehensive error handling system. Here are the best practices:

### Using HAL_TRY Macros

```cpp
#include <flight/hal/core/hal_error_macros.hpp>

void robust_hal_usage() {
    auto platform = Platform::create();
    
    // HAL_TRY: Early return on error
    HAL_TRY(platform->initialize());
    
    auto graphics = platform->get_graphics();
    HAL_TRY(graphics->initialize());
    
    // HAL_TRY_ASSIGN: Assign value or return on error
    auto cmd_buffer = graphics->create_command_buffer();
    HAL_TRY_ASSIGN(cmd, cmd_buffer);
    
    HAL_TRY(cmd->begin());
    // ... rendering commands ...
    HAL_TRY(cmd->end());
}
```

### Manual Error Handling

```cpp
void manual_error_handling() {
    auto platform = Platform::create();
    
    auto init_result = platform->initialize();
    if (!init_result) {
        HALError error = init_result.error();
        
        switch (error.category) {
            case HALErrorCategory::PlatformError:
                std::cerr << "Platform error: " << error.message << std::endl;
                // Handle platform-specific error
                break;
                
            case HALErrorCategory::OutOfMemory:
                std::cerr << "Out of memory during initialization\n";
                // Free memory and retry with reduced settings
                break;
                
            default:
                std::cerr << "Unknown error: " << error.message << std::endl;
                break;
        }
        return;
    }
    
    // Continue with successful initialization...
}
```

## Memory Management Tips

### Choose the Right Allocator

```cpp
void memory_management_example() {
    auto platform = Platform::create();
    HAL_TRY(platform->initialize());
    
    auto memory = platform->get_memory();
    
    // For game objects with known lifetime - use Pool
    auto pool_request = memory::make_allocation_request(
        1000 * sizeof(GameObject),
        MemoryType::System
    );
    pool_request.preferred_allocator = AllocatorType::Pool;
    auto object_pool = memory->allocate(pool_request);
    
    // For frame-based temporary data - use Linear
    auto linear_request = memory::make_allocation_request(
        256 * 1024,  // 256KB frame buffer
        MemoryType::Temporary
    );
    linear_request.preferred_allocator = AllocatorType::Linear;
    auto frame_buffer = memory->allocate(linear_request);
    
    // For general purpose - use System
    auto general_buffer = memory->allocate(64 * 1024);
    
    std::cout << "Different allocator strategies applied\n";
}
```

### Monitor Memory Pressure

```cpp
void memory_pressure_example() {
    auto platform = Platform::create();
    HAL_TRY(platform->initialize());
    
    auto memory = platform->get_memory();
    
    // Register for memory pressure notifications
    auto callback_result = memory->register_pressure_callback(
        MemoryPressureLevel::Medium,
        [](MemoryPressureLevel level, const MemoryPressureInfo& info) {
            std::cout << "Memory pressure: " << memory::to_string(level) << std::endl;
            
            if (level >= MemoryPressureLevel::High) {
                // Free non-essential resources
                free_texture_cache();
                reduce_audio_quality();
            }
        }
    );
    
    if (callback_result) {
        std::cout << "Memory pressure monitoring enabled\n";
    }
}
```

## Next Steps

Now that you have Flight HAL working, explore these areas:

### 1. **Interface Documentation**
- [Memory Interface](../interfaces/memory/index.md) - Advanced memory management
- [Graphics Interface](../interfaces/graphics/index.md) - Rendering and command buffers
- [Audio Interface](../interfaces/audio/index.md) - Real-time audio processing
- [Input Interface](../interfaces/input/index.md) - Input devices and haptic feedback

### 2. **Platform Guides**
- [Platform Setup](platform-setup.md) - Platform-specific configuration
- [Platform Optimization](../platform-guides/) - Platform-specific best practices

### 3. **Advanced Topics**
- [Cross-Interface Coordination](../examples/cross-interface.md) - Using multiple interfaces together
- [Performance Optimization](../integration/performance-optimization.md) - Getting maximum performance
- [Driver Implementation](../integration/driver-implementation.md) - Creating custom drivers

### 4. **Tools and Testing**
- [HAL Validator](../../hal_validation_tools_guide.md) - Validate your implementation
- [Performance Benchmarking](../../performance_benchmarking_system.md) - Measure performance
- [Stress Testing](../../stress_testing_system.md) - Test under load

## Common Issues

### Build Issues

**CMake can't find Flight HAL:**
```bash
# Ensure CMake version is 3.16+
cmake --version

# Clear CMake cache
rm -rf build/
mkdir build && cd build
cmake ..
```

**Missing C++17 support:**
```cmake
# Add to CMakeLists.txt
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

### Runtime Issues

**Platform initialization fails:**
- Check that your platform is supported
- Verify development tools are installed
- Check platform-specific documentation

**Graphics initialization fails:**
- Ensure graphics drivers are up to date
- Check that graphics context can be created
- Try with different graphics settings

**Memory allocation fails:**
- Check available system memory
- Try smaller allocation sizes
- Use memory pressure monitoring

## Getting Help

If you encounter issues:

1. **Check the logs** - Flight HAL provides detailed error messages
2. **Consult the documentation** - Each interface has comprehensive docs
3. **Try the examples** - Look at working code in the examples directory
4. **Check platform support** - Ensure your platform is supported
5. **File an issue** - Report bugs on the GitHub repository

---

**Congratulations!** You've successfully set up Flight HAL and created your first application. The cross-platform capabilities of Flight HAL are now at your fingertips, ready to power your next game or application across platforms from Dreamcast to the cloud.
