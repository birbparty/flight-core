/**
 * @file comprehensive_audio_example.cpp
 * @brief Comprehensive Audio Interface Usage Example
 * 
 * Demonstrates all major features of the Flight HAL Audio Interface including
 * streaming audio, low-latency callbacks, 3D spatial audio, and platform-specific
 * optimizations across Dreamcast AICA to modern audio systems.
 */

#include "../../include/flight/hal/interfaces/audio.hpp"
#include "../../include/flight/hal/core/hal_logging.hpp"
#include "../../include/flight/hal/core/driver_registry.hpp"
#include "../drivers/mock_audio_driver.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <memory>
#include <cmath>

using namespace flight::hal;
using namespace flight::hal::audio;

/**
 * @brief Example audio synthesizer using callbacks
 */
class SimpleSynthesizer {
public:
    SimpleSynthesizer(float frequency = 440.0f, float amplitude = 0.3f)
        : frequency_(frequency), amplitude_(amplitude), phase_(0.0f) {}
    
    void process(float* output, uint32_t frame_count, uint32_t channels, double sample_rate) {
        const float phase_increment = 2.0f * 3.14159f * frequency_ / static_cast<float>(sample_rate);
        
        for (uint32_t frame = 0; frame < frame_count; ++frame) {
            float sample = std::sin(phase_) * amplitude_;
            phase_ += phase_increment;
            
            if (phase_ >= 2.0f * 3.14159f) {
                phase_ -= 2.0f * 3.14159f;
            }
            
            // Output to all channels
            for (uint32_t ch = 0; ch < channels; ++ch) {
                output[frame * channels + ch] = sample;
            }
        }
    }
    
    void set_frequency(float frequency) { frequency_ = frequency; }
    void set_amplitude(float amplitude) { amplitude_ = amplitude; }

private:
    float frequency_;
    float amplitude_;
    float phase_;
};

/**
 * @brief Simple delay effect for demonstration
 */
class SimpleDelay {
public:
    explicit SimpleDelay(size_t buffer_size = 44100) // 1 second at 44.1kHz
        : buffer_(buffer_size, 0.0f), write_pos_(0) {}
    
    float process(float input, float delay_time = 0.3f, float feedback = 0.4f, float mix = 0.3f) {
        size_t delay_samples = static_cast<size_t>(delay_time * buffer_.size());
        size_t read_pos = (write_pos_ + buffer_.size() - delay_samples) % buffer_.size();
        
        float delayed = buffer_[read_pos];
        float output = input + delayed * mix;
        
        buffer_[write_pos_] = input + delayed * feedback;
        write_pos_ = (write_pos_ + 1) % buffer_.size();
        
        return output;
    }

private:
    std::vector<float> buffer_;
    size_t write_pos_;
};

/**
 * @brief Demonstrate basic audio streaming
 */
void demonstrate_audio_streaming(IAudioInterface* audio_interface) {
    std::cout << "\n=== Audio Streaming Demonstration ===\n";
    
    // Create audio stream for background music
    AudioStreamDescriptor music_desc = make_stream_descriptor("background_music.ogg", 0.7f);
    music_desc.loop_enabled = true;
    music_desc.preload_hint = true;
    music_desc.debug_name = "BackgroundMusic";
    
    auto music_result = audio_interface->create_stream(music_desc);
    if (!music_result) {
        std::cout << "Failed to create music stream: " << music_result.error().message() << "\n";
        return;
    }
    
    auto music_stream = std::move(music_result).value();
    std::cout << "✓ Created background music stream\n";
    
    // Simulate loading from memory (mock data)
    std::vector<int16_t> mock_audio_data(44100 * 2); // 1 second of stereo audio
    for (size_t i = 0; i < mock_audio_data.size(); i += 2) {
        float t = static_cast<float>(i / 2) / 44100.0f;
        float sample = std::sin(2.0f * 3.14159f * 440.0f * t) * 0.3f; // 440Hz tone
        
        mock_audio_data[i] = static_cast<int16_t>(sample * 32767);     // Left
        mock_audio_data[i + 1] = static_cast<int16_t>(sample * 32767); // Right
    }
    
    auto load_result = music_stream->load_from_memory(mock_audio_data.data(), 
                                                     mock_audio_data.size() * sizeof(int16_t));
    if (load_result) {
        std::cout << "✓ Loaded mock audio data\n";
    }
    
    // Play the stream
    auto play_result = music_stream->play();
    if (play_result) {
        std::cout << "✓ Started playing background music\n";
        
        // Demonstrate volume control
        music_stream->set_volume(0.5f);
        std::cout << "✓ Adjusted volume to 50%\n";
        
        // Check playback status
        if (music_stream->is_playing()) {
            std::cout << "✓ Stream is playing\n";
        }
        
        auto duration_result = music_stream->get_duration();
        if (duration_result) {
            std::cout << "Stream duration: " << duration_result.value() << " seconds\n";
        }
    }
    
    // Create sound effect stream
    AudioStreamDescriptor sfx_desc = make_stream_descriptor("explosion.wav", 1.0f);
    sfx_desc.loop_enabled = false;
    sfx_desc.debug_name = "ExplosionSFX";
    
    auto sfx_result = audio_interface->create_stream(sfx_desc);
    if (sfx_result) {
        auto sfx_stream = std::move(sfx_result).value();
        
        // Load and play sound effect
        sfx_stream->load_from_memory(mock_audio_data.data(), mock_audio_data.size() * sizeof(int16_t));
        sfx_stream->play();
        
        std::cout << "✓ Created and played sound effect\n";
    }
}

