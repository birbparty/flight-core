/**
 * @file comprehensive_capability_detection_example.cpp
 * @brief Comprehensive example demonstrating the Flight HAL capability detection system
 * 
 * Shows hardware detection, performance benchmarking, capability caching,
 * and adaptive performance optimization across different platforms.
 */

#include "flight/hal/core/capability_detection_system.hpp"
#include "flight/hal/core/hardware_detection.hpp"
#include "flight/hal/core/performance_benchmarking.hpp"
#include "flight/hal/core/capability_cache.hpp"
#include "flight/hal/core/hal_logging.hpp"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <bitset>

using namespace flight::hal;

/**
 * @brief Example capability detection event listener
 */
class ExampleCapabilityEventListener : public ICapabilityDetectionEventListener {
public:
    void on_detection_started(const CapabilityDetectionConfig& config) override {
        std::cout << "🔍 Capability detection started with config:\n";
        std::cout << "  - Hardware detection: " << (config.enable_hardware_detection ? "enabled" : "disabled") << "\n";
        std::cout << "  - Performance benchmarks: " << (config.enable_performance_benchmarks ? "enabled" : "disabled") << "\n";
        std::cout << "  - Capability caching: " << (config.enable_capability_caching ? "enabled" : "disabled") << "\n";
        std::cout << "  - Quick mode: " << (config.use_quick_detection_mode ? "enabled" : "disabled") << "\n";
    }
    
    void on_detection_completed(const CapabilityDetectionResults& results) override {
        std::cout << "\n✅ Capability detection completed!\n";
        std::cout << "  Duration: " << results.detection_duration.count() << "ms\n";
        std::cout << "  Confidence: " << std::fixed << std::setprecision(1) 
                  << (results.detection_confidence * 100.0) << "%\n";
        std::cout << "  Performance tier: " << capabilities::to_string(results.performance_tier) << "\n";
        std::cout << "  Capabilities detected: " << std::bitset<32>(results.capability_mask).count() << "/25\n";
        
        if (!results.warnings.empty()) {
            std::cout << "  ⚠️  Warnings: " << results.warnings.size() << "\n";
        }
        if (!results.errors.empty()) {
            std::cout << "  ❌ Errors: " << results.errors.size() << "\n";
        }
    }
    
    void on_detection_failed(const std::string& error_message, 
                           const CapabilityDetectionResults* partial_results) override {
        std::cout << "\n❌ Capability detection failed: " << error_message << "\n";
        if (partial_results) {
            std::cout << "  Partial results available with confidence: " 
                      << (partial_results->detection_confidence * 100.0) << "%\n";
        }
    }
    
    void on_capability_changed(const CapabilityChangeNotification& notification) override {
        std::cout << "\n🔄 Capability changed: " << capabilities::to_string(notification.changed_capability) << "\n";
        std::cout << "  From: " << (notification.was_supported ? "supported" : "not supported") 
                  << " → " << (notification.now_supported ? "supported" : "not supported") << "\n";
        std::cout << "  Reason: " << notification.change_reason << "\n";
    }
    
    void on_performance_tier_changed(PerformanceTier old_tier, 
                                    PerformanceTier new_tier,
                                    const std::string& reason) override {
        std::cout << "\n📊 Performance tier changed: " 
                  << capabilities::to_string(old_tier) << " → " 
                  << capabilities::to_string(new_tier) << "\n";
        std::cout << "  Reason: " << reason << "\n";
    }
};

/**
 * @brief Display hardware detection results
 */
