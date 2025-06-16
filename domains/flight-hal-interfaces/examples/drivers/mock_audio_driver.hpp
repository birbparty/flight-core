/**
 * @file mock_audio_driver.hpp
 * @brief Mock Audio Driver for Testing and Examples
 * 
 * Provides a complete mock implementation of the audio interface for testing
 * and demonstration purposes. Simulates various platform capabilities.
 */

#pragma once

#include "../../include/flight/hal/interfaces/audio.hpp"
#include "../../include/flight/hal/core/driver_registry.hpp"
#include <vector>
#include <memory>
#include <unordered_map>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace flight::hal {

/**
 * @brief Mock audio buffer implementation
 */
class MockAudioBuffer : public IAudioBuffer {
public:
    explicit MockAudioBuffer(const AudioBufferDescriptor& desc)
        : descriptor_(desc), valid_(true) {
        size_t buffer_size = audio::calculate_buffer_size(desc.frame_count, desc.channels, desc.format);
        data_.resize(buffer_size);
    }
    
    const AudioBufferDescriptor& get_descriptor() const override { return descriptor_; }
    AudioResourceHandle get_handle() const override { return handle_; }
    size_t get_size_bytes() const override { return data_.size(); }
    size_t get_frame_count() const override { return descriptor_.frame_count; }
    
    HALResult<void> upload_data(const void* data, size_t frame_count, size_t offset_frames = 0) override {
        if (!valid_) {
            return HALResult<void>::error(HALError(HALErrorCategory::Resource, 1, "Buffer is invalid", nullptr));
        }
        
        size_t bytes_per_frame = descriptor_.channels * audio::get_bytes_per_sample(descriptor_.format);
        size_t offset_bytes = offset_frames * bytes_per_frame;
        size_t copy_bytes = frame_count * bytes_per_frame;
        
        if (offset_bytes + copy_bytes > data_.size()) {
            return HALResult<void>::error(HALError(HALErrorCategory::Validation, 1, "Buffer overflow", nullptr));
        }
        
        std::memcpy(data_.data() + offset_bytes, data, copy_bytes);
        return HALResult<void>::success();
    }
    
    HALResult<void> download_data(void* data, size_t frame_count, size_t offset_frames = 0) const override {
        if (!valid_) {
            return HALResult<void>::error(HALError(HALErrorCategory::Resource, 1, "Buffer is invalid", nullptr));
        }
        
        size_t bytes_per_frame = descriptor_.channels * audio::get_bytes_per_sample(descriptor_.format);
        size_t offset_bytes = offset_frames * bytes_per_frame;
        size_t copy_bytes = frame_count * bytes_per_frame;
        
        if (offset_bytes + copy_bytes > data_.size()) {
            return HALResult<void>::error(HALError(HALErrorCategory::Validation, 1, "Buffer underflow", nullptr));
        }
        
        std::memcpy(data, data_.data() + offset_bytes, copy_bytes);
        return HALResult<void>::success();
    }
    
    HALResult<void> clear() override {
        std::fill(data_.begin(), data_.end(), 0);
        return HALResult<void>::success();
    }
    
    bool is_valid() const override { return valid_; }

private:
    AudioBufferDescriptor descriptor_;
    AudioResourceHandle handle_{1, AudioResourceType::AudioBuffer, 1};
    std::vector<uint8_t> data_;
    bool valid_;
};

/**
 * @brief Mock audio stream implementation
 */
class MockAudioStream : public IAudioStream {
public:
    explicit MockAudioStream(const AudioStreamDescriptor& desc)
        : descriptor_(desc), playing_(false), finished_(false), position_(0.0), duration_(1.0) {}
    
    const AudioStreamDescriptor& get_descriptor() const override { return descriptor_; }
    AudioResourceHandle get_handle() const override { return handle_; }
    
    HALResult<void> load_from_file(std::string_view file_path) override {
        // Mock file loading
        duration_ = 5.0; // 5 second mock file
        return HALResult<void>::success();
    }
    
