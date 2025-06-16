/**
 * @file gpu_audio_coordination_example.cpp
 * @brief Example demonstrating GPU-accelerated audio processing coordination
 * 
 * This example shows how the resource coordination system enables safe
 * cross-driver resource sharing for complex operations like GPU-accelerated
 * audio processing.
 */

#include "../../include/flight/hal/coordination/resource_handle.hpp"
#include "../../include/flight/hal/coordination/cross_driver_messenger.hpp"
#include "../../include/flight/hal/coordination/deadlock_prevention.hpp"
#include "../../include/flight/hal/core/hal_logging.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <memory>

using namespace flight::hal;
using namespace flight::hal::coordination;

/**
 * @brief Custom payload for audio processing requests
 */
class AudioProcessingPayload : public IMessagePayload {
public:
    struct AudioBuffer {
        std::vector<float> samples;
        uint32_t sample_rate;
        uint32_t channels;
        
        AudioBuffer(size_t sample_count = 1024, uint32_t rate = 44100, uint32_t ch = 2)
            : samples(sample_count, 0.0f), sample_rate(rate), channels(ch) {
            // Fill with test data (sine wave)
            for (size_t i = 0; i < samples.size(); ++i) {
                samples[i] = std::sin(2.0f * 3.14159f * 440.0f * i / sample_rate);
            }
        }
    };
    
    enum class ProcessingType {
        Reverb,
        Echo,
        Distortion,
        Normalization
    };
    
    AudioBuffer input_buffer;
    AudioBuffer output_buffer;
    ProcessingType processing_type;
    float processing_strength;
    
    AudioProcessingPayload(ProcessingType type = ProcessingType::Reverb, float strength = 0.5f)
        : processing_type(type), processing_strength(strength) {}
    
    // IMessagePayload implementation
    std::string get_type() const override { return "AudioProcessing"; }
    
    std::vector<uint8_t> serialize() const override {
        std::vector<uint8_t> data;
        
        // Serialize processing type and strength
        data.push_back(static_cast<uint8_t>(processing_type));
        data.resize(data.size() + sizeof(float));
        std::memcpy(data.data() + data.size() - sizeof(float), &processing_strength, sizeof(float));
        
        // Serialize input buffer
        uint32_t input_size = static_cast<uint32_t>(input_buffer.samples.size());
        data.resize(data.size() + sizeof(uint32_t));
        std::memcpy(data.data() + data.size() - sizeof(uint32_t), &input_size, sizeof(uint32_t));
        
        size_t old_size = data.size();
        data.resize(old_size + input_size * sizeof(float));
        std::memcpy(data.data() + old_size, input_buffer.samples.data(), input_size * sizeof(float));
        
        return data;
    }
    
    HALResult<void> deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() < sizeof(uint8_t) + sizeof(float) + sizeof(uint32_t)) {
            return HALResult<void>::error(HALError(HALErrorCategory::Validation, 1, 
                "Invalid AudioProcessingPayload data size", nullptr));
        }
        
        size_t offset = 0;
        
        // Deserialize processing type and strength
        processing_type = static_cast<ProcessingType>(data[offset]);
        offset += sizeof(uint8_t);
        
        std::memcpy(&processing_strength, data.data() + offset, sizeof(float));
        offset += sizeof(float);
        
        // Deserialize input buffer
        uint32_t input_size;
        std::memcpy(&input_size, data.data() + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        
        if (offset + input_size * sizeof(float) > data.size()) {
            return HALResult<void>::error(HALError(HALErrorCategory::Validation, 2, 
                "Invalid audio buffer size", nullptr));
        }
        
        input_buffer.samples.resize(input_size);
        std::memcpy(input_buffer.samples.data(), data.data() + offset, input_size * sizeof(float));
        
        return HALResult<void>::success();
    }
    
    std::unique_ptr<IMessagePayload> clone() const override {
        auto clone = std::make_unique<AudioProcessingPayload>();
        clone->input_buffer = input_buffer;
        clone->output_buffer = output_buffer;
        clone->processing_type = processing_type;
        clone->processing_strength = processing_strength;
        return std::move(clone);
    }
};

/**
 * @brief Mock GPU driver for audio processing
 */