void display_hardware_features(const CapabilityDetectionResults& results) {
    std::cout << "\n🖥️  Hardware Features Detected:\n";
    
    // CPU Features
    std::cout << "\n  CPU Features:\n";
    std::cout << "    Vendor: " << results.cpu_features.vendor << "\n";
    std::cout << "    Model: " << results.cpu_features.model << "\n";
    std::cout << "    Architecture: " << results.cpu_features.architecture << "\n";
    std::cout << "    Cores: " << results.cpu_features.cores << "\n";
    std::cout << "    Threads: " << results.cpu_features.threads << "\n";
    std::cout << "    FPU: " << (results.cpu_features.has_fpu ? "✅" : "❌") << "\n";
    std::cout << "    SIMD: " << (results.cpu_features.has_sse || results.cpu_features.has_neon ? "✅" : "❌") << "\n";
    
    // GPU Features
    std::cout << "\n  GPU Features:\n";
    std::cout << "    Vendor: " << results.gpu_features.vendor << "\n";
    std::cout << "    Device: " << results.gpu_features.device_name << "\n";
    std::cout << "    OpenGL: " << (results.gpu_features.supports_opengl ? "✅" : "❌") << "\n";
    std::cout << "    Vulkan: " << (results.gpu_features.supports_vulkan ? "✅" : "❌") << "\n";
    std::cout << "    Vertex Shaders: " << (results.gpu_features.supports_vertex_shaders ? "✅" : "❌") << "\n";
    std::cout << "    Fragment Shaders: " << (results.gpu_features.supports_fragment_shaders ? "✅" : "❌") << "\n";
    std::cout << "    Compute Shaders: " << (results.gpu_features.supports_compute_shaders ? "✅" : "❌") << "\n";
    
    // Memory Features
    std::cout << "\n  Memory Features:\n";
    std::cout << "    Total Physical: " << (results.memory_features.total_physical / (1024 * 1024)) << " MB\n";
    std::cout << "    Available Physical: " << (results.memory_features.available_physical / (1024 * 1024)) << " MB\n";
    std::cout << "    Virtual Memory: " << (results.memory_features.supports_dma ? "✅" : "❌") << "\n";
    std::cout << "    DMA Support: " << (results.memory_features.supports_dma ? "✅" : "❌") << "\n";
    std::cout << "    Large Pages: " << (results.memory_features.supports_large_pages ? "✅" : "❌") << "\n";
    
    // I/O Features
    std::cout << "\n  I/O Features:\n";
    std::cout << "    Persistent Storage: " << (results.io_features.supports_persistent_storage ? "✅" : "❌") << "\n";
    std::cout << "    Async I/O: " << (results.io_features.supports_async_io ? "✅" : "❌") << "\n";
    std::cout << "    Networking: " << (results.io_features.supports_networking ? "✅" : "❌") << "\n";
    std::cout << "    WiFi: " << (results.io_features.supports_wifi ? "✅" : "❌") << "\n";
    std::cout << "    Bluetooth: " << (results.io_features.supports_bluetooth ? "✅" : "❌") << "\n";
    std::cout << "    Touch Input: " << (results.io_features.supports_touch ? "✅" : "❌") << "\n";
    std::cout << "    Gamepad: " << (results.io_features.supports_gamepad ? "✅" : "❌") << "\n";
}

/**
 * @brief Display performance benchmark results
 */
void display_benchmark_results(const SystemBenchmarkResults& results) {
    std::cout << "\n📊 Performance Benchmark Results:\n";
    
    // CPU Benchmarks
    std::cout << "\n  CPU Performance:\n";
    std::cout << "    Integer Operations: " << std::fixed << std::setprecision(2) 
              << results.cpu.integer_ops.score << " (tier: " 
              << capabilities::to_string(results.cpu.tier) << ")\n";
    std::cout << "    Float Operations: " << results.cpu.float_ops.score << "\n";
    std::cout << "    SIMD Operations: " << results.cpu.simd_ops.score << "\n";
    std::cout << "    Memory Access: " << results.cpu.memory_access.score << "\n";
    std::cout << "    Overall Score: " << results.cpu.overall_score << "\n";
    
    // GPU Benchmarks
    std::cout << "\n  GPU Performance:\n";
    std::cout << "    Vertex Throughput: " << results.gpu.vertex_throughput.score 
              << " (tier: " << capabilities::to_string(results.gpu.tier) << ")\n";
    std::cout << "    Fragment Throughput: " << results.gpu.fragment_throughput.score << "\n";
    std::cout << "    Texture Bandwidth: " << results.gpu.texture_bandwidth.score << "\n";
    std::cout << "    Shader Complexity: " << results.gpu.shader_complexity.score << "\n";
    std::cout << "    Overall Score: " << results.gpu.overall_score << "\n";
    
    // Memory Benchmarks
    std::cout << "\n  Memory Performance:\n";
    std::cout << "    Sequential Read: " << results.memory.sequential_read.score 
              << " (tier: " << capabilities::to_string(results.memory.tier) << ")\n";
    std::cout << "    Sequential Write: " << results.memory.sequential_write.score << "\n";
    std::cout << "    Random Access: " << results.memory.random_access.score << "\n";
    std::cout << "    Memory Bandwidth: " << results.memory.memory_bandwidth.score << "\n";
    std::cout << "    Overall Score: " << results.memory.overall_score << "\n";
    
    // Overall Results
    std::cout << "\n  Overall System:\n";
    std::cout << "    Combined Score: " << results.overall_score << "\n";
    std::cout << "    Performance Tier: " << capabilities::to_string(results.overall_tier) << "\n";
    std::cout << "    Benchmark Duration: " << results.total_duration.count() << "ms\n";
}

