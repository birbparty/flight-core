# Flight HAL Audio Interface Design

## Overview

The Flight HAL Audio Interface provides a unified abstraction for audio systems spanning from Dreamcast AICA (2MB audio RAM, ADPCM compression) to modern systems (Web Audio API, Core Audio, DirectSound). It supports streaming audio, low-latency callbacks, 3D spatial audio, and hardware-accelerated processing.

## Architecture

### Core Components

1. **IAudioInterface** - Main audio system interface
2. **IAudioBuffer** - Audio sample buffer management
3. **IAudioStream** - High-level streaming audio
4. **IAudioProcessor** - Low-latency callback processing
5. **ISpatialAudio** - 3D positional audio
6. **IAudioEffect** - Audio effects processing

### Platform-Specific Extensions

- **IAICAExtension** - Dreamcast AICA hardware features
- **IWebAudioExtension** - Web Audio API worklets
- **ICoreAudioExtension** - macOS Audio Units

## Key Features

### Dual Processing Models

The interface supports two distinct audio processing approaches:

#### 1. Streaming Audio
For music and long sounds with automatic buffering and format conversion:

```cpp
// Create and play an audio stream
auto stream_desc = audio::make_stream_descriptor("music.ogg", 0.8f);
auto stream_result = audio_interface->create_stream(stream_desc);
if (stream_result) {
    auto stream = stream_result.take_value();
    stream->play();
}
```

#### 2. Callback Processing
For low-latency effects and real-time synthesis:

```cpp
// Create a low-latency processor
auto processor_result = audio_interface->create_processor(44100, 2, 256);
if (processor_result) {
    auto processor = processor_result.take_value();
    
    processor->set_callback([](float* input, float* output, uint32_t frames, double time, uint32_t channels) {
        // Real-time audio processing here
        for (uint32_t i = 0; i < frames * channels; ++i) {
            output[i] = input ? input[i] * 0.5f : 0.0f; // Simple gain
        }
    });
    
    processor->start_processing();
}
```

### 3D Spatial Audio

Comprehensive 3D audio support with distance attenuation, Doppler effects, and environmental simulation:

```cpp
// Setup 3D audio listener
auto spatial = audio_interface->get_spatial_audio();
if (spatial) {
    AudioListener listener = audio::make_default_listener();
    listener.position = Vector3{0, 0, 0};
    listener.forward = Vector3{0, 0, -1};
    spatial->set_listener(listener);
    
    // Configure 3D source
    Audio3DSource source = audio::make_3d_source(Vector3{10, 0, -5});
    source.min_distance = 1.0f;
    source.max_distance = 50.0f;
    spatial->set_source_3d(stream_handle, source);
}
```

### Platform Capability Detection

Runtime capability detection enables graceful degradation:

```cpp
bool supports_3d = audio_interface->supports_audio_capability(AudioCapability::SpatialAudio3D);
bool supports_adpcm = audio_interface->supports_audio_capability(AudioCapability::ADPCMDecoding);
bool supports_gpu = audio_interface->supports_audio_capability(AudioCapability::GPUAcceleration);

// Adapt behavior based on capabilities
if (supports_3d) {
    setup_3d_audio();
} else {
    setup_stereo_panning();
}
```

## Audio Formats

### PCM Formats (Universal Support)
- **PCM_U8/S16/S24/S32** - Integer PCM formats
- **PCM_F32/F64** - Floating-point PCM formats

### Compressed Formats (Platform-Dependent)
- **ADPCM_4BIT** - 4-bit ADPCM (Dreamcast AICA)
- **DREAMCAST_ADPCM** - Dreamcast-specific ADPCM variant
- **PSP_ATRAC3** - PSP ATRAC3 compression
- **MP3/OGG_VORBIS/AAC/FLAC** - Modern compressed formats

### Special Formats
- **RAW_INTERLEAVED/RAW_PLANAR** - Raw sample data
- **WEB_AUDIO_BUFFER** - Web Audio API optimized format

## Platform-Specific Optimizations

### Dreamcast AICA
- **2MB Audio RAM Management** - Efficient use of limited audio memory
- **Hardware ADPCM Decoding** - 4-bit ADPCM compression support
- **64 Hardware Channels** - Concurrent audio source management
- **Store Queue Optimization** - Fast audio data transfers

```cpp
// Access AICA-specific features
auto aica = static_cast<IAICAExtension*>(audio_interface->get_aica_extension());
if (aica) {
    // Upload to AICA audio RAM
    aica->upload_to_audio_ram(adpcm_data, data_size, 0);
    
    // Configure ADPCM channel
    aica->set_adpcm_parameters(0, start_addr, loop_addr, end_addr);
    
    // Hardware DSP effects
    std::map<std::string, float> reverb_params = {{"wet_level", 0.3f}, {"room_size", 0.7f}};
    aica->configure_dsp_effect(0, 1, reverb_params);
}
```

### Web Audio API
- **AudioWorklets** - Low-latency scriptable processors
- **AudioContext Management** - Browser audio context handling
- **Format Optimization** - Browser-native audio formats

```cpp
// Web Audio worklet processor
auto web_audio = static_cast<IWebAudioExtension*>(audio_interface->get_web_audio_extension());
if (web_audio) {
    std::string worklet_code = R"(
        class CustomProcessor extends AudioWorkletProcessor {
            process(inputs, outputs, parameters) {
                // Custom audio processing
                return true;
            }
        }
        registerProcessor('custom-processor', CustomProcessor);
    )";
    
    web_audio->create_worklet_processor(worklet_code, "custom-processor");
}
```

### macOS Core Audio
- **Audio Units** - Professional audio plugin architecture
- **Low-Latency I/O** - Sub-millisecond audio processing
- **MIDI Integration** - Hardware MIDI device support