    HALResult<void> load_from_memory(const void* data, size_t size) override {
        // Mock memory loading
        duration_ = 1.0; // 1 second mock audio
        return HALResult<void>::success();
    }
    
    HALResult<void> play() override {
        playing_ = true;
        finished_ = false;
        position_ = 0.0;
        return HALResult<void>::success();
    }
    
    HALResult<void> pause() override {
        playing_ = false;
        return HALResult<void>::success();
    }
    
    HALResult<void> stop() override {
        playing_ = false;
        finished_ = false;
        position_ = 0.0;
        return HALResult<void>::success();
    }
    
    HALResult<void> set_volume(float volume) override {
        volume_ = std::clamp(volume, 0.0f, 1.0f);
        return HALResult<void>::success();
    }
    
    HALResult<void> set_position(double position_seconds) override {
        position_ = std::clamp(position_seconds, 0.0, duration_);
        return HALResult<void>::success();
    }
    
    HALResult<void> set_position_3d(const Vector3& position) override {
        position_3d_ = position;
        return HALResult<void>::success();
    }
    
    HALResult<void> set_loop_enabled(bool loop_enabled) override {
        loop_enabled_ = loop_enabled;
        return HALResult<void>::success();
    }
    
    HALResult<double> get_position() const override {
        return HALResult<double>::success(position_);
    }
    
    HALResult<double> get_duration() const override {
        return HALResult<double>::success(duration_);
    }
    
    bool is_playing() const override { return playing_; }
    bool is_finished() const override { return finished_; }
    
    HALResult<void> set_completion_callback(StreamCompletionCallback callback) override {
        completion_callback_ = callback;
        return HALResult<void>::success();
    }

private:
    AudioStreamDescriptor descriptor_;
    AudioResourceHandle handle_{2, AudioResourceType::AudioStream, 1};
    bool playing_;
    bool finished_;
    bool loop_enabled_ = false;
    double position_;
    double duration_;
    float volume_ = 1.0f;
    Vector3 position_3d_;
    StreamCompletionCallback completion_callback_;
};

/**
 * @brief Mock audio processor implementation
 */
class MockAudioProcessor : public IAudioProcessor {
public:
    MockAudioProcessor(uint32_t sample_rate, uint16_t channels, uint32_t buffer_size)
        : sample_rate_(sample_rate), channels_(channels), buffer_size_(buffer_size),
          processing_(false) {}
    
    AudioResourceHandle get_handle() const override { return handle_; }
    
    HALResult<void> set_callback(AudioCallback callback) override {
        callback_ = callback;
        return HALResult<void>::success();
    }
    
    HALResult<void> start_processing() override {
        if (processing_) {
            return HALResult<void>::error(errors::invalid_state(1, "Already processing"));
        }
        
        processing_ = true;
        // In a real implementation, this would start a real-time audio thread
        return HALResult<void>::success();
    }
    
    HALResult<void> stop_processing() override {
        processing_ = false;
        return HALResult<void>::success();
    }
    
    HALResult<void> set_buffer_size(uint32_t frames) override {
        if (processing_) {
            return HALResult<void>::error(errors::invalid_state(1, "Cannot change buffer size while processing"));
        }
        buffer_size_ = frames;
        return HALResult<void>::success();
    }
    
    HALResult<void> set_sample_rate(uint32_t sample_rate) override {
        if (processing_) {
            return HALResult<void>::error(errors::invalid_state(1, "Cannot change sample rate while processing"));
        }
        sample_rate_ = sample_rate;
        return HALResult<void>::success();
    }
    
    HALResult<void> set_channel_count(uint16_t channels) override {
        if (processing_) {
            return HALResult<void>::error(errors::invalid_state(1, "Cannot change channel count while processing"));
        }
        channels_ = channels;
        return HALResult<void>::success();
    }
    
    uint32_t get_buffer_size() const override { return buffer_size_; }
    uint32_t get_sample_rate() const override { return sample_rate_; }
    uint16_t get_channel_count() const override { return channels_; }
    bool is_processing() const override { return processing_; }
    
