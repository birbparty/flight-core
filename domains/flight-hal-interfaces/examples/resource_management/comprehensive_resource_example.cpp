/**
 * @file comprehensive_resource_example.cpp
 * @brief Comprehensive Resource Management System Example
 * 
 * Demonstrates RAII patterns, budget enforcement, pool management,
 * and cross-driver resource sharing in the Flight HAL system.
 */

#include "flight/hal/core/resource_manager.hpp"
#include "flight/hal/core/hal_logging.hpp"
#include "flight/hal/coordination/resource_handle.hpp"
#include <iostream>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>

using namespace flight::hal;
using namespace flight::hal::core;
using namespace flight::hal::coordination;

// Example resource types for demonstration
struct TextureResource {
    std::vector<uint8_t> data;
    uint32_t width, height;
    
    TextureResource(uint32_t w, uint32_t h) : width(w), height(h) {
        data.resize(w * h * 4); // RGBA
    }
};

struct AudioBufferResource {
    std::vector<float> samples;
    uint32_t sample_rate;
    
    AudioBufferResource(uint32_t rate, uint32_t duration_ms) 
        : sample_rate(rate) {
        samples.resize((rate * duration_ms) / 1000);
    }
};

/**
 * @brief Example demonstrating basic RAII resource management
 */
void demonstrate_basic_raii() {
    std::cout << "\n=== Basic RAII Resource Management ===\n";
    
    auto& resource_manager = ResourceManager::instance();
    
    // Initialize resource manager
    auto init_result = resource_manager.initialize();
    if (!init_result) {
        std::cout << "Failed to initialize resource manager\n";
        return;
    }
    
    {
        // Create metadata for a texture resource
        ResourceMetadata texture_metadata;
        texture_metadata.type = ResourceType::Memory;
        texture_metadata.size_bytes = 1024 * 1024; // 1MB texture
        texture_metadata.alignment_bytes = 16;
        texture_metadata.access_pattern = AccessPattern::ReadWrite;
        texture_metadata.priority = ResourcePriority::Normal;
        texture_metadata.flags = ResourceFlags::Cacheable | ResourceFlags::GPUAccessible;
        texture_metadata.description = "Game texture resource";
        
        // Acquire texture resource with RAII
        auto texture_result = resource_manager.acquire_resource<TextureResource>(
            "MainMenuTexture", texture_metadata);
        
        if (texture_result) {
            auto texture_ref = std::move(texture_result.value());
            
            std::cout << "Successfully acquired texture resource: " 
                      << texture_ref.handle().name() << "\n";
            std::cout << "Resource ID: " << texture_ref.handle().id() << "\n";
            std::cout << "Resource size: " << texture_metadata.size_bytes << " bytes\n";
            
            // Use the resource
            if (texture_ref.is_valid()) {
                // Resource is automatically managed and will be cleaned up
                // when texture_ref goes out of scope
                std::cout << "Using texture resource...\n";
            }
            
        } else {
            std::cout << "Failed to acquire texture resource: " 
                      << texture_result.error().message() << "\n";
        }
        
        // Resource is automatically released here when texture_ref destructor runs
        std::cout << "Texture resource automatically released\n";
    }
    
    // Demonstrate scoped resource management
    {
        ResourceMetadata audio_metadata;
        audio_metadata.type = ResourceType::Memory;
        audio_metadata.size_bytes = 512 * 1024; // 512KB audio buffer
        audio_metadata.alignment_bytes = 8;
        audio_metadata.access_pattern = AccessPattern::Streaming;
        audio_metadata.priority = ResourcePriority::High;
        audio_metadata.flags = ResourceFlags::DMACapable;
        audio_metadata.description = "Audio playback buffer";
        
        auto scoped_result = resource_manager.acquire_scoped_resource(
            "AudioBuffer", audio_metadata);
        
        if (scoped_result) {
            auto scoped_resource = std::move(scoped_result.value());
            
            std::cout << "Acquired scoped audio resource: " 
                      << scoped_resource.handle().name() << "\n";
            
            // Resource will be automatically released when scoped_resource destructor runs
        }
        
        std::cout << "Scoped audio resource automatically released\n";
    }
}

