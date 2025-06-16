/**
 * @file gpu_audio_integration_test.cpp
 * @brief Integration test for GPU-accelerated audio processing
 * 
 * Tests the coordination between graphics and audio drivers for
 * GPU-accelerated audio processing scenarios.
 */

#include "../framework/integration_test_base.hpp"
#include "../../../examples/drivers/mock_graphics_driver.hpp"
#include "../../../examples/drivers/mock_audio_driver.hpp"
#include "../../../include/flight/hal/coordination/cross_driver_messenger.hpp"

#include <memory>
#include <thread>
#include <chrono>

namespace flight::hal::integration {

/**
 * @brief Simple message payload for GPU-audio coordination
 */
class AudioProcessingPayload : public coordination::IMessagePayload {
public:
    struct AudioData {
        std::vector<float> samples;
        uint32_t sample_rate;
        uint32_t channels;
        
        AudioData(size_t sample_count = 1024, uint32_t rate = 44100, uint32_t ch = 2)
            : samples(sample_count, 0.5f), sample_rate(rate), channels(ch) {}
    };
    
    AudioData input_data;
    AudioData output_data;
    bool processing_complete{false};
    
    // IMessagePayload implementation
    std::string get_type() const override { return "AudioProcessing"; }
    
    std::vector<uint8_t> serialize() const override {
        std::vector<uint8_t> data;
        // Simple serialization for demo
        data.resize(sizeof(uint32_t) * 3); // sample_rate, channels, sample_count
        uint32_t* ptr = reinterpret_cast<uint32_t*>(data.data());
        ptr[0] = input_data.sample_rate;
        ptr[1] = input_data.channels;
        ptr[2] = static_cast<uint32_t>(input_data.samples.size());
        return data;
    }
    
    HALResult<void> deserialize(const std::vector<uint8_t>& data) override {
        if (data.size() < sizeof(uint32_t) * 3) {
            return HALResult<void>::error(errors::validation_failed(1, "Invalid data size"));
        }
        const uint32_t* ptr = reinterpret_cast<const uint32_t*>(data.data());
        input_data.sample_rate = ptr[0];
        input_data.channels = ptr[1];
        input_data.samples.resize(ptr[2], 0.5f);
        return HALResult<void>::success();
    }
    
    std::unique_ptr<coordination::IMessagePayload> clone() const override {
        auto clone = std::make_unique<AudioProcessingPayload>();
        clone->input_data = input_data;
        clone->output_data = output_data;
        clone->processing_complete = processing_complete;
        return std::move(clone);
    }
};

/**
 * @brief Mock GPU driver for integration testing
 */
class MockGPUDriver : public coordination::IMessageHandler {
public:
    MockGPUDriver() : driver_id_("gpu_driver") {}
    
    HALResult<std::unique_ptr<coordination::Message>> handle_message(const coordination::Message& message) override {
        auto payload = message.payload_as<AudioProcessingPayload>();
        if (!payload) {
            return HALResult<std::unique_ptr<coordination::Message>>::error(
                errors::validation_failed(1, "Invalid payload type"));
        }
        
        // Simulate GPU processing
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        auto response_payload = std::make_unique<AudioProcessingPayload>();
        response_payload->input_data = payload->input_data;
        response_payload->output_data = payload->input_data; // Copy input to output for demo
        response_payload->processing_complete = true;
        
        // Apply simple processing (amplify)
        for (auto& sample : response_payload->output_data.samples) {
            sample *= 1.5f;
            sample = std::max(-1.0f, std::min(1.0f, sample)); // Clamp
        }
        
        coordination::MessageHeader response_header;
        response_header.type = coordination::MessageType::Response;
        response_header.sender_id = driver_id_;
        response_header.recipient_id = message.header().sender_id;
        
        auto response_message = std::make_unique<coordination::Message>(response_header, std::move(response_payload));
        return HALResult<std::unique_ptr<coordination::Message>>::success(std::move(response_message));
    }
    
    std::string get_handler_id() const override { return driver_id_; }
    