/**
 * @brief Demonstrate low-latency callback processing
 */
void demonstrate_callback_processing(IAudioInterface* audio_interface) {
    std::cout << "\n=== Low-Latency Callback Processing ===\n";
    
    // Create audio processor for real-time synthesis
    auto processor_result = audio_interface->create_processor(44100, 2, 256);
    if (!processor_result) {
        std::cout << "Failed to create audio processor: " << processor_result.error().message() << "\n";
        return;
    }
    
    auto processor = std::move(processor_result).value();
    std::cout << "✓ Created audio processor (44.1kHz, stereo, 256 frames)\n";
    
    // Create synthesizer and effects
    auto synthesizer = std::make_shared<SimpleSynthesizer>(440.0f, 0.3f);
    auto delay_effect = std::make_shared<SimpleDelay>(44100);
    
    // Set up real-time audio callback
    processor->set_callback([synthesizer, delay_effect](
        float* input_buffer, 
        float* output_buffer, 
        uint32_t frame_count, 
        double sample_time, 
        uint32_t channels) {
        
        // Generate synthesized audio
        synthesizer->process(output_buffer, frame_count, channels, 44100.0);
        
        // Apply delay effect
        for (uint32_t frame = 0; frame < frame_count; ++frame) {
            for (uint32_t ch = 0; ch < channels; ++ch) {
                size_t index = frame * channels + ch;
                output_buffer[index] = delay_effect->process(output_buffer[index]);
            }
        }
    });
    
    std::cout << "✓ Set up real-time audio callback with synthesizer and delay\n";
    
    // Start processing
    auto start_result = processor->start_processing();
    if (start_result) {
        std::cout << "✓ Started real-time audio processing\n";
        
        // Demonstrate parameter changes during playback
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        synthesizer->set_frequency(880.0f); // Octave up
        std::cout << "✓ Changed frequency to 880Hz\n";
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        synthesizer->set_frequency(220.0f); // Octave down
        std::cout << "✓ Changed frequency to 220Hz\n";
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Get processor information
        std::cout << "Buffer size: " << processor->get_buffer_size() << " frames\n";
        std::cout << "Sample rate: " << processor->get_sample_rate() << " Hz\n";
        std::cout << "Channels: " << processor->get_channel_count() << "\n";
        
        auto latency_result = processor->get_latency_frames();
        if (latency_result) {
            std::cout << "Latency: " << latency_result.value() << " frames\n";
        }
        
        // Stop processing
        processor->stop_processing();
        std::cout << "✓ Stopped real-time audio processing\n";
    }
}

/**
 * @brief Demonstrate 3D spatial audio
 */
