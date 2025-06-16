/**
 * @file audio.cpp
 * @brief Flight HAL Audio Interface Implementation
 * 
 * Core implementation for audio utility functions and support classes.
 */

#include "../../include/flight/hal/interfaces/audio.hpp"
#include "../../include/flight/hal/core/hal_logging.hpp"
#include <cstring>
#include <algorithm>
#include <cmath>

namespace flight::hal::audio {

/**
 * @brief Convert audio capability to string
 */
const char* to_string(AudioCapability capability) {
    switch (capability) {
        case AudioCapability::PCMPlayback: return "PCMPlayback";
        case AudioCapability::StreamingPlayback: return "StreamingPlayback";
        case AudioCapability::CallbackProcessing: return "CallbackProcessing";
        case AudioCapability::MultiChannel: return "MultiChannel";
        case AudioCapability::HardwareMixing: return "HardwareMixing";
        case AudioCapability::SoftwareMixing: return "SoftwareMixing";
        case AudioCapability::VolumeControl: return "VolumeControl";
        case AudioCapability::PanControl: return "PanControl";
        case AudioCapability::PCMFormats: return "PCMFormats";
        case AudioCapability::CompressedFormats: return "CompressedFormats";
        case AudioCapability::ADPCMDecoding: return "ADPCMDecoding";
        case AudioCapability::RealtimeDecoding: return "RealtimeDecoding";
        case AudioCapability::HardwareDSP: return "HardwareDSP";
        case AudioCapability::SoftwareEffects: return "SoftwareEffects";
        case AudioCapability::ConvolutionReverb: return "ConvolutionReverb";
        case AudioCapability::ParametricEQ: return "ParametricEQ";
        case AudioCapability::SpatialAudio3D: return "SpatialAudio3D";
        case AudioCapability::DistanceAttenuation: return "DistanceAttenuation";
        case AudioCapability::DopplerEffect: return "DopplerEffect";
        case AudioCapability::EnvironmentalAudio: return "EnvironmentalAudio";
        case AudioCapability::LowLatencyMode: return "LowLatencyMode";
        case AudioCapability::VariableBufferSize: return "VariableBufferSize";
        case AudioCapability::MultiThreadedProcessing: return "MultiThreadedProcessing";
        case AudioCapability::GPUAcceleration: return "GPUAcceleration";
        case AudioCapability::StoreQueueOptimization: return "StoreQueueOptimization";
        case AudioCapability::AudioWorklets: return "AudioWorklets";
        case AudioCapability::AudioUnitSupport: return "AudioUnitSupport";
        case AudioCapability::DirectSoundSupport: return "DirectSoundSupport";
        case AudioCapability::AudioRecording: return "AudioRecording";
        case AudioCapability::RealtimeMonitoring: return "RealtimeMonitoring";
        case AudioCapability::FullDuplexAudio: return "FullDuplexAudio";
        case AudioCapability::EchoCancellation: return "EchoCancellation";
        default: return "Unknown";
    }
}

/**
 * @brief Convert audio format to string
 */
const char* to_string(AudioFormat format) {
    switch (format) {
        case AudioFormat::PCM_U8: return "PCM_U8";
        case AudioFormat::PCM_S16: return "PCM_S16";
        case AudioFormat::PCM_S24: return "PCM_S24";
        case AudioFormat::PCM_S32: return "PCM_S32";
        case AudioFormat::PCM_F32: return "PCM_F32";
        case AudioFormat::PCM_F64: return "PCM_F64";
        case AudioFormat::ADPCM_4BIT: return "ADPCM_4BIT";
        case AudioFormat::ADPCM_IMA: return "ADPCM_IMA";
        case AudioFormat::MP3: return "MP3";
        case AudioFormat::OGG_VORBIS: return "OGG_VORBIS";
        case AudioFormat::AAC: return "AAC";
        case AudioFormat::FLAC: return "FLAC";
        case AudioFormat::RAW_INTERLEAVED: return "RAW_INTERLEAVED";
        case AudioFormat::RAW_PLANAR: return "RAW_PLANAR";
        case AudioFormat::DREAMCAST_ADPCM: return "DREAMCAST_ADPCM";
        case AudioFormat::PSP_ATRAC3: return "PSP_ATRAC3";
        case AudioFormat::WEB_AUDIO_BUFFER: return "WEB_AUDIO_BUFFER";
        default: return "Unknown";
    }
}

/**
 * @brief Convert audio effect type to string
 */
const char* to_string(AudioEffectType effect_type) {
    switch (effect_type) {
        case AudioEffectType::None: return "None";
        case AudioEffectType::Gain: return "Gain";
        case AudioEffectType::Pan: return "Pan";
        case AudioEffectType::Delay: return "Delay";
        case AudioEffectType::Reverb: return "Reverb";
        case AudioEffectType::LowPassFilter: return "LowPassFilter";
        case AudioEffectType::HighPassFilter: return "HighPassFilter";
        case AudioEffectType::BandPassFilter: return "BandPassFilter";
        case AudioEffectType::ParametricEQ: return "ParametricEQ";
        case AudioEffectType::Compressor: return "Compressor";
        case AudioEffectType::Limiter: return "Limiter";
        case AudioEffectType::Gate: return "Gate";
        case AudioEffectType::Expander: return "Expander";
        case AudioEffectType::Chorus: return "Chorus";
        case AudioEffectType::Flanger: return "Flanger";
        case AudioEffectType::Phaser: return "Phaser";
        case AudioEffectType::Tremolo: return "Tremolo";
        case AudioEffectType::Overdrive: return "Overdrive";
        case AudioEffectType::Distortion: return "Distortion";
        case AudioEffectType::BitCrusher: return "BitCrusher";
        case AudioEffectType::Spatialization: return "Spatialization";
        case AudioEffectType::ConvolutionReverb: return "ConvolutionReverb";
        case AudioEffectType::Binaural: return "Binaural";
        case AudioEffectType::DreamcastDSP: return "DreamcastDSP";
        case AudioEffectType::WebAudioWorklet: return "WebAudioWorklet";
        case AudioEffectType::CoreAudioUnit: return "CoreAudioUnit";
        default: return "Unknown";
    }
}

/**
 * @brief Convert distance model to string
 */
const char* to_string(DistanceModel model) {
    switch (model) {
        case DistanceModel::None: return "None";
        case DistanceModel::Linear: return "Linear";
        case DistanceModel::Inverse: return "Inverse";
        case DistanceModel::Exponential: return "Exponential";
        default: return "Unknown";
    }
}

/**
 * @brief Get bytes per sample for audio format
 */
uint32_t get_bytes_per_sample(AudioFormat format) {
    switch (format) {
        case AudioFormat::PCM_U8:
            return 1;
        case AudioFormat::PCM_S16:
            return 2;
        case AudioFormat::PCM_S24:
            return 3;
        case AudioFormat::PCM_S32:
        case AudioFormat::PCM_F32:
            return 4;
        case AudioFormat::PCM_F64:
            return 8;
        case AudioFormat::ADPCM_4BIT:
            return 1; // Variable, but approximate
        case AudioFormat::ADPCM_IMA:
            return 1; // Variable, but approximate
        case AudioFormat::DREAMCAST_ADPCM:
            return 1; // 4-bit ADPCM
        default:
            return 0; // Compressed formats have variable size
    }
}

/**
 * @brief Check if audio format is compressed
 */
bool is_compressed_format(AudioFormat format) {
    switch (format) {
        case AudioFormat::ADPCM_4BIT:
        case AudioFormat::ADPCM_IMA:
        case AudioFormat::MP3:
        case AudioFormat::OGG_VORBIS:
        case AudioFormat::AAC:
        case AudioFormat::FLAC:
        case AudioFormat::DREAMCAST_ADPCM:
        case AudioFormat::PSP_ATRAC3:
            return true;
        default:
            return false;
    }
}

/**
 * @brief Check if audio format is floating point
 */
bool is_floating_point_format(AudioFormat format) {
    switch (format) {
        case AudioFormat::PCM_F32:
        case AudioFormat::PCM_F64:
            return true;
        default:
            return false;
    }
}

/**
 * @brief Calculate buffer size in bytes
 */
size_t calculate_buffer_size(size_t frame_count, uint16_t channels, AudioFormat format) {
    uint32_t bytes_per_sample = get_bytes_per_sample(format);
    if (bytes_per_sample == 0) {
        // For compressed formats, return approximate size
        return frame_count * channels * 2; // Assume 16-bit equivalent
    }
    return frame_count * channels * bytes_per_sample;
}

/**
 * @brief Create default audio buffer descriptor
 */
AudioBufferDescriptor make_buffer_descriptor(size_t frame_count,
                                            uint32_t sample_rate,
                                            uint16_t channels,
                                            AudioFormat format) {
    AudioBufferDescriptor descriptor{};
    descriptor.frame_count = frame_count;
    descriptor.sample_rate = sample_rate;
    descriptor.channels = channels;
    descriptor.format = format;
    descriptor.usage = AudioBufferUsage::Static;
    descriptor.low_latency_hint = false;
    descriptor.debug_name = "";
    return descriptor;
}

/**
 * @brief Create default audio stream descriptor
 */
AudioStreamDescriptor make_stream_descriptor(std::string_view source_path, float volume) {
    AudioStreamDescriptor descriptor{};
    descriptor.source_path = source_path;
    descriptor.preferred_format = AudioFormat::PCM_F32;
    descriptor.preferred_sample_rate = 44100;
    descriptor.preferred_channels = 2;
    descriptor.preload_hint = false;
    descriptor.loop_enabled = false;
    descriptor.volume = volume;
    descriptor.debug_name = "";
    return descriptor;
}

/**
 * @brief Create default audio device descriptor
 */
AudioDeviceDescriptor make_device_descriptor(uint32_t sample_rate,
                                            uint16_t channels,
                                            uint32_t buffer_size) {
    AudioDeviceDescriptor descriptor{};
    descriptor.device_name = "";
    descriptor.sample_rate = sample_rate;
    descriptor.channels = channels;
    descriptor.buffer_size_frames = buffer_size;
    descriptor.format = AudioFormat::PCM_F32;
    descriptor.low_latency_mode = false;
    descriptor.full_duplex = false;
    return descriptor;
}

/**
 * @brief Create default audio listener
 */
AudioListener make_default_listener() {
    AudioListener listener{};
    listener.position = Vector3{0.0f, 0.0f, 0.0f};
    listener.forward = Vector3{0.0f, 0.0f, -1.0f};
    listener.up = Vector3{0.0f, 1.0f, 0.0f};
    listener.velocity = Vector3{0.0f, 0.0f, 0.0f};
    listener.gain = 1.0f;
    return listener;
}

/**
 * @brief Create default 3D audio source
 */
Audio3DSource make_3d_source(const Vector3& position) {
    Audio3DSource source{};
    source.position = position;
    source.velocity = Vector3{0.0f, 0.0f, 0.0f};
    source.min_distance = 1.0f;
    source.max_distance = 100.0f;
    source.rolloff_factor = 1.0f;
    source.cone_inner_angle = 360.0f;
    source.cone_outer_angle = 360.0f;
    source.cone_outer_gain = 1.0f;
    source.cone_direction = Vector3{0.0f, 0.0f, -1.0f};
    return source;
}

/**
 * @brief Convert samples between formats
 */
void convert_samples(const void* input_data,
                    AudioFormat input_format,
                    void* output_data,
                    AudioFormat output_format,
                    size_t sample_count) {
    if (input_format == output_format) {
        // Same format, just copy
        size_t bytes_per_sample = get_bytes_per_sample(input_format);
        std::memcpy(output_data, input_data, sample_count * bytes_per_sample);
        return;
    }
    
    // Convert through float as intermediate format
    std::vector<float> float_samples(sample_count);
    
    // Convert input to float
    const uint8_t* input_u8 = static_cast<const uint8_t*>(input_data);
    const int16_t* input_s16 = static_cast<const int16_t*>(input_data);
    const int32_t* input_s32 = static_cast<const int32_t*>(input_data);
    const float* input_f32 = static_cast<const float*>(input_data);
    const double* input_f64 = static_cast<const double*>(input_data);
    
    for (size_t i = 0; i < sample_count; ++i) {
        switch (input_format) {
            case AudioFormat::PCM_U8:
                float_samples[i] = (input_u8[i] - 128) / 128.0f;
                break;
            case AudioFormat::PCM_S16:
                float_samples[i] = input_s16[i] / 32768.0f;
                break;
            case AudioFormat::PCM_S24:
                // Reconstruct 24-bit value
                {
                    const uint8_t* input_bytes = static_cast<const uint8_t*>(input_data);
                    int32_t value = (input_bytes[i*3] << 8) | (input_bytes[i*3+1] << 16) | (input_bytes[i*3+2] << 24);
                    value >>= 8; // Sign extend
                    float_samples[i] = value / 8388608.0f;
                }
                break;
            case AudioFormat::PCM_S32:
                float_samples[i] = input_s32[i] / 2147483648.0f;
                break;
            case AudioFormat::PCM_F32:
                float_samples[i] = input_f32[i];
                break;
            case AudioFormat::PCM_F64:
                float_samples[i] = static_cast<float>(input_f64[i]);
                break;
            default:
                float_samples[i] = 0.0f; // Unsupported format
                break;
        }
    }
    
    // Convert float to output format
    uint8_t* output_u8 = static_cast<uint8_t*>(output_data);
    int16_t* output_s16 = static_cast<int16_t*>(output_data);
    int32_t* output_s32 = static_cast<int32_t*>(output_data);
    float* output_f32 = static_cast<float*>(output_data);
    double* output_f64 = static_cast<double*>(output_data);
    
    for (size_t i = 0; i < sample_count; ++i) {
        float sample = std::clamp(float_samples[i], -1.0f, 1.0f);
        
        switch (output_format) {
            case AudioFormat::PCM_U8:
                output_u8[i] = static_cast<uint8_t>((sample * 128.0f) + 128);
                break;
            case AudioFormat::PCM_S16:
                output_s16[i] = static_cast<int16_t>(sample * 32767.0f);
                break;
            case AudioFormat::PCM_S24:
                {
                    int32_t value = static_cast<int32_t>(sample * 8388607.0f);
                    uint8_t* output_bytes = static_cast<uint8_t*>(output_data);
                    output_bytes[i*3] = (value >> 8) & 0xFF;
                    output_bytes[i*3+1] = (value >> 16) & 0xFF;
                    output_bytes[i*3+2] = (value >> 24) & 0xFF;
                }
                break;
            case AudioFormat::PCM_S32:
                output_s32[i] = static_cast<int32_t>(sample * 2147483647.0f);
                break;
            case AudioFormat::PCM_F32:
                output_f32[i] = sample;
                break;
            case AudioFormat::PCM_F64:
                output_f64[i] = static_cast<double>(sample);
                break;
            default:
                // Unsupported format
                break;
        }
    }
}

/**
 * @brief Interleave planar audio channels
 */
void interleave_channels(const void* const* planar_data,
                        uint16_t channel_count,
                        size_t frame_count,
                        void* interleaved_data,
                        uint32_t bytes_per_sample) {
    const uint8_t* const* planar_bytes = reinterpret_cast<const uint8_t* const*>(planar_data);
    uint8_t* interleaved_bytes = static_cast<uint8_t*>(interleaved_data);
    
    for (size_t frame = 0; frame < frame_count; ++frame) {
        for (uint16_t channel = 0; channel < channel_count; ++channel) {
            size_t src_offset = frame * bytes_per_sample;
            size_t dst_offset = (frame * channel_count + channel) * bytes_per_sample;
            
            std::memcpy(interleaved_bytes + dst_offset,
                       planar_bytes[channel] + src_offset,
                       bytes_per_sample);
        }
    }
}

/**
 * @brief Deinterleave interleaved audio channels
 */
void deinterleave_channels(const void* interleaved_data,
                          uint16_t channel_count,
                          size_t frame_count,
                          void* const* planar_data,
                          uint32_t bytes_per_sample) {
    const uint8_t* interleaved_bytes = static_cast<const uint8_t*>(interleaved_data);
    uint8_t* const* planar_bytes = reinterpret_cast<uint8_t* const*>(planar_data);
    
    for (size_t frame = 0; frame < frame_count; ++frame) {
        for (uint16_t channel = 0; channel < channel_count; ++channel) {
            size_t src_offset = (frame * channel_count + channel) * bytes_per_sample;
            size_t dst_offset = frame * bytes_per_sample;
            
            std::memcpy(planar_bytes[channel] + dst_offset,
                       interleaved_bytes + src_offset,
                       bytes_per_sample);
        }
    }
}

/**
 * @brief Apply volume to audio samples
 */
void apply_volume(void* samples,
                 size_t sample_count,
                 AudioFormat format,
                 float volume) {
    switch (format) {
        case AudioFormat::PCM_S16:
            {
                int16_t* s16_samples = static_cast<int16_t*>(samples);
                for (size_t i = 0; i < sample_count; ++i) {
                    s16_samples[i] = static_cast<int16_t>(s16_samples[i] * volume);
                }
            }
            break;
        case AudioFormat::PCM_F32:
            {
                float* f32_samples = static_cast<float*>(samples);
                for (size_t i = 0; i < sample_count; ++i) {
                    f32_samples[i] *= volume;
                }
            }
            break;
        case AudioFormat::PCM_F64:
            {
                double* f64_samples = static_cast<double*>(samples);
                for (size_t i = 0; i < sample_count; ++i) {
                    f64_samples[i] *= volume;
                }
            }
            break;
        default:
            HAL_LOG_MESSAGE(flight::hal::LogLevel::Warning, 
                ("Volume adjustment not implemented for format: " + std::string(to_string(format))).c_str());
            break;
    }
}

/**
 * @brief Mix two audio buffers together
 */
void mix_buffers(const void* source1,
                const void* source2,
                void* destination,
                size_t sample_count,
                AudioFormat format,
                float mix_ratio) {
    switch (format) {
        case AudioFormat::PCM_S16:
            {
                const int16_t* s1 = static_cast<const int16_t*>(source1);
                const int16_t* s2 = static_cast<const int16_t*>(source2);
                int16_t* dest = static_cast<int16_t*>(destination);
                
                for (size_t i = 0; i < sample_count; ++i) {
                    float mixed = s1[i] * (1.0f - mix_ratio) + s2[i] * mix_ratio;
                    dest[i] = static_cast<int16_t>(std::clamp(mixed, -32768.0f, 32767.0f));
                }
            }
            break;
        case AudioFormat::PCM_F32:
            {
                const float* s1 = static_cast<const float*>(source1);
                const float* s2 = static_cast<const float*>(source2);
                float* dest = static_cast<float*>(destination);
                
                for (size_t i = 0; i < sample_count; ++i) {
                    dest[i] = s1[i] * (1.0f - mix_ratio) + s2[i] * mix_ratio;
                }
            }
            break;
        case AudioFormat::PCM_F64:
            {
                const double* s1 = static_cast<const double*>(source1);
                const double* s2 = static_cast<const double*>(source2);
                double* dest = static_cast<double*>(destination);
                
                for (size_t i = 0; i < sample_count; ++i) {
                    dest[i] = s1[i] * (1.0 - mix_ratio) + s2[i] * mix_ratio;
                }
            }
            break;
        default:
            HAL_LOG_MESSAGE(flight::hal::LogLevel::Warning, 
                ("Buffer mixing not implemented for format: " + std::string(to_string(format))).c_str());
            break;
    }
}

} // namespace flight::hal::audio