/**
 * @brief Example demonstrating budget management and pressure handling
 */
void demonstrate_budget_management() {
    std::cout << "\n=== Budget Management and Pressure Handling ===\n";
    
    auto& resource_manager = ResourceManager::instance();
    
    // Set up resource budgets
    ResourceBudget memory_budget(10 * 1024 * 1024, 1024 * 1024, 75, 90); // 10MB max, 1MB reserved
    ResourceBudget hardware_budget(5 * 1024 * 1024, 512 * 1024, 80, 95);  // 5MB max, 512KB reserved
    
    resource_manager.set_budget(ResourceType::Memory, memory_budget);
    resource_manager.set_budget(ResourceType::Hardware, hardware_budget);
    
    std::cout << "Set memory budget: " << memory_budget.max_bytes / (1024 * 1024) << "MB\n";
    std::cout << "Set hardware budget: " << hardware_budget.max_bytes / (1024 * 1024) << "MB\n";
    
    // Register pressure callback
    resource_manager.register_pressure_callback([](ResourceType type, ResourcePressure pressure, const ResourceStats& stats) {
        std::cout << "PRESSURE ALERT: Type=" << static_cast<int>(type) 
                  << ", Level=" << static_cast<int>(pressure)
                  << ", Usage=" << stats.current_usage / 1024 << "KB\n";
    });
    
    // Register reclamation callback
    resource_manager.register_reclamation_callback(ResourceType::Memory, 
        [](ResourceType type, size_t requested_bytes) -> size_t {
            std::cout << "RECLAMATION: Attempting to free " << requested_bytes / 1024 << "KB\n";
            // In a real implementation, this would free caches, temporary resources, etc.
            return requested_bytes / 2; // Simulate freeing half of requested amount
        });
    
    // Allocate resources to trigger pressure
    std::vector<ResourceRef<TextureResource>> textures;
    
    for (int i = 0; i < 8; ++i) {
        ResourceMetadata metadata;
        metadata.type = ResourceType::Memory;
        metadata.size_bytes = 1536 * 1024; // 1.5MB per texture
        metadata.alignment_bytes = 16;
        metadata.access_pattern = AccessPattern::ReadOnly;
        metadata.priority = ResourcePriority::Normal;
        metadata.flags = ResourceFlags::Cacheable;
        metadata.description = "Test texture " + std::to_string(i);
        
        auto result = resource_manager.acquire_resource<TextureResource>(
            "TestTexture" + std::to_string(i), metadata);
        
        if (result) {
            textures.push_back(std::move(result.value()));
            std::cout << "Allocated texture " << i << " (1.5MB)\n";
            
            // Check resource statistics
            auto stats_result = resource_manager.get_resource_stats(ResourceType::Memory);
            if (stats_result) {
                const auto& stats = stats_result.value();
                std::cout << "  Current usage: " << stats.current_usage / 1024 << "KB"
                          << ", Allocations: " << stats.allocation_count << "\n";
            }
        } else {
            std::cout << "Failed to allocate texture " << i << ": " 
                      << result.error().message() << "\n";
            break;
        }
        
        // Small delay to observe pressure changes
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "Final texture count: " << textures.size() << "\n";
    
    // Resources will be automatically cleaned up when textures vector is destroyed
    textures.clear();
    std::cout << "All textures released\n";
}

/**
 * @brief Example demonstrating resource pooling
 */
void demonstrate_resource_pooling() {
    std::cout << "\n=== Resource Pooling ===\n";
    
    auto& resource_manager = ResourceManager::instance();
    auto& pool_manager = resource_manager.pool_manager();
    
    // Create a custom pool for small audio buffers
    PoolManager::PoolConfig audio_pool_config{
        .type = ResourceType::Memory,
        .block_size = 4096, // 4KB blocks
        .initial_count = 10,
        .max_count = 50,
        .alignment = 8,
        .thread_safe = true,
        .name = "SmallAudioBufferPool"
    };
    
    auto pool_result = pool_manager.create_pool(audio_pool_config);
    if (pool_result) {
        std::cout << "Created audio buffer pool (4KB blocks, 10 initial)\n";
    } else {
        std::cout << "Failed to create pool: " << pool_result.error().message() << "\n";
        return;
    }
    
    // Allocate resources from the pool
    std::vector<ResourceRef<AudioBufferResource>> audio_buffers;
    
    for (int i = 0; i < 15; ++i) {
        ResourceMetadata metadata;
        metadata.type = ResourceType::Memory;
        metadata.size_bytes = 4096; // Match pool block size
        metadata.alignment_bytes = 8;
        metadata.access_pattern = AccessPattern::Streaming;
        metadata.priority = ResourcePriority::High;
        metadata.flags = ResourceFlags::DMACapable;
        metadata.description = "Pooled audio buffer " + std::to_string(i);
        
        auto result = resource_manager.acquire_resource<AudioBufferResource>(
            "AudioBuffer" + std::to_string(i), metadata);
        
        if (result) {
            audio_buffers.push_back(std::move(result.value()));
            std::cout << "Allocated pooled audio buffer " << i << "\n";
        } else {
            std::cout << "Failed to allocate audio buffer " << i << "\n";
        }
    }
    
    // Get pool statistics
    auto stats_result = pool_manager.get_pool_stats(ResourceType::Memory, 4096);
    if (stats_result) {
        const auto& stats = stats_result.value();
        std::cout << "Pool stats - Used: " << stats.used_bytes / 1024 << "KB"
                  << ", Free: " << stats.free_bytes / 1024 << "KB"
                  << ", Allocations: " << stats.allocation_count << "\n";
    }
    
    // Release half the buffers to demonstrate pool reuse
    audio_buffers.resize(audio_buffers.size() / 2);
    std::cout << "Released half the audio buffers\n";
    
    // Allocate more to show pool reuse
    for (int i = 15; i < 20; ++i) {
        ResourceMetadata metadata;
        metadata.type = ResourceType::Memory;
        metadata.size_bytes = 4096;
        metadata.alignment_bytes = 8;
        metadata.access_pattern = AccessPattern::Streaming;
        metadata.priority = ResourcePriority::High;
        metadata.flags = ResourceFlags::DMACapable;
        metadata.description = "Reused pooled buffer " + std::to_string(i);
        
        auto result = resource_manager.acquire_resource<AudioBufferResource>(
            "ReusedAudioBuffer" + std::to_string(i), metadata);
        
        if (result) {
            audio_buffers.push_back(std::move(result.value()));
            std::cout << "Allocated reused buffer " << i << " (likely from pool)\n";
        }
    }
}

/**
 * @brief Example demonstrating cross-driver resource sharing
 */
void demonstrate_resource_sharing() {
    std::cout << "\n=== Cross-Driver Resource Sharing ===\n";
    
    auto& resource_manager = ResourceManager::instance();
    
    // Create a resource in the graphics driver
    ResourceMetadata vertex_buffer_metadata;
    vertex_buffer_metadata.type = ResourceType::Hardware;
    vertex_buffer_metadata.size_bytes = 256 * 1024; // 256KB vertex buffer
    vertex_buffer_metadata.alignment_bytes = 16;
    vertex_buffer_metadata.access_pattern = AccessPattern::ReadOnly;
    vertex_buffer_metadata.priority = ResourcePriority::High;
    vertex_buffer_metadata.flags = ResourceFlags::Shareable | ResourceFlags::GPUAccessible;
    vertex_buffer_metadata.description = "Shared vertex buffer";
    
    auto vertex_buffer_result = resource_manager.acquire_resource<void>(
        "SharedVertexBuffer", vertex_buffer_metadata);
    
    if (!vertex_buffer_result) {
        std::cout << "Failed to create vertex buffer\n";
        return;
    }
    
    auto vertex_buffer_ref = std::move(vertex_buffer_result.value());
    std::cout << "Created shared vertex buffer: " << vertex_buffer_ref.handle().name() << "\n";
    
    // Share the resource with the audio driver (for visualization)
    auto shared_result = resource_manager.share_resource(
        vertex_buffer_ref.handle(), "AudioDriver");
    
    if (shared_result) {
        auto shared_handle = shared_result.value();
        std::cout << "Successfully shared resource with AudioDriver\n";
        std::cout << "Shared resource name: " << shared_handle.name() << "\n";
        std::cout << "Shared resource ID: " << shared_handle.id() << "\n";
        
        // The audio driver can now access this resource
        // In a real implementation, it would receive the shared handle
        // through the cross-driver messaging system
        
    } else {
        std::cout << "Failed to share resource: " << shared_result.error().message() << "\n";
    }
}

/**
 * @brief Example demonstrating emergency resource allocation
 */
void demonstrate_emergency_allocation() {
    std::cout << "\n=== Emergency Resource Allocation ===\n";
    
    auto& resource_manager = ResourceManager::instance();
    
    // Set a very tight budget to force emergency scenarios
    ResourceBudget tight_budget(2 * 1024 * 1024, 512 * 1024, 50, 75); // 2MB max, very tight
    resource_manager.set_budget(ResourceType::Hardware, tight_budget);
    
    std::cout << "Set tight hardware budget: 2MB max\n";
    
    // Try to allocate more than the budget allows
    ResourceMetadata large_resource_metadata;
    large_resource_metadata.type = ResourceType::Hardware;
    large_resource_metadata.size_bytes = 3 * 1024 * 1024; // 3MB - exceeds budget
    large_resource_metadata.alignment_bytes = 16;
    large_resource_metadata.access_pattern = AccessPattern::ReadWrite;
    large_resource_metadata.priority = ResourcePriority::Critical;
    large_resource_metadata.flags = ResourceFlags::None;
    large_resource_metadata.description = "Large emergency resource";
    
    // Try normal allocation first (should fail)
    auto normal_result = resource_manager.acquire_resource<void>(
        "LargeResource", large_resource_metadata, AcquisitionMode::NonBlocking);
    
    if (!normal_result) {
        std::cout << "Normal allocation failed as expected: " 
                  << normal_result.error().message() << "\n";
        
        // Try emergency allocation
        auto emergency_result = resource_manager.acquire_resource<void>(
            "EmergencyResource", large_resource_metadata, AcquisitionMode::Emergency);
        
        if (emergency_result) {
            auto emergency_ref = std::move(emergency_result.value());
            std::cout << "Emergency allocation succeeded: " 
                      << emergency_ref.handle().name() << "\n";
            
            // In a real system, this would only be used for critical operations
            std::cout << "Emergency resource should be released ASAP\n";
            
        } else {
            std::cout << "Even emergency allocation failed: " 
                      << emergency_result.error().message() << "\n";
        }
    }
}

/**
 * @brief Main demonstration function
 */
int main() {
    std::cout << "Flight HAL Resource Management System Demo\n";
    std::cout << "==========================================\n";
    
    try {
        // Initialize logging
        flight::hal::HALLogger::set_log_level(flight::hal::LogLevel::Info);
        
        // Run demonstrations
        demonstrate_basic_raii();
        demonstrate_budget_management();
        demonstrate_resource_pooling();
        demonstrate_resource_sharing();
        demonstrate_emergency_allocation();
        
        // Shutdown resource manager
        auto& resource_manager = ResourceManager::instance();
        auto shutdown_result = resource_manager.shutdown();
        if (shutdown_result) {
            std::cout << "\nResource Manager shutdown successfully\n";
        }
        
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
        return 1;
    }
    
    std::cout << "\nDemo completed successfully!\n";
    return 0;
}