void demonstrate_spatial_audio(IAudioInterface* audio_interface) {
    std::cout << "\n=== 3D Spatial Audio Demonstration ===\n";
    
    // Check for 3D audio support
    bool supports_3d = audio_interface->supports_audio_capability(AudioCapability::SpatialAudio3D);
    if (!supports_3d) {
        std::cout << "3D spatial audio not supported on this platform\n";
        return;
    }
    
    auto spatial = audio_interface->get_spatial_audio();
    if (!spatial) {
        std::cout << "Spatial audio interface not available\n";
        return;
    }
    
    std::cout << "✓ 3D spatial audio is supported\n";
    
    // Set up 3D audio listener (player position)
    AudioListener listener = make_default_listener();
    listener.position = Vector3{0.0f, 0.0f, 0.0f};
    listener.forward = Vector3{0.0f, 0.0f, -1.0f};
    listener.up = Vector3{0.0f, 1.0f, 0.0f};
    listener.gain = 1.0f;
    
    auto listener_result = spatial->set_listener(listener);
    if (listener_result) {
        std::cout << "✓ Set 3D audio listener at origin\n";
    }
    
    // Configure distance attenuation model
    spatial->set_distance_model(DistanceModel::Inverse);
    spatial->set_doppler_factor(1.0f);
    spatial->set_speed_of_sound(343.0f); // m/s
    
    std::cout << "✓ Configured 3D audio parameters\n";
    
    // Create audio streams for 3D positioning
    auto stream1_result = audio_interface->create_stream(make_stream_descriptor("source1.wav"));
    auto stream2_result = audio_interface->create_stream(make_stream_descriptor("source2.wav"));
    
    if (stream1_result && stream2_result) {
        auto stream1 = std::move(stream1_result).value();
        auto stream2 = std::move(stream2_result).value();
        
        // Position first source to the left
        Audio3DSource source1 = make_3d_source(Vector3{-10.0f, 0.0f, 0.0f});
        source1.min_distance = 1.0f;
        source1.max_distance = 50.0f;
        source1.rolloff_factor = 1.0f;
        
        auto pos1_result = spatial->set_source_3d(stream1->get_handle(), source1);
        if (pos1_result) {
            std::cout << "✓ Positioned source 1 at (-10, 0, 0)\n";
        }
        
        // Position second source to the right with movement
        Audio3DSource source2 = make_3d_source(Vector3{10.0f, 0.0f, 0.0f});
        source2.velocity = Vector3{-5.0f, 0.0f, 0.0f}; // Moving left for Doppler
        source2.min_distance = 1.0f;
        source2.max_distance = 50.0f;
        source2.rolloff_factor = 1.0f;
        
        auto pos2_result = spatial->set_source_3d(stream2->get_handle(), source2);
        if (pos2_result) {
            std::cout << "✓ Positioned source 2 at (10, 0, 0) with velocity (-5, 0, 0)\n";
        }
        
        // Simulate moving audio sources
        for (int i = 0; i < 5; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            
            // Update source positions
            source1.position.x += 2.0f;
            source2.position.x -= 2.0f;
            
            spatial->set_source_3d(stream1->get_handle(), source1);
            spatial->set_source_3d(stream2->get_handle(), source2);
            
            std::cout << "Source 1 position: (" << source1.position.x << ", 0, 0)\n";
            std::cout << "Source 2 position: (" << source2.position.x << ", 0, 0)\n";
        }
        
        std::cout << "✓ Demonstrated moving 3D audio sources\n";
    }
}

/**
 * @brief Demonstrate audio effects processing
 */
void demonstrate_audio_effects(IAudioInterface* audio_interface) {
    std::cout << "\n=== Audio Effects Demonstration ===\n";
    
    // Create reverb effect
    AudioEffectParams reverb_params;
    reverb_params.type = AudioEffectType::Reverb;
    reverb_params.parameters["room_size"] = 0.7f;
    reverb_params.parameters["damping"] = 0.5f;
    reverb_params.parameters["wet_level"] = 0.3f;
    reverb_params.wet_level = 0.3f;
    reverb_params.dry_level = 0.7f;
    reverb_params.bypass = false;
    
    auto reverb_result = audio_interface->create_effect(AudioEffectType::Reverb, reverb_params);
    if (reverb_result) {
        auto reverb = std::move(reverb_result).value();
        std::cout << "✓ Created reverb effect\n";
        
        // Adjust parameters in real-time
        reverb->set_parameter("room_size", 0.9f);
        reverb->set_parameter("damping", 0.3f);
        std::cout << "✓ Adjusted reverb parameters\n";
        
        // Toggle effect
        reverb->set_enabled(false);
        std::cout << "✓ Disabled reverb effect\n";
        
        reverb->set_enabled(true);
        std::cout << "✓ Re-enabled reverb effect\n";
    }
    
    // Create delay effect
    AudioEffectParams delay_params;
    delay_params.type = AudioEffectType::Delay;
    delay_params.parameters["delay_time"] = 0.25f;  // 250ms
    delay_params.parameters["feedback"] = 0.4f;
    delay_params.wet_level = 0.3f;
    delay_params.dry_level = 0.7f;
    
    auto delay_result = audio_interface->create_effect(AudioEffectType::Delay, delay_params);
    if (delay_result) {
        auto delay = std::move(delay_result).value();
        std::cout << "✓ Created delay effect (250ms, 40% feedback)\n";
        
        // Get parameter value
        auto delay_time_result = delay->get_parameter("delay_time");
        if (delay_time_result) {
            std::cout << "Current delay time: " << delay_time_result.value() << "s\n";
        }
    }
    
    // Create parametric EQ
    AudioEffectParams eq_params;
    eq_params.type = AudioEffectType::ParametricEQ;
    eq_params.parameters["low_freq"] = 100.0f;
    eq_params.parameters["low_gain"] = 2.0f;     // +2dB
    eq_params.parameters["mid_freq"] = 1000.0f;
    eq_params.parameters["mid_gain"] = -1.0f;    // -1dB
    eq_params.parameters["high_freq"] = 8000.0f;
    eq_params.parameters["high_gain"] = 1.5f;    // +1.5dB
    
    auto eq_result = audio_interface->create_effect(AudioEffectType::ParametricEQ, eq_params);
    if (eq_result) {
        std::cout << "✓ Created parametric EQ with frequency shaping\n";
    }
}