class MockGPUDriver : public IMessageHandler {
public:
    MockGPUDriver() : driver_id_("gpu_driver") {
        // Register GPU compute resource
        ResourceMetadata gpu_metadata;
        gpu_metadata.type = ResourceType::Hardware;
        gpu_metadata.priority = ResourcePriority::High;
        gpu_metadata.flags = ResourceFlags::Exclusive | ResourceFlags::GPUAccessible;
        gpu_metadata.description = "GPU Compute Units";
        
        auto registry_result = ResourceRegistry::instance().register_resource("gpu_compute", gpu_metadata);
        if (registry_result) {
            gpu_compute_resource_ = registry_result.value();
            std::cout << "[GPU] Registered GPU compute resource\n";
        }
        
        // Register texture memory resource
        ResourceMetadata texture_metadata;
        texture_metadata.type = ResourceType::Memory;
        texture_metadata.priority = ResourcePriority::Normal;
        texture_metadata.flags = ResourceFlags::Shareable | ResourceFlags::GPUAccessible | ResourceFlags::MemoryMapped;
        texture_metadata.size_bytes = 16 * 1024 * 1024; // 16MB texture memory
        texture_metadata.description = "GPU Texture Memory";
        
        auto texture_result = ResourceRegistry::instance().register_resource("gpu_texture_memory", texture_metadata);
        if (texture_result) {
            texture_memory_resource_ = texture_result.value();
            std::cout << "[GPU] Registered texture memory resource\n";
        }
    }
    
    // IMessageHandler implementation
    HALResult<std::unique_ptr<Message>> handle_message(const Message& message) override {
        std::cout << "[GPU] Received message from: " << message.header().sender_id << "\n";
        
        auto audio_payload = message.payload_as<AudioProcessingPayload>();
        if (!audio_payload) {
            return HALResult<std::unique_ptr<Message>>::error(
                HALError(HALErrorCategory::Validation, 1, "Invalid payload type", nullptr));
        }
        
        // Simulate GPU processing with resource coordination
        return process_audio_on_gpu(*audio_payload, message.header().sender_id);
    }
    
    std::string get_handler_id() const override { return driver_id_; }
    
    bool can_handle(MessageType type) const override {
        return type == MessageType::Request || type == MessageType::Notification;
    }

private:
    HALResult<std::unique_ptr<Message>> process_audio_on_gpu(const AudioProcessingPayload& request,
                                                            const std::string& requester_id) {
        std::cout << "[GPU] Starting audio processing for " << requester_id << "\n";
        
        // Step 1: Acquire GPU compute resource
        HAL_LOCK_RESOURCE_WITH_PRIORITY(driver_id_, gpu_compute_resource_, ResourcePriority::High);
        std::cout << "[GPU] Acquired GPU compute resource\n";
        
        // Step 2: Acquire texture memory for processing
        HAL_LOCK_RESOURCE(driver_id_, texture_memory_resource_);
        std::cout << "[GPU] Acquired texture memory resource\n";
        
        // Step 3: Simulate GPU processing
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate processing time
        
        // Step 4: Create processed audio data
        auto response_payload = std::make_unique<AudioProcessingPayload>();
        response_payload->input_buffer = request.input_buffer;
        response_payload->output_buffer = request.input_buffer; // Copy input to output
        response_payload->processing_type = request.processing_type;
        response_payload->processing_strength = request.processing_strength;
        
        // Apply simple processing (just amplify for demo)
        for (auto& sample : response_payload->output_buffer.samples) {
            sample *= (1.0f + response_payload->processing_strength);
            // Clamp to prevent clipping
            sample = std::max(-1.0f, std::min(1.0f, sample));
        }
        
        std::cout << "[GPU] Audio processing completed\n";
        
        // Step 5: Create response message
        MessageHeader response_header;
        response_header.type = MessageType::Response;
        response_header.sender_id = driver_id_;
        response_header.recipient_id = requester_id;
        
        auto response_message = std::make_unique<Message>(response_header, std::move(response_payload));
        
        // Resources will be automatically released when locks go out of scope
        return HALResult<std::unique_ptr<Message>>::success(std::move(response_message));
    }
    
    std::string driver_id_;
    ResourceHandle gpu_compute_resource_;
    ResourceHandle texture_memory_resource_;
};

/**
 * @brief Mock Audio driver that coordinates with GPU
 */