    HALResult<uint32_t> get_latency_frames() const override {
        return HALResult<uint32_t>::success(buffer_size_ * 2); // Mock latency
    }

private:
    AudioResourceHandle handle_{3, AudioResourceType::ProcessorUnit, 1};
    uint32_t sample_rate_;
    uint16_t channels_;
    uint32_t buffer_size_;
    std::atomic<bool> processing_;
    AudioCallback callback_;
};

/**
 * @brief Mock spatial audio implementation
 */
class MockSpatialAudio : public ISpatialAudio {
public:
    HALResult<void> set_listener(const AudioListener& listener) override {
        listener_ = listener;
        return HALResult<void>::success();
    }
    
    HALResult<void> set_source_3d(AudioResourceHandle source, const Audio3DSource& params) override {
        sources_3d_[source.id] = params;
        return HALResult<void>::success();
    }
    
    HALResult<void> set_doppler_factor(float factor) override {
        doppler_factor_ = factor;
        return HALResult<void>::success();
    }
    
    HALResult<void> set_distance_model(DistanceModel model) override {
        distance_model_ = model;
        return HALResult<void>::success();
    }
    
    HALResult<void> set_speed_of_sound(float speed) override {
        speed_of_sound_ = speed;
        return HALResult<void>::success();
    }
    
    const AudioListener& get_listener() const override { return listener_; }
    bool is_3d_audio_supported() const override { return true; }

private:
    AudioListener listener_;
    std::unordered_map<uint32_t, Audio3DSource> sources_3d_;
    float doppler_factor_ = 1.0f;
    DistanceModel distance_model_ = DistanceModel::Inverse;
    float speed_of_sound_ = 343.0f;
};

/**
 * @brief Mock audio effect implementation
 */
class MockAudioEffect : public IAudioEffect {
public:
    explicit MockAudioEffect(AudioEffectType type, const AudioEffectParams& params)
        : type_(type), params_(params), enabled_(true) {}
    
    AudioResourceHandle get_handle() const override { return handle_; }
    AudioEffectType get_effect_type() const override { return type_; }
    
    HALResult<void> set_parameters(const AudioEffectParams& params) override {
        params_ = params;
        return HALResult<void>::success();
    }
    
    const AudioEffectParams& get_parameters() const override { return params_; }
    
    HALResult<void> set_parameter(std::string_view parameter_name, float value) override {
        params_.parameters[std::string(parameter_name)] = value;
        return HALResult<void>::success();
    }
    
    HALResult<float> get_parameter(std::string_view parameter_name) const override {
        auto it = params_.parameters.find(std::string(parameter_name));
        if (it != params_.parameters.end()) {
            return HALResult<float>::success(it->second);
        }
        return HALResult<float>::error(HALError(HALErrorCategory::Validation, 1, "Parameter not found", nullptr));
    }
    
    HALResult<void> set_enabled(bool enabled) override {
        enabled_ = enabled;
        return HALResult<void>::success();
    }
    
    bool is_enabled() const override { return enabled_; }
    
    HALResult<void> reset() override {
        params_.parameters.clear();
        enabled_ = true;
        return HALResult<void>::success();
    }

private:
    AudioResourceHandle handle_{4, AudioResourceType::EffectChain, 1};
    AudioEffectType type_;
    AudioEffectParams params_;
    bool enabled_;
};

/**
 * @brief Mock platform extension implementations
 */
class MockAICAExtension : public IAICAExtension {
public:
    HALResult<void> upload_to_audio_ram(const void* data, size_t size, size_t offset = 0) override {
        if (offset + size > 2 * 1024 * 1024) { // 2MB limit
            return HALResult<void>::error(HALError(HALErrorCategory::Resource, 1, "AICA audio RAM overflow", nullptr));
        }
        audio_ram_usage_ += size;
        return HALResult<void>::success();
    }
    