/**
 * @brief Demonstrate platform-specific features
 */
void demonstrate_platform_features(IAudioInterface* audio_interface) {
    std::cout << "\n=== Platform-Specific Features ===\n";
    
    // Check platform capabilities
    std::cout << "Platform Audio Capabilities:\n";
    std::cout << "  ADPCM Decoding: " << 
        (audio_interface->supports_audio_capability(AudioCapability::ADPCMDecoding) ? "Yes" : "No") << "\n";
    std::cout << "  Hardware Mixing: " << 
        (audio_interface->supports_audio_capability(AudioCapability::HardwareMixing) ? "Yes" : "No") << "\n";
    std::cout << "  Low Latency Mode: " << 
        (audio_interface->supports_audio_capability(AudioCapability::LowLatencyMode) ? "Yes" : "No") << "\n";
    std::cout << "  GPU Acceleration: " << 
        (audio_interface->supports_audio_capability(AudioCapability::GPUAcceleration) ? "Yes" : "No") << "\n";
    std::cout << "  Store Queue Optimization: " << 
        (audio_interface->supports_audio_capability(AudioCapability::StoreQueueOptimization) ? "Yes" : "No") << "\n";
    std::cout << "  Audio Worklets: " << 
        (audio_interface->supports_audio_capability(AudioCapability::AudioWorklets) ? "Yes" : "No") << "\n";
    std::cout << "  Audio Units: " << 
        (audio_interface->supports_audio_capability(AudioCapability::AudioUnitSupport) ? "Yes" : "No") << "\n";
    
    // Dreamcast AICA features
    auto aica = static_cast<IAICAExtension*>(audio_interface->get_aica_extension());
    if (aica) {
        std::cout << "\n--- Dreamcast AICA Features ---\n";
        std::cout << "AICA audio RAM usage: " << aica->get_audio_ram_usage() << " bytes\n";
        std::cout << "Available audio RAM: " << aica->get_available_audio_ram() << " bytes\n";
        
        // Demo ADPCM upload
        std::vector<uint8_t> adpcm_data(1024, 0x80); // Mock ADPCM data
        auto upload_result = aica->upload_to_audio_ram(adpcm_data.data(), adpcm_data.size());
        if (upload_result) {
            std::cout << "✓ Uploaded ADPCM data to AICA audio RAM\n";
        }
        
        // Configure ADPCM channel
        auto adpcm_result = aica->set_adpcm_parameters(0, 0x1000, 0x1200, 0x1400);
        if (adpcm_result) {
            std::cout << "✓ Configured ADPCM channel 0\n";
        }
    }
    
    // Web Audio features
    auto web_audio = static_cast<IWebAudioExtension*>(audio_interface->get_web_audio_extension());
    if (web_audio) {
        std::cout << "\n--- Web Audio Features ---\n";
        std::cout << "AudioContext sample rate: " << web_audio->get_context_sample_rate() << " Hz\n";
        std::cout << "AudioContext state: " << web_audio->get_context_state() << "\n";
        
        // Create simple worklet
        std::string worklet_code = R"(
            class GainProcessor extends AudioWorkletProcessor {
                process(inputs, outputs, parameters) {
                    const input = inputs[0];
                    const output = outputs[0];
                    
                    for (let channel = 0; channel < output.length; ++channel) {
                        const inputChannel = input[channel];
                        const outputChannel = output[channel];
                        
                        for (let i = 0; i < outputChannel.length; ++i) {
                            outputChannel[i] = inputChannel ? inputChannel[i] * 0.5 : 0;
                        }
                    }
                    
                    return true;
                }
            }
            
            registerProcessor('gain-processor', GainProcessor);
        )";
        
        auto worklet_result = web_audio->create_worklet_processor(worklet_code, "gain-processor");
        if (worklet_result) {
            std::cout << "✓ Created Web Audio worklet processor\n";
        }
    }
    
    // Core Audio features
    auto core_audio = static_cast<ICoreAudioExtension*>(audio_interface->get_core_audio_extension());
    if (core_audio) {
        std::cout << "\n--- Core Audio Features ---\n";
        std::cout << "I/O Latency: " << core_audio->get_io_latency() << " frames\n";
        
        // Set preferred buffer size
        auto buffer_result = core_audio->set_preferred_buffer_size(128);
        if (buffer_result) {
            std::cout << "✓ Set preferred buffer size to 128 frames\n";
        }
    }
}