class MockAudioDriver : public IMessageHandler {
public:
    MockAudioDriver() : driver_id_("audio_driver") {
        // Register audio buffer resource
        ResourceMetadata audio_metadata;
        audio_metadata.type = ResourceType::Memory;
        audio_metadata.priority = ResourcePriority::High;
        audio_metadata.flags = ResourceFlags::Exclusive | ResourceFlags::DMACapable;
        audio_metadata.size_bytes = 4 * 1024; // 4KB audio buffer
        audio_metadata.description = "Audio Output Buffer";
        
        auto registry_result = ResourceRegistry::instance().register_resource("audio_buffer", audio_metadata);
        if (registry_result) {
            audio_buffer_resource_ = registry_result.value();
            std::cout << "[AUDIO] Registered audio buffer resource\n";
        }
        
        // Register audio hardware resource
        ResourceMetadata hw_metadata;
        hw_metadata.type = ResourceType::Hardware;
        hw_metadata.priority = ResourcePriority::Critical;
        hw_metadata.flags = ResourceFlags::Exclusive | ResourceFlags::DMACapable;
        hw_metadata.description = "Audio Hardware Interface";
        
        auto hw_result = ResourceRegistry::instance().register_resource("audio_hardware", hw_metadata);
        if (hw_result) {
            audio_hardware_resource_ = hw_result.value();
            std::cout << "[AUDIO] Registered audio hardware resource\n";
        }
    }
    
    // IMessageHandler implementation
    HALResult<std::unique_ptr<Message>> handle_message(const Message& message) override {
        std::cout << "[AUDIO] Received response from: " << message.header().sender_id << "\n";
        
        auto audio_payload = message.payload_as<AudioProcessingPayload>();
        if (!audio_payload) {
            return HALResult<std::unique_ptr<Message>>::error(
                HALError(HALErrorCategory::Validation, 1, "Invalid payload type", nullptr));
        }
        
        // Process the GPU-processed audio (but don't return from here in message handler)
        finalize_audio_output(*audio_payload);
        
        // Return nullptr since we don't need to send a response back
        return HALResult<std::unique_ptr<Message>>::success(nullptr);
    }
    
    std::string get_handler_id() const override { return driver_id_; }
    
    bool can_handle(MessageType type) const override {
        return type == MessageType::Response || type == MessageType::Notification;
    }
    
    /**
     * @brief Request GPU processing for audio data
     */
    HALResult<void> request_gpu_processing() {
        std::cout << "[AUDIO] Requesting GPU processing\n";
        
        // Create audio processing request
        auto audio_payload = std::make_unique<AudioProcessingPayload>(
            AudioProcessingPayload::ProcessingType::Reverb, 0.7f);
        
        MessageHeader request_header;
        request_header.type = MessageType::Request;
        request_header.sender_id = driver_id_;
        request_header.recipient_id = "gpu_driver";
        request_header.priority = MessagePriority::High;
        
        Message request_message(request_header, std::move(audio_payload));
        
        // Send request through messenger
        auto& messenger = CrossDriverMessenger::instance();
        auto response_result = messenger.send_request(std::move(request_message), 
                                                     std::chrono::milliseconds(2000));
        
        if (!response_result) {
            std::cout << "[AUDIO] GPU processing request failed: " << response_result.error().message() << "\n";
            return HALResult<void>::error(response_result.error());
        }
        
        std::cout << "[AUDIO] GPU processing request completed successfully\n";
        
        // Process the response
        auto response_payload = response_result.value().payload_as<AudioProcessingPayload>();
        if (response_payload) {
            auto finalize_result = finalize_audio_output(*response_payload);
            if (!finalize_result) {
                return HALResult<void>::error(HALError(HALErrorCategory::Internal, 1, 
                    "Failed to finalize audio output", nullptr));
            }
        }
        
        return HALResult<void>::success();
    }

private:
    HALResult<void> finalize_audio_output(const AudioProcessingPayload& processed_audio) {
        std::cout << "[AUDIO] Finalizing audio output\n";
        
        // Acquire audio resources
        HAL_LOCK_RESOURCE_WITH_PRIORITY(driver_id_, audio_buffer_resource_, ResourcePriority::Critical);
        std::cout << "[AUDIO] Acquired audio buffer resource\n";
        
        HAL_LOCK_RESOURCE_WITH_PRIORITY(driver_id_, audio_hardware_resource_, ResourcePriority::Critical);
        std::cout << "[AUDIO] Acquired audio hardware resource\n";
        
        // Simulate writing processed audio to hardware
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        std::cout << "[AUDIO] Audio output completed - " << processed_audio.output_buffer.samples.size() 
                  << " samples processed\n";
        
        // Resources will be automatically released
        return HALResult<void>::success();
    }
    