    HALResult<void> set_adpcm_parameters(uint32_t channel, uint32_t start_address, 
                                        uint32_t loop_address, uint32_t end_address) override {
        if (channel >= 64) {
            return HALResult<void>::error(HALError(HALErrorCategory::Validation, 1, "Invalid AICA channel", nullptr));
        }
        return HALResult<void>::success();
    }
    
    HALResult<void> configure_dsp_effect(uint32_t effect_slot, uint32_t effect_type,
                                        const std::map<std::string, float>& parameters) override {
        if (effect_slot >= 16) {
            return HALResult<void>::error(HALError(HALErrorCategory::Validation, 1, "Invalid DSP slot", nullptr));
        }
        return HALResult<void>::success();
    }
    
    size_t get_audio_ram_usage() const override { return audio_ram_usage_; }
    size_t get_available_audio_ram() const override { return 2 * 1024 * 1024 - audio_ram_usage_; }

private:
    size_t audio_ram_usage_ = 0;
};

class MockWebAudioExtension : public IWebAudioExtension {
public:
    HALResult<void> create_worklet_processor(std::string_view worklet_code, std::string_view processor_name) override {
        // Mock worklet creation
        return HALResult<void>::success();
    }
    
    HALResult<void> set_worklet_parameters(std::string_view processor_name,
                                          const std::map<std::string, float>& parameters) override {
        return HALResult<void>::success();
    }
    
    uint32_t get_context_sample_rate() const override { return 44100; }
    std::string get_context_state() const override { return "running"; }
};

class MockCoreAudioExtension : public ICoreAudioExtension {
public:
    HALResult<uint32_t> load_audio_unit(uint32_t component_type, uint32_t component_subtype, uint32_t manufacturer) override {
        return HALResult<uint32_t>::success(1); // Mock Audio Unit handle
    }
    
    HALResult<void> set_audio_unit_parameter(uint32_t audio_unit, uint32_t parameter_id, float value) override {
        return HALResult<void>::success();
    }
    
    uint32_t get_io_latency() const override { return 128; }
    
    HALResult<void> set_preferred_buffer_size(uint32_t buffer_size) override {
        preferred_buffer_size_ = buffer_size;
        return HALResult<void>::success();
    }

private:
    uint32_t preferred_buffer_size_ = 1024;
};

/**
 * @brief Mock audio driver implementation
 */
class MockAudioDriver : public IHALDriver, public IAudioInterface {
public:
    MockAudioDriver() : initialized_(false), next_handle_id_(1) {
        // Initialize device info
        device_info_.device_name = "Mock Audio Device";
        device_info_.driver_name = "Mock Audio Driver";
        device_info_.capability_mask = 
            static_cast<uint32_t>(AudioCapability::PCMPlayback) |
            static_cast<uint32_t>(AudioCapability::StreamingPlayback) |
            static_cast<uint32_t>(AudioCapability::CallbackProcessing) |
            static_cast<uint32_t>(AudioCapability::MultiChannel) |
            static_cast<uint32_t>(AudioCapability::SoftwareMixing) |
            static_cast<uint32_t>(AudioCapability::VolumeControl) |
            static_cast<uint32_t>(AudioCapability::SpatialAudio3D) |
            static_cast<uint32_t>(AudioCapability::SoftwareEffects) |
            static_cast<uint32_t>(AudioCapability::LowLatencyMode) |
            static_cast<uint32_t>(AudioCapability::ADPCMDecoding) |
            static_cast<uint32_t>(AudioCapability::StoreQueueOptimization) |
            static_cast<uint32_t>(AudioCapability::AudioWorklets) |
            static_cast<uint32_t>(AudioCapability::AudioUnitSupport);
        
        device_info_.supported_formats = {
            AudioFormat::PCM_S16, AudioFormat::PCM_F32, AudioFormat::ADPCM_4BIT
        };
        device_info_.supported_sample_rates = {22050, 44100, 48000, 96000};
        device_info_.supported_channel_counts = {1, 2, 6, 8};
        device_info_.min_buffer_size = 64;
        device_info_.max_buffer_size = 4096;
        device_info_.preferred_buffer_size = 1024;
        device_info_.input_latency_frames = 128;
        device_info_.output_latency_frames = 128;
        device_info_.has_hardware_mixing = false;
        device_info_.has_hardware_effects = false;
        device_info_.has_3d_audio = true;
        device_info_.max_sources = 32;
        device_info_.total_audio_memory = 0; // Unlimited
        device_info_.available_audio_memory = 0;
        
        // Create extension objects
        aica_extension_ = std::make_unique<MockAICAExtension>();
        web_audio_extension_ = std::make_unique<MockWebAudioExtension>();
        core_audio_extension_ = std::make_unique<MockCoreAudioExtension>();
        spatial_audio_ = std::make_unique<MockSpatialAudio>();
    }
    