/**
 * @brief Demonstrate performance monitoring
 */
void demonstrate_performance_monitoring(IAudioInterface* audio_interface) {
    std::cout << "\n=== Performance Monitoring ===\n";
    
    // Get audio statistics
    AudioStats stats = audio_interface->get_stats();
    
    std::cout << "Audio Performance Statistics:\n";
    std::cout << "  Frames processed: " << stats.frames_processed << "\n";
    std::cout << "  Buffer underruns: " << stats.buffer_underruns << "\n";
    std::cout << "  Buffer overruns: " << stats.buffer_overruns << "\n";
    std::cout << "  Callback count: " << stats.callback_count << "\n";
    std::cout << "  Dropped samples: " << stats.dropped_samples << "\n";
    std::cout << "  Average callback time: " << stats.average_callback_time_us << " μs\n";
    std::cout << "  Peak callback time: " << stats.peak_callback_time_us << " μs\n";
    std::cout << "  CPU load: " << stats.cpu_load_percentage << "%\n";
    std::cout << "  Active sources: " << stats.active_sources << "\n";
    std::cout << "  Active effects: " << stats.active_effects << "\n";
    
    // Memory usage
    auto memory_result = audio_interface->get_audio_memory_usage();
    if (memory_result) {
        std::cout << "  Audio memory used: " << memory_result.value() << " bytes\n";
    }
    
    auto available_result = audio_interface->get_available_audio_memory();
    if (available_result) {
        std::cout << "  Available audio memory: " << available_result.value() << " bytes\n";
    }
    
    // Reset statistics
    audio_interface->reset_stats();
    std::cout << "✓ Reset performance statistics\n";
}

/**
 * @brief Main demonstration function
 */
int main() {
    std::cout << "=== Flight HAL Comprehensive Audio Example ===\n";
    
    // Initialize HAL system
    auto& registry = DriverRegistry::instance();
    
    // Register mock audio driver for demonstration
    auto mock_driver = std::make_shared<MockAudioDriver>();
    auto register_result = registry.register_driver("audio", mock_driver);
    if (!register_result) {
        std::cout << "Failed to register mock audio driver\n";
        return 1;
    }
    
    // Get audio interface
    auto interface_result = registry.get_interface<IAudioInterface>("audio");
    if (!interface_result) {
        std::cout << "Failed to get audio interface: " << interface_result.error().message() << "\n";
        return 1;
    }
    
    auto audio_interface = interface_result.value();
    std::cout << "✓ Audio interface obtained\n";
    
    // Initialize audio system
    AudioDeviceDescriptor device_desc = make_device_descriptor(44100, 2, 1024);
    device_desc.low_latency_mode = true;
    
    auto init_result = audio_interface->initialize(device_desc);
    if (!init_result) {
        std::cout << "Failed to initialize audio system: " << init_result.error().message() << "\n";
        return 1;
    }
    
    std::cout << "✓ Audio system initialized\n";
    
    // Get device information
    const auto& device_info = audio_interface->get_device_info();
    std::cout << "Audio Device: " << device_info.device_name << "\n";
    std::cout << "Driver: " << device_info.driver_name << "\n";
    std::cout << "Max sources: " << device_info.max_sources << "\n";
    std::cout << "Preferred buffer size: " << device_info.preferred_buffer_size << " frames\n";
    
    // Run demonstrations
    try {
        demonstrate_audio_streaming(audio_interface);
        demonstrate_callback_processing(audio_interface);
        demonstrate_spatial_audio(audio_interface);
        demonstrate_audio_effects(audio_interface);
        demonstrate_platform_features(audio_interface);
        demonstrate_performance_monitoring(audio_interface);
        
        std::cout << "\n=== All Audio Demonstrations Completed Successfully ===\n";
        
    } catch (const std::exception& e) {
        std::cout << "Exception during demonstration: " << e.what() << "\n";
        return 1;
    }
    
    // Cleanup
    auto shutdown_result = audio_interface->shutdown();
    if (shutdown_result) {
        std::cout << "✓ Audio system shut down cleanly\n";
    }
    
    std::cout << "\n=== Audio Example Completed ===\n";
    return 0;
}