    bool can_handle(coordination::MessageType type) const override {
        return type == coordination::MessageType::Request;
    }

private:
    std::string driver_id_;
};

/**
 * @brief Mock Audio driver for integration testing
 */
class MockAudioDriver : public coordination::IMessageHandler {
public:
    MockAudioDriver() : driver_id_("audio_driver") {}
    
    HALResult<std::unique_ptr<coordination::Message>> handle_message(const coordination::Message& message) override {
        auto payload = message.payload_as<AudioProcessingPayload>();
        if (!payload) {
            return HALResult<std::unique_ptr<coordination::Message>>::error(
                errors::validation_failed(1, "Invalid payload type"));
        }
        
        // Process the GPU-processed audio
        last_processed_samples_ = payload->output_data.samples.size();
        processing_complete_ = payload->processing_complete;
        
        return HALResult<std::unique_ptr<coordination::Message>>::success(nullptr);
    }
    
    std::string get_handler_id() const override { return driver_id_; }
    
    bool can_handle(coordination::MessageType type) const override {
        return type == coordination::MessageType::Response;
    }
    
    size_t get_last_processed_samples() const { return last_processed_samples_; }
    bool is_processing_complete() const { return processing_complete_; }

private:
    std::string driver_id_;
    size_t last_processed_samples_{0};
    bool processing_complete_{false};
};

/**
 * @brief GPU-Audio coordination integration test
 */
class GPUAudioIntegrationTest : public IntegrationTestBase {
public:
    GPUAudioIntegrationTest() : IntegrationTestBase("GPUAudioIntegration") {}

    TestScenario get_scenario() const override {
        TestScenario scenario;
        scenario.name = "GPU-Audio Coordination";
        scenario.description = "Tests GPU-accelerated audio processing coordination";
        scenario.required_drivers = {"gpu_driver", "audio_driver"};
        scenario.required_resources = {}; // Will register resources in setup
        scenario.timeout = std::chrono::milliseconds(5000);
        scenario.requires_real_hardware = false;
        return scenario;
    }

protected:
    HALResult<void> setup_test() override {
        log_message("Setting up GPU-Audio integration test");
        
        // Create mock drivers
        gpu_driver_ = std::make_shared<MockGPUDriver>();
        audio_driver_ = std::make_shared<MockAudioDriver>();
        
        // Register drivers with coordinator
        auto result = get_coordinator().register_driver("gpu_driver", gpu_driver_);
        if (!result) {
            return result;
        }
        
        result = get_coordinator().register_driver("audio_driver", audio_driver_);
        if (!result) {
            return result;
        }
        
        log_message("Drivers registered successfully");
        return HALResult<void>::success();
    }
    
    HALResult<void> execute_test() override {
        log_message("Executing GPU-Audio coordination test");
        
        // Create audio processing payload
        auto payload = std::make_unique<AudioProcessingPayload>();
        payload->input_data = AudioProcessingPayload::AudioData(2048, 44100, 2);
        
        record_metric("input_samples", static_cast<double>(payload->input_data.samples.size()));
        
        // Send message from audio driver to GPU driver
        auto send_result = get_coordinator().send_message(
            "audio_driver", "gpu_driver", std::move(payload));
        
        if (!send_result) {
            return send_result;
        }
        
        log_message("Audio processing request sent to GPU");
        
        // Wait for processing to complete
        bool processing_completed = wait_for_condition(
            [this]() { return audio_driver_->is_processing_complete(); },
            std::chrono::milliseconds(3000)
        );
        
        assert_condition(processing_completed, "GPU audio processing should complete");
        assert_condition(audio_driver_->get_last_processed_samples() > 0, 
                        "Audio driver should have received processed samples");
        
        record_metric("output_samples", static_cast<double>(audio_driver_->get_last_processed_samples()));
        
        log_message("GPU-Audio coordination test completed successfully");
        return HALResult<void>::success();
    }
    