    std::string driver_id_;
    ResourceHandle audio_buffer_resource_;
    ResourceHandle audio_hardware_resource_;
};

/**
 * @brief Mock Graphics driver that also uses GPU resources
 */
class MockGraphicsDriver {
public:
    MockGraphicsDriver() : driver_id_("graphics_driver") {
        // Register graphics memory resource
        ResourceMetadata gfx_metadata;
        gfx_metadata.type = ResourceType::Memory;
        gfx_metadata.priority = ResourcePriority::Normal;
        gfx_metadata.flags = ResourceFlags::Shareable | ResourceFlags::GPUAccessible;
        gfx_metadata.size_bytes = 64 * 1024 * 1024; // 64MB graphics memory
        gfx_metadata.description = "Graphics Frame Buffer";
        
        auto registry_result = ResourceRegistry::instance().register_resource("graphics_memory", gfx_metadata);
        if (registry_result) {
            graphics_memory_resource_ = registry_result.value();
            std::cout << "[GFX] Registered graphics memory resource\n";
        }
    }
    
    /**
     * @brief Simulate graphics rendering that might conflict with audio processing
     */
    void perform_rendering() {
        std::cout << "[GFX] Starting graphics rendering\n";
        
        try {
            // This will compete with audio/GPU coordination
            HAL_LOCK_RESOURCE_WITH_TIMEOUT(driver_id_, graphics_memory_resource_, 
                                          std::chrono::milliseconds(1000));
            std::cout << "[GFX] Acquired graphics memory resource\n";
            
            // Try to also get GPU compute (this might cause coordination issues)
            auto gpu_compute_result = ResourceRegistry::instance().find_resource("gpu_compute");
            if (gpu_compute_result) {
                ResourceLock gpu_lock(driver_id_, gpu_compute_result.value(), 
                                    ResourcePriority::Normal, std::chrono::milliseconds(500));
                
                if (gpu_lock.is_locked()) {
                    std::cout << "[GFX] Also acquired GPU compute resource\n";
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    std::cout << "[GFX] Graphics rendering completed\n";
                } else {
                    std::cout << "[GFX] Could not acquire GPU compute - audio has priority\n";
                }
            }
            
        } catch (const std::exception& e) {
            std::cout << "[GFX] Graphics rendering failed: " << e.what() << "\n";
        }
    }
    
private:
    std::string driver_id_;
    ResourceHandle graphics_memory_resource_;
};

/**
 * @brief Demonstrate deadlock detection and resolution
 */