```cpp
// Core Audio professional features
auto core_audio = static_cast<ICoreAudioExtension*>(audio_interface->get_core_audio_extension());
if (core_audio) {
    // Load professional reverb Audio Unit
    auto au_result = core_audio->load_audio_unit(
        kAudioUnitType_Effect,
        kAudioUnitSubType_MatrixReverb,
        kAudioUnitManufacturer_Apple
    );
    
    if (au_result) {
        uint32_t reverb_unit = au_result.value();
        core_audio->set_audio_unit_parameter(reverb_unit, 0, 0.5f); // Room size
        core_audio->set_audio_unit_parameter(reverb_unit, 1, 0.3f); // Wet/dry mix
    }
}
```

## Audio Effects System

Flexible audio effects with platform-specific optimizations:

```cpp
// Create and configure effects
AudioEffectParams reverb_params;
reverb_params.type = AudioEffectType::Reverb;
reverb_params.parameters["room_size"] = 0.7f;
reverb_params.parameters["damping"] = 0.5f;
reverb_params.wet_level = 0.3f;
reverb_params.dry_level = 0.7f;

auto reverb_result = audio_interface->create_effect(AudioEffectType::Reverb, reverb_params);
if (reverb_result) {
    auto reverb = reverb_result.take_value();
    
    // Real-time parameter adjustment
    reverb->set_parameter("room_size", 0.9f);
    reverb->set_enabled(true);
}
```

## Resource Coordination Integration

The audio interface integrates with the HAL resource coordination system for cross-driver communication:

```cpp
// GPU-accelerated audio processing coordination
class AudioGPUProcessor {
    HALResult<void> process_with_gpu(AudioResourceHandle source) {
        // Request GPU resources for convolution
        auto gpu_result = request_gpu_convolution(source, convolution_params);
        
        // Coordinate with graphics driver for shared GPU usage
        return gpu_result;
    }
};
```

## Memory Management

Efficient audio memory management with platform-aware allocation:

```cpp
// Create audio buffer with memory type hints
AudioBufferDescriptor buffer_desc = audio::make_buffer_descriptor(
    1024,           // frames
    44100,          // sample rate  
    2,              // channels
    AudioFormat::PCM_F32
);
buffer_desc.usage = AudioBufferUsage::Streaming;
buffer_desc.low_latency_hint = true;

auto buffer_result = audio_interface->create_buffer(buffer_desc);
```

## Performance Monitoring

Built-in performance monitoring and statistics:

```cpp
// Monitor audio performance
AudioStats stats = audio_interface->get_stats();

printf("Buffer underruns: %llu\n", stats.buffer_underruns);
printf("Average callback time: %.2f μs\n", stats.average_callback_time_us);
printf("CPU load: %.1f%%\n", stats.cpu_load_percentage);
printf("Active sources: %u\n", stats.active_sources);

// Memory usage monitoring
auto memory_result = audio_interface->get_audio_memory_usage();
if (memory_result) {
    printf("Audio memory used: %zu bytes\n", memory_result.value());
}
```

## Error Handling

Comprehensive error handling with detailed diagnostics:

```cpp
auto stream_result = audio_interface->create_stream(stream_desc);
if (!stream_result) {
    const auto& error = stream_result.error();
    HAL_LOG_ERROR(error);
    
    // Handle specific error types
    switch (error.category()) {
        case HALErrorCategory::Resource:
            // Handle resource allocation failure
            break;
        case HALErrorCategory::Platform:
            // Handle platform-specific issues
            break;
        case HALErrorCategory::Validation:
            // Handle parameter validation errors
            break;
    }
}
```

## Design Principles

### 1. Platform Abstraction
- Unified API across all supported platforms
- Platform-specific optimizations through extensions
- Graceful degradation when features are unavailable

### 2. Performance Focus
- Zero-copy audio processing where possible
- Lock-free real-time callbacks
- Memory-pool allocation for low latency

### 3. Memory Efficiency
- Platform-aware memory allocation
- Support for constrained environments (Dreamcast: 2MB audio RAM)
- Efficient format conversion and buffering

### 4. Extensibility
- Plugin architecture for custom effects
- Platform-specific extension interfaces
- Future-proof capability detection

## Usage Examples

### Basic Audio Playback
```cpp
// Initialize audio system
AudioDeviceDescriptor device_desc = audio::make_device_descriptor();
auto init_result = audio_interface->initialize(device_desc);

// Play a sound file
auto stream_desc = audio::make_stream_descriptor("sound.wav");
auto stream = audio_interface->create_stream(stream_desc).value();
stream->play();
```

### Real-Time Audio Processing
```cpp
// Low-latency audio processor
auto processor = audio_interface->create_processor(44100, 2, 128).value();

processor->set_callback([](float* input, float* output, uint32_t frames, double time, uint32_t channels) {
    // Apply real-time reverb effect
    for (uint32_t i = 0; i < frames * channels; ++i) {
        float delayed = get_delayed_sample(input[i], 0.1); // 100ms delay
        output[i] = input[i] * 0.7f + delayed * 0.3f;
    }
});

processor->start_processing();
```

### 3D Audio Game Scene
```cpp
// Setup 3D audio environment
auto spatial = audio_interface->get_spatial_audio();
AudioListener listener = audio::make_default_listener();
listener.position = player_position;
listener.forward = player_forward;
spatial->set_listener(listener);

// Position enemy audio sources
for (auto& enemy : enemies) {
    Audio3DSource source = audio::make_3d_source(enemy.position);
    source.velocity = enemy.velocity; // For Doppler effect
    spatial->set_source_3d(enemy.audio_handle, source);
}
```

This comprehensive audio interface provides the foundation for professional audio applications while maintaining compatibility across diverse platforms from retro gaming systems to modern audio workstations.