    HALResult<void> teardown_test() override {
        log_message("Tearing down GPU-Audio integration test");
        
        // Unregister drivers
        get_coordinator().unregister_driver("gpu_driver");
        get_coordinator().unregister_driver("audio_driver");
        
        // Clear driver references
        gpu_driver_.reset();
        audio_driver_.reset();
        
        return HALResult<void>::success();
    }

private:
    std::shared_ptr<MockGPUDriver> gpu_driver_;
    std::shared_ptr<MockAudioDriver> audio_driver_;
};

// Register the test
REGISTER_INTEGRATION_TEST(GPUAudioIntegrationTest, "gpu_audio_coordination");

/**
 * @brief Cross-driver resource sharing test
 */
class CrossDriverResourceTest : public IntegrationTestBase {
public:
    CrossDriverResourceTest() : IntegrationTestBase("CrossDriverResource") {}

    TestScenario get_scenario() const override {
        TestScenario scenario;
        scenario.name = "Cross-Driver Resource Sharing";
        scenario.description = "Tests resource sharing between multiple drivers";
        scenario.required_drivers = {"driver_a", "driver_b"};
        scenario.required_resources = {"shared_memory", "exclusive_hardware"};
        scenario.timeout = std::chrono::milliseconds(3000);
        scenario.requires_real_hardware = false;
        return scenario;
    }

protected:
    HALResult<void> setup_test() override {
        log_message("Setting up cross-driver resource test");
        
        // Register test resources
        coordination::ResourceMetadata shared_memory_meta;
        shared_memory_meta.type = coordination::ResourceType::Memory;
        shared_memory_meta.priority = coordination::ResourcePriority::Normal;
        shared_memory_meta.flags = coordination::ResourceFlags::Shareable;
        shared_memory_meta.size_bytes = 1024 * 1024; // 1MB
        shared_memory_meta.description = "Shared memory for testing";
        
        auto& registry = coordination::ResourceRegistry::instance();
        auto shared_result = registry.register_resource("shared_memory", shared_memory_meta);
        if (!shared_result) {
            return HALResult<void>::error(shared_result.error());
        }
        shared_memory_handle_ = shared_result.value();
        
        coordination::ResourceMetadata exclusive_hw_meta;
        exclusive_hw_meta.type = coordination::ResourceType::Hardware;
        exclusive_hw_meta.priority = coordination::ResourcePriority::High;
        exclusive_hw_meta.flags = coordination::ResourceFlags::Exclusive;
        exclusive_hw_meta.description = "Exclusive hardware for testing";
        
        auto exclusive_result = registry.register_resource("exclusive_hardware", exclusive_hw_meta);
        if (!exclusive_result) {
            return HALResult<void>::error(exclusive_result.error());
        }
        exclusive_hw_handle_ = exclusive_result.value();
        
        log_message("Test resources registered");
        return HALResult<void>::success();
    }
    
    HALResult<void> execute_test() override {
        log_message("Executing cross-driver resource sharing test");
        
        // Test shared resource access
        assert_condition(shared_memory_handle_.is_valid(), "Shared memory handle should be valid");
        assert_condition(exclusive_hw_handle_.is_valid(), "Exclusive hardware handle should be valid");
        
        // Verify resource metadata
        auto& registry = coordination::ResourceRegistry::instance();
        auto metadata_result = registry.get_metadata(shared_memory_handle_);
        assert_condition(metadata_result.is_success(), "Should be able to get shared memory metadata");
        
        auto metadata = metadata_result.value();
        assert_condition(metadata.type == coordination::ResourceType::Memory, 
                        "Resource type should be Memory");
        assert_condition(metadata.flags & coordination::ResourceFlags::Shareable, 
                        "Resource should be shareable");
        
        record_metric("shared_resource_size", static_cast<double>(metadata.size_bytes));
        
        log_message("Cross-driver resource test completed successfully");
        return HALResult<void>::success();
    }
    
    HALResult<void> teardown_test() override {
        log_message("Tearing down cross-driver resource test");
        
        // Unregister test resources
        auto& registry = coordination::ResourceRegistry::instance();
        registry.unregister_resource(shared_memory_handle_);
        registry.unregister_resource(exclusive_hw_handle_);
        
        return HALResult<void>::success();
    }

private:
    coordination::ResourceHandle shared_memory_handle_;
    coordination::ResourceHandle exclusive_hw_handle_;
};

// Register the test
REGISTER_INTEGRATION_TEST(CrossDriverResourceTest, "cross_driver_resource");

} // namespace flight::hal::integration