    // IHALDriver implementation
    HALResult<void> initialize() override {
        initialized_ = true;
        return HALResult<void>::success();
    }
    
    HALResult<void> shutdown() override {
        initialized_ = false;
        return HALResult<void>::success();
    }
    
    bool is_initialized() const override { return initialized_; }
    
    std::string_view get_driver_name() const override { return "MockAudioDriver"; }
    
    HALResult<void*> get_interface(std::string_view interface_name) override {
        if (interface_name == "audio") {
            return HALResult<void*>::success(static_cast<IAudioInterface*>(this));
        }
        return HALResult<void*>::error(errors::driver_not_loaded(1, "Interface not found"));
    }
    
    // IAudioInterface implementation
    HALResult<void> initialize(const AudioDeviceDescriptor& /*device_descriptor*/) override {
        initialized_ = true;
        return HALResult<void>::success();
    }
    
    HALResult<void> shutdown() override { 
        initialized_ = false;
        return HALResult<void>::success(); 
    }
    
    const AudioDeviceInfo& get_device_info() const override { return device_info_; }
    bool is_initialized() const override { return initialized_; }
    
    HALResult<std::vector<AudioDeviceInfo>> get_available_devices() const override {
        return HALResult<std::vector<AudioDeviceInfo>>::success({device_info_});
    }
    
    HALResult<std::unique_ptr<IAudioBuffer>> create_buffer(const AudioBufferDescriptor& descriptor,
                                                          const void* initial_data = nullptr) override {
        auto buffer = std::make_unique<MockAudioBuffer>(descriptor);
        if (initial_data) {
            buffer->upload_data(initial_data, descriptor.frame_count);
        }
        return HALResult<std::unique_ptr<IAudioBuffer>>::success(std::move(buffer));
    }
    
    HALResult<std::unique_ptr<IAudioStream>> create_stream(const AudioStreamDescriptor& descriptor) override {
        auto stream = std::make_unique<MockAudioStream>(descriptor);
        return HALResult<std::unique_ptr<IAudioStream>>::success(std::move(stream));
    }
    
    HALResult<std::unique_ptr<IAudioProcessor>> create_processor(uint32_t sample_rate, uint16_t channels, uint32_t buffer_size) override {
        auto processor = std::make_unique<MockAudioProcessor>(sample_rate, channels, buffer_size);
        return HALResult<std::unique_ptr<IAudioProcessor>>::success(std::move(processor));
    }
    
    HALResult<std::unique_ptr<IAudioEffect>> create_effect(AudioEffectType effect_type, const AudioEffectParams& params = {}) override {
        auto effect = std::make_unique<MockAudioEffect>(effect_type, params);
        return HALResult<std::unique_ptr<IAudioEffect>>::success(std::move(effect));
    }
    
    ISpatialAudio* get_spatial_audio() override { return spatial_audio_.get(); }
    
    HALResult<void> set_master_volume(float volume) override {
        master_volume_ = std::clamp(volume, 0.0f, 1.0f);
        return HALResult<void>::success();
    }
    
    float get_master_volume() const override { return master_volume_; }
    