/**
 * @brief Display capability support matrix
 */
void display_capability_matrix(const CapabilityDetectionResults& results) {
    std::cout << "\n🛠️  Capability Support Matrix:\n";
    
    struct CapabilityGroup {
        std::string name;
        std::vector<HALCapability> capabilities;
    };
    
    std::vector<CapabilityGroup> groups = {
        {"Threading", {HALCapability::Threading, HALCapability::AtomicOperations}},
        {"Memory", {HALCapability::DMA, HALCapability::VirtualMemory, HALCapability::MemoryProtection}},
        {"Graphics", {HALCapability::Hardware3D, HALCapability::Hardware2D, 
                     HALCapability::VertexShaders, HALCapability::FragmentShaders, HALCapability::ComputeShaders}},
        {"Audio", {HALCapability::HardwareAudio, HALCapability::MultiChannelAudio, HALCapability::AudioEffects}},
        {"Input", {HALCapability::MultiTouch, HALCapability::Accelerometer, HALCapability::Gamepad}},
        {"Network", {HALCapability::Networking, HALCapability::WiFi, HALCapability::Bluetooth}},
        {"Storage", {HALCapability::PersistentStorage, HALCapability::AsyncIO}},
        {"Timing", {HALCapability::HighPrecisionTimer, HALCapability::RealTimeClock}},
        {"Power", {HALCapability::PowerManagement}}
    };
    
    for (const auto& group : groups) {
        std::cout << "\n  " << group.name << ":\n";
        for (auto cap : group.capabilities) {
            bool supported = capabilities::has_capability(results.capability_mask, cap);
            bool has_fallback = results.fallback_availability.count(cap) && 
                               results.fallback_availability.at(cap);
            
            std::cout << "    " << capabilities::to_string(cap) << ": ";
            if (supported) {
                std::cout << "✅ Native";
            } else if (has_fallback) {
                std::cout << "🔄 Fallback";
            } else {
                std::cout << "❌ Not Available";
            }
            std::cout << "\n";
        }
    }
}

/**
 * @brief Demonstrate cache performance
 */
void demonstrate_cache_performance(ICapabilityDetectionSystem& detection_system) {
    std::cout << "\n💾 Cache Performance Demonstration:\n";
    
    // First detection (cache miss)
    auto start = std::chrono::high_resolution_clock::now();
    auto result1 = detection_system.detect_capabilities(true); // Force detection
    auto end = std::chrono::high_resolution_clock::now();
    auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "  First detection (cache miss): " << duration1.count() << "ms\n";
    
    // Second detection (cache hit)
    start = std::chrono::high_resolution_clock::now();
    auto result2 = detection_system.detect_capabilities(false); // Use cache
    end = std::chrono::high_resolution_clock::now();
    auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "  Second detection (cache hit): " << duration2.count() << "ms\n";
    std::cout << "  Cache speedup: " << std::fixed << std::setprecision(1) 
              << (static_cast<double>(duration1.count()) / duration2.count()) << "x\n";
}

/**
 * @brief Demonstrate adaptive performance optimization
 */
void demonstrate_adaptive_optimization(ICapabilityDetectionSystem& detection_system) {
    std::cout << "\n🎯 Adaptive Performance Optimization:\n";
    
    // Get current performance tier
    auto tier_result = detection_system.get_performance_tier_with_reasoning("overall");
    std::cout << "  Current performance tier: " << capabilities::to_string(tier_result.first) << "\n";
    std::cout << "  Reasoning: " << tier_result.second << "\n";
    
    // Get recommended settings for different use cases
    std::vector<std::string> use_cases = {"gaming", "productivity", "power_saving"};
    
    for (const auto& use_case : use_cases) {
        std::cout << "\n  Recommended settings for " << use_case << ":\n";
        auto settings = detection_system.get_recommended_settings(use_case);
        for (const auto& setting : settings) {
            std::cout << "    " << setting.first << ": " << setting.second << "\n";
        }
    }
}