void demonstrate_deadlock_prevention() {
    std::cout << "\n=== Deadlock Prevention Demonstration ===\n";
    
    auto& deadlock_prevention = DeadlockPrevention::instance();
    
    // Create some resources for the test
    ResourceMetadata resource1_meta;
    resource1_meta.type = ResourceType::Hardware;
    resource1_meta.priority = ResourcePriority::Normal;
    resource1_meta.flags = ResourceFlags::Exclusive;
    
    ResourceMetadata resource2_meta;
    resource2_meta.type = ResourceType::Memory;
    resource2_meta.priority = ResourcePriority::Normal;
    resource2_meta.flags = ResourceFlags::Exclusive;
    
    auto resource1_result = ResourceRegistry::instance().register_resource("test_resource1", resource1_meta);
    auto resource2_result = ResourceRegistry::instance().register_resource("test_resource2", resource2_meta);
    
    if (!resource1_result || !resource2_result) {
        std::cout << "Failed to register test resources\n";
        return;
    }
    
    auto resource1 = resource1_result.value();
    auto resource2 = resource2_result.value();
    
    std::cout << "Testing resource ordering constraints...\n";
    
    // Test proper ordering (Memory before Hardware)
    {
        ResourceLock lock1("test_driver1", resource2); // Memory first
        if (lock1.is_locked()) {
            std::cout << "✓ Acquired memory resource first\n";
            
            ResourceLock lock2("test_driver1", resource1); // Hardware second
            if (lock2.is_locked()) {
                std::cout << "✓ Acquired hardware resource second - proper ordering\n";
            } else {
                std::cout << "✗ Failed to acquire hardware resource\n";
            }
        }
    }
    
    // Test improper ordering (Hardware before Memory) - should fail
    {
        ResourceLock lock1("test_driver2", resource1); // Hardware first
        if (lock1.is_locked()) {
            std::cout << "✓ Acquired hardware resource first\n";
            
            ResourceLock lock2("test_driver2", resource2); // Memory second - should fail
            if (lock2.is_locked()) {
                std::cout << "✗ Acquired memory resource second - BAD ordering!\n";
            } else {
                std::cout << "✓ Failed to acquire memory resource - ordering constraint enforced\n";
            }
        }
    }
    
    // Test deadlock detection
    std::cout << "\nTesting deadlock detection...\n";
    auto deadlock_info = deadlock_prevention.detect_deadlock();
    if (deadlock_info) {
        if (deadlock_info.value().deadlock_detected) {
            std::cout << "⚠ Deadlock detected: " << deadlock_info.value().description << "\n";
            
            auto resolution_result = deadlock_prevention.resolve_deadlock(deadlock_info.value());
            if (resolution_result) {
                std::cout << "✓ Deadlock resolved successfully\n";
            } else {
                std::cout << "✗ Failed to resolve deadlock\n";
            }
        } else {
            std::cout << "✓ No deadlocks detected\n";
        }
    }
    
    // Print statistics
    auto stats = deadlock_prevention.get_stats();
    std::cout << "\nDeadlock Prevention Statistics:\n";
    std::cout << "  Requests processed: " << stats.requests_processed << "\n";
    std::cout << "  Requests denied: " << stats.requests_denied << "\n";
    std::cout << "  Deadlocks detected: " << stats.deadlocks_detected << "\n";
    std::cout << "  Deadlocks resolved: " << stats.deadlocks_resolved << "\n";
}

int main() {
    std::cout << "=== Flight HAL Resource Coordination Example ===\n\n";
    
    // Initialize coordination systems
    auto& messenger = CrossDriverMessenger::instance();
    auto& deadlock_prevention = DeadlockPrevention::instance();
    
    auto messenger_init = messenger.initialize();
    auto deadlock_init = deadlock_prevention.initialize();
    
    if (!messenger_init || !deadlock_init) {
        std::cout << "Failed to initialize coordination systems\n";
        return 1;
    }
    
    std::cout << "✓ Resource coordination systems initialized\n\n";
    
    // Create drivers
    auto gpu_driver = std::make_shared<MockGPUDriver>();
    auto audio_driver = std::make_shared<MockAudioDriver>();
    auto graphics_driver = std::make_unique<MockGraphicsDriver>();
    
    // Register message handlers
    messenger.register_handler("gpu_driver", gpu_driver);
    messenger.register_handler("audio_driver", audio_driver);
    
    std::cout << "✓ Drivers created and registered\n\n";
    
    // Demonstrate GPU-accelerated audio processing
    std::cout << "=== GPU-Accelerated Audio Processing ===\n";
    
    // Start graphics rendering in background (potential resource conflict)
    std::thread graphics_thread([&graphics_driver]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        graphics_driver->perform_rendering();
    });
    
    // Request GPU processing for audio
    auto audio_result = audio_driver->request_gpu_processing();
    if (audio_result) {
        std::cout << "✓ GPU-accelerated audio processing completed successfully\n";
    } else {
        std::cout << "✗ GPU-accelerated audio processing failed: " << audio_result.error().message() << "\n";
    }
    
    graphics_thread.join();
    
    // Demonstrate cross-driver messaging
    std::cout << "\n=== Cross-Driver Messaging Statistics ===\n";
    auto msg_stats = messenger.get_stats();
    std::cout << "Messages sent: " << msg_stats.messages_sent << "\n";
    std::cout << "Messages received: " << msg_stats.messages_received << "\n";
    std::cout << "Messages dropped: " << msg_stats.messages_dropped << "\n";
    std::cout << "Requests sent: " << msg_stats.requests_sent << "\n";
    std::cout << "Request timeouts: " << msg_stats.requests_timeout << "\n";
    
    // Demonstrate deadlock prevention
    demonstrate_deadlock_prevention();
    
    // Cleanup
    std::cout << "\n=== Cleanup ===\n";
    messenger.shutdown();
    deadlock_prevention.shutdown();
    
    std::cout << "✓ Resource coordination systems shut down\n";
    std::cout << "\n=== Example completed successfully ===\n";
    
    return 0;
}