    HALResult<void> set_device_event_callback(AudioDeviceEventCallback callback) override {
        device_event_callback_ = callback;
        return HALResult<void>::success();
    }
    
    HALResult<void> convert_format(const void* input_data, size_t input_frames, AudioFormat input_format, uint16_t input_channels,
                                  void* output_data, size_t output_frames, AudioFormat output_format, uint16_t output_channels) override {
        // Use utility function
        audio::convert_samples(input_data, input_format, output_data, output_format, input_frames * input_channels);
        return HALResult<void>::success();
    }
    
    uint32_t get_bytes_per_sample(AudioFormat format) const override {
        return audio::get_bytes_per_sample(format);
    }
    
    bool supports_audio_capability(AudioCapability capability) const override {
        return (device_info_.capability_mask & static_cast<uint32_t>(capability)) != 0;
    }
    
    bool supports_format(AudioFormat format) const override {
        return std::find(device_info_.supported_formats.begin(), device_info_.supported_formats.end(), format) 
               != device_info_.supported_formats.end();
    }
    
    bool supports_sample_rate(uint32_t sample_rate) const override {
        return std::find(device_info_.supported_sample_rates.begin(), device_info_.supported_sample_rates.end(), sample_rate)
               != device_info_.supported_sample_rates.end();
    }
    
    std::vector<uint32_t> get_supported_sample_rates() const override {
        return device_info_.supported_sample_rates;
    }
    
    std::vector<AudioFormat> get_supported_formats() const override {
        return device_info_.supported_formats;
    }
    
    uint32_t get_max_sources() const override { return device_info_.max_sources; }
    uint32_t get_min_buffer_size() const override { return device_info_.min_buffer_size; }
    uint32_t get_max_buffer_size() const override { return device_info_.max_buffer_size; }
    
    AudioStats get_stats() const override {
        AudioStats stats{};
        stats.frames_processed = 44100 * 10; // Mock 10 seconds
        stats.buffer_underruns = 0;
        stats.buffer_overruns = 0;
        stats.callback_count = 1000;
        stats.dropped_samples = 0;
        stats.average_callback_time_us = 150.0;
        stats.peak_callback_time_us = 300.0;
        stats.cpu_load_percentage = 15.5;
        stats.audio_memory_used = 1024 * 1024; // 1MB
        stats.peak_audio_memory_used = 2048 * 1024; // 2MB
        stats.active_sources = 3;
        stats.active_effects = 2;
        return stats;
    }
    
    HALResult<void> reset_stats() override { return HALResult<void>::success(); }
    
    HALResult<size_t> get_audio_memory_usage() const override {
        return HALResult<size_t>::success(1024 * 1024); // 1MB mock usage
    }
    
    HALResult<size_t> get_available_audio_memory() const override {
        return HALResult<size_t>::success(64 * 1024 * 1024); // 64MB mock available
    }
    
    void* get_extension_interface(std::string_view extension_name) override {
        if (extension_name == "aica") return aica_extension_.get();
        if (extension_name == "web_audio") return web_audio_extension_.get();
        if (extension_name == "core_audio") return core_audio_extension_.get();
        return nullptr;
    }
    
    void* get_aica_extension() override { return aica_extension_.get(); }
    void* get_web_audio_extension() override { return web_audio_extension_.get(); }
    void* get_core_audio_extension() override { return core_audio_extension_.get(); }
    
    std::string_view get_interface_name() const override { return "audio"; }

private:
    bool initialized_;
    uint32_t next_handle_id_;
    AudioDeviceInfo device_info_;
    float master_volume_ = 1.0f;
    AudioDeviceEventCallback device_event_callback_;
    
    std::unique_ptr<MockAICAExtension> aica_extension_;
    std::unique_ptr<MockWebAudioExtension> web_audio_extension_;
    std::unique_ptr<MockCoreAudioExtension> core_audio_extension_;
    std::unique_ptr<MockSpatialAudio> spatial_audio_;
};

} // namespace flight::hal