/**
 * @brief Demonstrate capability prediction
 */
void demonstrate_capability_prediction(ICapabilityDetectionSystem& detection_system) {
    std::cout << "\n🔮 Capability Prediction:\n";
    
    // Test with hypothetical hardware configurations
    std::vector<std::unordered_map<std::string, std::string>> hardware_configs = {
        {{"cpu_vendor", "Intel"}, {"cpu_model", "i7-12700K"}, {"gpu_vendor", "NVIDIA"}, {"gpu_model", "RTX 4070"}},
        {{"cpu_vendor", "AMD"}, {"cpu_model", "Ryzen 7 5800X"}, {"gpu_vendor", "AMD"}, {"gpu_model", "RX 6700 XT"}},
        {{"cpu_vendor", "ARM"}, {"cpu_model", "Cortex-A78"}, {"gpu_vendor", "Mali"}, {"gpu_model", "G78"}},
        {{"platform", "Dreamcast"}, {"cpu", "SH-4"}, {"gpu", "PowerVR2"}},
        {{"platform", "PSP"}, {"cpu", "MIPS R4000"}, {"gpu", "Graphics Synthesizer"}}
    };
    
    for (size_t i = 0; i < hardware_configs.size(); ++i) {
        std::cout << "\n  Configuration " << (i + 1) << ":\n";
        for (const auto& spec : hardware_configs[i]) {
            std::cout << "    " << spec.first << ": " << spec.second << "\n";
        }
        
        auto prediction_result = detection_system.predict_capabilities(hardware_configs[i]);
        if (prediction_result.is_success()) {
            uint32_t predicted_caps = prediction_result.value();
            std::cout << "    Predicted capabilities: " << std::bitset<32>(predicted_caps).count() << "/25\n";
            
            // Determine predicted performance tier
            auto tier = PerformanceTier::Standard; // Would be calculated based on prediction
            std::cout << "    Predicted tier: " << capabilities::to_string(tier) << "\n";
        } else {
            std::cout << "    Prediction failed: " << prediction_result.error().message << "\n";
        }
    }
}

/**
 * @brief Main example function
 */
int main() {
    std::cout << "🚀 Flight HAL Capability Detection System - Comprehensive Example\n";
    std::cout << "================================================================\n";
    
    try {
        // Create capability detection system
        auto detection_system = create_capability_detection_system();
        
        // Create and register event listener
        auto event_listener = std::make_shared<ExampleCapabilityEventListener>();
        detection_system->register_event_listener(event_listener);
        
        // Configure detection system for comprehensive analysis
        auto config = create_capability_detection_config_for_use_case("development");
        config.enable_hardware_detection = true;
        config.enable_performance_benchmarks = true;
        config.enable_capability_caching = true;
        config.use_quick_detection_mode = false; // Full detection for demo
        
        std::cout << "\n⚙️  Initializing capability detection system...\n";
        auto init_result = detection_system->initialize(config);
        if (!init_result.is_success()) {
            std::cerr << "Failed to initialize detection system: " << init_result.error().message << "\n";
            return 1;
        }
        
        // Perform comprehensive capability detection
        std::cout << "\n🔍 Starting comprehensive capability detection...\n";
        auto detection_result = detection_system->detect_capabilities(true);
        
        if (detection_result.is_success()) {
            const auto& results = detection_result.value();
            
            // Display detailed results
            display_hardware_features(results);
            display_benchmark_results(results.benchmark_results);
            display_capability_matrix(results);
            
            // Demonstrate advanced features
            demonstrate_cache_performance(*detection_system);
            demonstrate_adaptive_optimization(*detection_system);
            demonstrate_capability_prediction(*detection_system);
            
            // Show detection statistics
            std::cout << "\n📈 Detection System Statistics:\n";
            auto stats = detection_system->get_statistics();
            for (const auto& stat : stats) {
                std::cout << "  " << stat.first << ": " << stat.second << "\n";
            }
            
        } else {
            std::cerr << "Capability detection failed: " << detection_result.error().message << "\n";
            return 1;
        }
        
        // Cleanup
        std::cout << "\n🧹 Shutting down capability detection system...\n";
        detection_system->shutdown();
        
        std::cout << "\n✨ Capability detection demonstration completed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Exception occurred: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
