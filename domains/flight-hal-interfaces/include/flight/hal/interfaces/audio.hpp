/**
 * @file audio.hpp
 * @brief Flight HAL Elite Audio Interface
 * 
 * Unified audio interface spanning from Dreamcast AICA (2MB audio RAM, ADPCM compression)
 * to modern systems (Web Audio API, Core Audio, DirectSound). Provides streaming audio,
 * low-latency callbacks, 3D spatial audio, and hardware-accelerated processing.
 */

#pragma once

#include "../core/driver_registry.hpp"
#include "../core/hal_result.hpp"
#include "../core/hal_capabilities.hpp"
#include "memory.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>
#include <string_view>
#include <memory>
#include <array>
#include <map>

namespace flight::hal {

/**
 * @brief Audio-specific capability enumeration
 * 
 * Runtime detection of audio features for graceful degradation across platforms
 * from Dreamcast AICA to modern audio systems.
 */
enum class AudioCapability : uint32_t {
    // Core playback capabilities
    PCMPlayback = 1 << 0,              ///< Basic PCM audio playback
    StreamingPlayback = 1 << 1,        ///< Streaming audio from files/network
    CallbackProcessing = 1 << 2,       ///< Low-latency callback-based processing
    MultiChannel = 1 << 3,             ///< Multi-channel audio support
    
    // Mixing capabilities
    HardwareMixing = 1 << 4,           ///< Hardware-accelerated mixing
    SoftwareMixing = 1 << 5,           ///< Software mixing fallback
    VolumeControl = 1 << 6,            ///< Per-source volume control
    PanControl = 1 << 7,               ///< Stereo panning control
    
    // Format support
    PCMFormats = 1 << 8,               ///< Multiple PCM bit depths/rates
    CompressedFormats = 1 << 9,        ///< Compressed audio formats (MP3, OGG)
    ADPCMDecoding = 1 << 10,           ///< ADPCM compression (Dreamcast)
    RealtimeDecoding = 1 << 11,        ///< Real-time format conversion
    
    // Processing capabilities
    HardwareDSP = 1 << 12,             ///< Hardware DSP effects
    SoftwareEffects = 1 << 13,         ///< Software audio effects
    ConvolutionReverb = 1 << 14,       ///< Convolution-based reverb
    ParametricEQ = 1 << 15,            ///< Parametric equalizer
    
    // 3D spatial audio
    SpatialAudio3D = 1 << 16,          ///< 3D positional audio
    DistanceAttenuation = 1 << 17,     ///< Distance-based volume attenuation
    DopplerEffect = 1 << 18,           ///< Doppler shift simulation
    EnvironmentalAudio = 1 << 19,      ///< Environmental audio effects
    
    // Advanced features
    LowLatencyMode = 1 << 20,          ///< Ultra-low latency processing
    VariableBufferSize = 1 << 21,      ///< Runtime buffer size changes
    MultiThreadedProcessing = 1 << 22, ///< Multi-threaded audio processing
    GPUAcceleration = 1 << 23,         ///< GPU-accelerated audio processing
    
    // Platform-specific optimizations
    StoreQueueOptimization = 1 << 24,  ///< Dreamcast store queue optimization
    AudioWorklets = 1 << 25,           ///< Web Audio worklets
    AudioUnitSupport = 1 << 26,        ///< macOS Audio Units
    DirectSoundSupport = 1 << 27,      ///< DirectSound optimization
    
    // Recording capabilities
    AudioRecording = 1 << 28,          ///< Audio input/recording
    RealtimeMonitoring = 1 << 29,      ///< Real-time input monitoring
    FullDuplexAudio = 1 << 30,         ///< Simultaneous input/output
    EchoCancellation = 1U << 31        ///< Echo cancellation
};

/**
 * @brief Audio resource types
 */
enum class AudioResourceType : uint8_t {
    AudioBuffer = 0,    ///< PCM audio data buffer
    StreamBuffer,       ///< Streaming audio buffer
    MixerChannel,       ///< Hardware/software mixer channel
    ProcessorUnit,      ///< DSP/effect processing unit
    AudioDevice,        ///< Audio input/output device
    AudioContext,       ///< Audio processing context
    SpatialListener,    ///< 3D audio listener
    SpatialSource,      ///< 3D audio source
    EffectChain,        ///< Audio effect processing chain
    AudioStream         ///< High-level audio stream object
};

/**
 * @brief Audio formats supporting various hardware capabilities
 */
enum class AudioFormat : uint16_t {
    // PCM formats (all platforms)
    PCM_U8 = 0,         ///< 8-bit unsigned PCM
    PCM_S16,            ///< 16-bit signed PCM (most common)
    PCM_S24,            ///< 24-bit signed PCM
    PCM_S32,            ///< 32-bit signed PCM
    PCM_F32,            ///< 32-bit float PCM
    PCM_F64,            ///< 64-bit double PCM
    
    // Compressed formats (platform-dependent)
    ADPCM_4BIT = 100,   ///< 4-bit ADPCM (Dreamcast AICA)
    ADPCM_IMA,          ///< IMA ADPCM compression
    MP3,                ///< MP3 audio (software decode)
    OGG_VORBIS,         ///< Ogg Vorbis (software decode)
    AAC,                ///< AAC audio (hardware decode on modern)
    FLAC,               ///< FLAC lossless compression
    
    // Special formats
    RAW_INTERLEAVED = 200, ///< Raw interleaved samples
    RAW_PLANAR,         ///< Raw planar samples
    
    // Platform-specific
    DREAMCAST_ADPCM = 300, ///< Dreamcast-specific ADPCM variant
    PSP_ATRAC3,         ///< PSP ATRAC3 compression
    WEB_AUDIO_BUFFER    ///< Web Audio API buffer format
};

/**
 * @brief Buffer usage patterns for optimization
 */
enum class AudioBufferUsage : uint8_t {
    Static = 0,         ///< Set once, played multiple times
    Streaming,          ///< Continuously updated streaming buffer
    Callback,           ///< Updated via real-time callbacks
    Effect,             ///< Used for audio effect processing
    Temporary,          ///< Short-lived temporary buffer
    Recording           ///< Used for audio input/recording
};

/**
 * @brief Audio effect types
 */
enum class AudioEffectType : uint8_t {
    None = 0,           ///< No effect (bypass)
    
    // Basic effects
    Gain,               ///< Volume/gain adjustment
    Pan,                ///< Stereo panning
    Delay,              ///< Simple delay/echo
    Reverb,             ///< Reverberation
    
    // Filtering effects
    LowPassFilter,      ///< Low-pass filter
    HighPassFilter,     ///< High-pass filter
    BandPassFilter,     ///< Band-pass filter
    ParametricEQ,       ///< Parametric equalizer
    
    // Dynamic effects
    Compressor,         ///< Dynamic range compressor
    Limiter,            ///< Peak limiter
    Gate,               ///< Noise gate
    Expander,           ///< Dynamic range expander
    
    // Modulation effects
    Chorus,             ///< Chorus effect
    Flanger,            ///< Flanger effect
    Phaser,             ///< Phaser effect
    Tremolo,            ///< Tremolo (amplitude modulation)
    
    // Distortion effects
    Overdrive,          ///< Overdrive distortion
    Distortion,         ///< Hard distortion
    BitCrusher,         ///< Bit-crushing effect
    
    // Spatial effects
    Spatialization,     ///< 3D spatialization
    ConvolutionReverb,  ///< Convolution-based reverb
    Binaural,           ///< Binaural audio processing
    
    // Platform-specific
    DreamcastDSP,       ///< Dreamcast AICA DSP effects
    WebAudioWorklet,    ///< Web Audio worklet processor
    CoreAudioUnit       ///< macOS Audio Unit
};

/**
 * @brief Distance attenuation models for 3D audio
 */
enum class DistanceModel : uint8_t {
    None = 0,           ///< No distance attenuation
    Linear,             ///< Linear distance attenuation
    Inverse,            ///< Inverse distance attenuation
    Exponential         ///< Exponential distance attenuation
};

// Forward declarations
class IAudioBuffer;
class IAudioStream;
class IAudioProcessor;
class IAudioDevice;
class ISpatialAudio;
class IAudioEffect;

/**
 * @brief 3D vector for spatial audio calculations
 */
struct Vector3 {
    float x, y, z;
    
    Vector3(float x = 0.0f, float y = 0.0f, float z = 0.0f) : x(x), y(y), z(z) {}
    
    Vector3 operator+(const Vector3& other) const { return {x + other.x, y + other.y, z + other.z}; }
    Vector3 operator-(const Vector3& other) const { return {x - other.x, y - other.y, z - other.z}; }
    Vector3 operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar}; }
    
    float length() const { return std::sqrt(x*x + y*y + z*z); }
    Vector3 normalize() const { float len = length(); return len > 0.0f ? *this * (1.0f/len) : Vector3{}; }
};

/**
 * @brief Audio resource handle
 */
struct AudioResourceHandle {
    uint32_t id;                    ///< Unique resource identifier
    AudioResourceType type;         ///< Resource type
    uint32_t generation;            ///< Generation counter for validation
    
    bool is_valid() const { return id != 0; }
    void invalidate() { id = 0; generation = 0; }
};

/**
 * @brief Audio buffer creation parameters
 */
struct AudioBufferDescriptor {
    size_t frame_count;             ///< Number of audio frames
    uint32_t sample_rate;           ///< Sample rate in Hz
    uint16_t channels;              ///< Number of audio channels
    AudioFormat format;             ///< Audio sample format
    AudioBufferUsage usage;         ///< Buffer usage pattern
    bool low_latency_hint;          ///< Hint for low-latency optimization
    std::string_view debug_name;    ///< Debug name for profiling
};

/**
 * @brief Audio stream creation parameters
 */
struct AudioStreamDescriptor {
    std::string_view source_path;   ///< File path or URL for stream source
    AudioFormat preferred_format;   ///< Preferred output format
    uint32_t preferred_sample_rate; ///< Preferred sample rate
    uint16_t preferred_channels;    ///< Preferred channel count
    bool preload_hint;              ///< Hint to preload stream data
    bool loop_enabled;              ///< Enable seamless looping
    float volume;                   ///< Initial volume (0.0-1.0)
    std::string_view debug_name;    ///< Debug name for profiling
};

/**
 * @brief Audio device configuration
 */
struct AudioDeviceDescriptor {
    std::string_view device_name;   ///< Specific device name (empty for default)
    uint32_t sample_rate;           ///< Desired sample rate
    uint16_t channels;              ///< Desired channel count
    uint32_t buffer_size_frames;    ///< Buffer size in frames
    AudioFormat format;             ///< Desired sample format
    bool low_latency_mode;          ///< Enable low-latency mode
    bool full_duplex;               ///< Enable input and output
};

/**
 * @brief 3D audio listener configuration
 */
struct AudioListener {
    Vector3 position;               ///< Listener position in 3D space
    Vector3 forward;                ///< Forward direction vector
    Vector3 up;                     ///< Up direction vector
    Vector3 velocity;               ///< Listener velocity (for Doppler)
    float gain;                     ///< Master gain multiplier
};

/**
 * @brief 3D audio source configuration
 */
struct Audio3DSource {
    Vector3 position;               ///< Source position in 3D space
    Vector3 velocity;               ///< Source velocity (for Doppler)
    float min_distance;             ///< Distance where attenuation starts
    float max_distance;             ///< Distance where sound becomes inaudible
    float rolloff_factor;           ///< Rate of distance attenuation
    float cone_inner_angle;         ///< Inner cone angle (degrees)
    float cone_outer_angle;         ///< Outer cone angle (degrees)
    float cone_outer_gain;          ///< Gain outside the outer cone
    Vector3 cone_direction;         ///< Direction of the sound cone
};

/**
 * @brief Audio effect parameters
 */
struct AudioEffectParams {
    AudioEffectType type;           ///< Type of effect to apply
    std::map<std::string, float> parameters; ///< Effect-specific parameters
    bool bypass;                    ///< Bypass the effect
    float wet_level;                ///< Wet signal level (0.0-1.0)
    float dry_level;                ///< Dry signal level (0.0-1.0)
};

/**
 * @brief Audio device information
 */
struct AudioDeviceInfo {
    std::string device_name;        ///< Device name
    std::string driver_name;        ///< Audio driver name
    uint32_t capability_mask;       ///< Supported AudioCapability bitmask
    
    // Supported formats
    std::vector<AudioFormat> supported_formats;
    std::vector<uint32_t> supported_sample_rates;
    std::vector<uint16_t> supported_channel_counts;
    
    // Device capabilities
    uint32_t min_buffer_size;       ///< Minimum buffer size in frames
    uint32_t max_buffer_size;       ///< Maximum buffer size in frames
    uint32_t preferred_buffer_size; ///< Recommended buffer size
    
    // Latency information
    uint32_t input_latency_frames;  ///< Input latency in frames
    uint32_t output_latency_frames; ///< Output latency in frames
    
    // Hardware features
    bool has_hardware_mixing;       ///< Hardware mixing support
    bool has_hardware_effects;      ///< Hardware effects support
    bool has_3d_audio;              ///< 3D audio support
    uint32_t max_sources;           ///< Maximum simultaneous audio sources
    
    // Memory constraints (Dreamcast: 2MB audio RAM)
    size_t total_audio_memory;      ///< Total audio memory (0 if unlimited)
    size_t available_audio_memory;  ///< Currently available audio memory
};

/**
 * @brief Audio performance statistics
 */
struct AudioStats {
    uint64_t frames_processed;      ///< Total audio frames processed
    uint64_t buffer_underruns;      ///< Audio buffer underrun count
    uint64_t buffer_overruns;       ///< Audio buffer overrun count
    uint64_t callback_count;        ///< Number of audio callbacks
    uint64_t dropped_samples;       ///< Dropped samples due to overload
    
    // Timing statistics
    double average_callback_time_us; ///< Average callback time in microseconds
    double peak_callback_time_us;   ///< Peak callback time in microseconds
    double cpu_load_percentage;     ///< CPU load percentage (0-100)
    
    // Memory usage
    size_t audio_memory_used;       ///< Audio memory currently in use
    size_t peak_audio_memory_used;  ///< Peak audio memory usage
    uint32_t active_sources;        ///< Currently active audio sources
    uint32_t active_effects;        ///< Currently active audio effects
};

/**
 * @brief Low-latency audio callback function type
 * 
 * Called from real-time audio thread. Must be lock-free and complete
 * processing within the buffer duration to avoid dropouts.
 */
using AudioCallback = std::function<void(
    float* input_buffer,    ///< Input audio samples (may be nullptr if no input)
    float* output_buffer,   ///< Output audio samples to fill
    uint32_t frame_count,   ///< Number of frames to process
    double sample_time,     ///< Sample-accurate timestamp
    uint32_t channels       ///< Number of audio channels
)>;

/**
 * @brief Audio stream completion callback
 */
using StreamCompletionCallback = std::function<void(AudioResourceHandle stream, bool completed_successfully)>;

/**
 * @brief Audio device event callback
 */
using AudioDeviceEventCallback = std::function<void(const std::string& device_name, bool device_added)>;

/**
 * @brief Core audio buffer interface
 * 
 * Represents a buffer of audio samples that can be played, recorded to,
 * or used for audio processing operations.
 */
class IAudioBuffer {
public:
    virtual ~IAudioBuffer() = default;
    
    /**
     * @brief Get buffer descriptor
     * @return Buffer configuration information
     */
    virtual const AudioBufferDescriptor& get_descriptor() const = 0;
    
    /**
     * @brief Get audio resource handle
     * @return Handle for this buffer
     */
    virtual AudioResourceHandle get_handle() const = 0;
    
    /**
     * @brief Get buffer size in bytes
     * @return Total buffer size in bytes
     */
    virtual size_t get_size_bytes() const = 0;
    
    /**
     * @brief Get buffer size in frames
     * @return Number of audio frames in buffer
     */
    virtual size_t get_frame_count() const = 0;
    
    /**
     * @brief Upload audio data to buffer
     * @param data Audio sample data
     * @param frame_count Number of frames to upload
     * @param offset_frames Offset in buffer to start upload
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> upload_data(const void* data, size_t frame_count, size_t offset_frames = 0) = 0;
    
    /**
     * @brief Download audio data from buffer
     * @param data Buffer to receive audio data
     * @param frame_count Number of frames to download
     * @param offset_frames Offset in buffer to start download
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> download_data(void* data, size_t frame_count, size_t offset_frames = 0) const = 0;
    
    /**
     * @brief Clear buffer contents
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> clear() = 0;
    
    /**
     * @brief Check if buffer is valid and ready to use
     * @return true if buffer is valid
     */
    virtual bool is_valid() const = 0;
};

/**
 * @brief Audio stream interface
 * 
 * High-level interface for playing audio files or streaming audio data.
 * Handles format conversion, buffering, and playback control.
 */
class IAudioStream {
public:
    virtual ~IAudioStream() = default;
    
    /**
     * @brief Get stream descriptor
     * @return Stream configuration information
     */
    virtual const AudioStreamDescriptor& get_descriptor() const = 0;
    
    /**
     * @brief Get audio resource handle
     * @return Handle for this stream
     */
    virtual AudioResourceHandle get_handle() const = 0;
    
    /**
     * @brief Load audio from file
     * @param file_path Path to audio file
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> load_from_file(std::string_view file_path) = 0;
    
    /**
     * @brief Load audio from memory
     * @param data Audio file data in memory
     * @param size Size of audio data
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> load_from_memory(const void* data, size_t size) = 0;
    
    /**
     * @brief Start playing the stream
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> play() = 0;
    
    /**
     * @brief Pause the stream
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> pause() = 0;
    
    /**
     * @brief Stop the stream and reset position
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> stop() = 0;
    
    /**
     * @brief Set stream volume
     * @param volume Volume level (0.0 = silent, 1.0 = full volume)
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_volume(float volume) = 0;
    
    /**
     * @brief Set stream playback position
     * @param position_seconds Position in seconds
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_position(double position_seconds) = 0;
    
    /**
     * @brief Set 3D position for spatial audio
     * @param position Position in 3D space
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_position_3d(const Vector3& position) = 0;
    
    /**
     * @brief Enable or disable looping
     * @param loop_enabled Whether to loop the stream
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_loop_enabled(bool loop_enabled) = 0;
    
    /**
     * @brief Get current playback position
     * @return HALResult containing current position in seconds
     */
    virtual HALResult<double> get_position() const = 0;
    
    /**
     * @brief Get stream duration
     * @return HALResult containing duration in seconds
     */
    virtual HALResult<double> get_duration() const = 0;
    
    /**
     * @brief Check if stream is currently playing
     * @return true if stream is playing
     */
    virtual bool is_playing() const = 0;
    
    /**
     * @brief Check if stream has finished playing
     * @return true if stream has reached the end
     */
    virtual bool is_finished() const = 0;
    
    /**
     * @brief Set stream completion callback
     * @param callback Callback to invoke when stream completes
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_completion_callback(StreamCompletionCallback callback) = 0;
};

/**
 * @brief Audio processor interface
 * 
 * Low-latency audio processing interface using callbacks. Suitable for
 * real-time audio effects, synthesizers, and interactive audio applications.
 */
class IAudioProcessor {
public:
    virtual ~IAudioProcessor() = default;
    
    /**
     * @brief Get audio resource handle
     * @return Handle for this processor
     */
    virtual AudioResourceHandle get_handle() const = 0;
    
    /**
     * @brief Set audio processing callback
     * @param callback Callback function for audio processing
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_callback(AudioCallback callback) = 0;
    
    /**
     * @brief Start audio processing
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> start_processing() = 0;
    
    /**
     * @brief Stop audio processing
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> stop_processing() = 0;
    
    /**
     * @brief Set buffer size for processing
     * @param frames Buffer size in frames
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_buffer_size(uint32_t frames) = 0;
    
    /**
     * @brief Set processing sample rate
     * @param sample_rate Sample rate in Hz
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_sample_rate(uint32_t sample_rate) = 0;
    
    /**
     * @brief Set number of processing channels
     * @param channels Number of audio channels
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_channel_count(uint16_t channels) = 0;
    
    /**
     * @brief Get current buffer size
     * @return Current buffer size in frames
     */
    virtual uint32_t get_buffer_size() const = 0;
    
    /**
     * @brief Get current sample rate
     * @return Current sample rate in Hz
     */
    virtual uint32_t get_sample_rate() const = 0;
    
    /**
     * @brief Get current channel count
     * @return Current number of channels
     */
    virtual uint16_t get_channel_count() const = 0;
    
    /**
     * @brief Check if processor is currently running
     * @return true if processing is active
     */
    virtual bool is_processing() const = 0;
    
    /**
     * @brief Get processor latency
     * @return HALResult containing latency in frames
     */
    virtual HALResult<uint32_t> get_latency_frames() const = 0;
};

/**
 * @brief Spatial audio interface
 * 
 * Provides 3D positional audio capabilities with distance attenuation,
 * Doppler effects, and environmental audio simulation.
 */
class ISpatialAudio {
public:
    virtual ~ISpatialAudio() = default;
    
    /**
     * @brief Set 3D audio listener configuration
     * @param listener Listener parameters
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_listener(const AudioListener& listener) = 0;
    
    /**
     * @brief Configure audio source for 3D positioning
     * @param source Audio source handle
     * @param params 3D audio parameters
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_source_3d(AudioResourceHandle source, const Audio3DSource& params) = 0;
    
    /**
     * @brief Set global Doppler factor
     * @param factor Doppler effect strength (1.0 = realistic, 0.0 = disabled)
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_doppler_factor(float factor) = 0;
    
    /**
     * @brief Set distance attenuation model
     * @param model Distance model to use
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_distance_model(DistanceModel model) = 0;
    
    /**
     * @brief Set speed of sound for Doppler calculations
     * @param speed Speed in units per second
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_speed_of_sound(float speed) = 0;
    
    /**
     * @brief Get current listener configuration
     * @return Current listener settings
     */
    virtual const AudioListener& get_listener() const = 0;
    
    /**
     * @brief Check if 3D audio is supported
     * @return true if 3D audio capabilities are available
     */
    virtual bool is_3d_audio_supported() const = 0;
};

/**
 * @brief Audio effect interface
 * 
 * Represents an audio effect that can be applied to audio sources
 * or inserted into effect chains for processing.
 */
class IAudioEffect {
public:
    virtual ~IAudioEffect() = default;
    
    /**
     * @brief Get effect resource handle
     * @return Handle for this effect
     */
    virtual AudioResourceHandle get_handle() const = 0;
    
    /**
     * @brief Get effect type
     * @return Type of this effect
     */
    virtual AudioEffectType get_effect_type() const = 0;
    
    /**
     * @brief Set effect parameters
     * @param params Effect configuration parameters
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_parameters(const AudioEffectParams& params) = 0;
    
    /**
     * @brief Get current effect parameters
     * @return Current effect configuration
     */
    virtual const AudioEffectParams& get_parameters() const = 0;
    
    /**
     * @brief Set specific effect parameter
     * @param parameter_name Name of parameter to set
     * @param value Parameter value
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_parameter(std::string_view parameter_name, float value) = 0;
    
    /**
     * @brief Get specific effect parameter
     * @param parameter_name Name of parameter to get
     * @return HALResult containing parameter value
     */
    virtual HALResult<float> get_parameter(std::string_view parameter_name) const = 0;
    
    /**
     * @brief Enable or disable the effect
     * @param enabled Whether effect should be active
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_enabled(bool enabled) = 0;
    
    /**
     * @brief Check if effect is currently enabled
     * @return true if effect is enabled
     */
    virtual bool is_enabled() const = 0;
    
    /**
     * @brief Reset effect to default state
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> reset() = 0;
};

/**
 * @brief Enhanced audio interface
 * 
 * Comprehensive audio system supporting streaming, low-latency callbacks,
 * 3D spatial audio, and hardware-specific optimizations across platforms
 * from Dreamcast AICA to modern audio systems.
 */
class IAudioInterface : public IHALInterface, public ICapabilityProvider {
public:
    virtual ~IAudioInterface() = default;
    
    // === System Management ===
    
    /**
     * @brief Initialize audio system
     * @param device_descriptor Audio device configuration
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> initialize(const AudioDeviceDescriptor& device_descriptor = {}) = 0;
    
    /**
     * @brief Shutdown audio system
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> shutdown() = 0;
    
    /**
     * @brief Get audio device information
     * @return Device capabilities and information
     */
    virtual const AudioDeviceInfo& get_device_info() const = 0;
    
    /**
     * @brief Check if audio system is initialized
     * @return true if initialized and ready
     */
    virtual bool is_initialized() const = 0;
    
    /**
     * @brief Get list of available audio devices
     * @return HALResult containing vector of device information
     */
    virtual HALResult<std::vector<AudioDeviceInfo>> get_available_devices() const = 0;
    
    // === Resource Creation ===
    
    /**
     * @brief Create audio buffer
     * @param descriptor Buffer creation parameters
     * @param initial_data Initial audio data (optional)
     * @return HALResult containing buffer pointer on success
     */
    virtual HALResult<std::unique_ptr<IAudioBuffer>> create_buffer(const AudioBufferDescriptor& descriptor,
                                                                  const void* initial_data = nullptr) = 0;
    
    /**
     * @brief Create audio stream
     * @param descriptor Stream creation parameters
     * @return HALResult containing stream pointer on success
     */
    virtual HALResult<std::unique_ptr<IAudioStream>> create_stream(const AudioStreamDescriptor& descriptor) = 0;
    
    /**
     * @brief Create audio processor
     * @param sample_rate Processing sample rate
     * @param channels Number of channels
     * @param buffer_size Buffer size in frames
     * @return HALResult containing processor pointer on success
     */
    virtual HALResult<std::unique_ptr<IAudioProcessor>> create_processor(uint32_t sample_rate,
                                                                        uint16_t channels,
                                                                        uint32_t buffer_size) = 0;
    
    /**
     * @brief Create audio effect
     * @param effect_type Type of effect to create
     * @param params Effect parameters
     * @return HALResult containing effect pointer on success
     */
    virtual HALResult<std::unique_ptr<IAudioEffect>> create_effect(AudioEffectType effect_type,
                                                                  const AudioEffectParams& params = {}) = 0;
    
    // === Resource Management ===
    
    /**
     * @brief Get spatial audio interface
     * @return Pointer to spatial audio interface (nullptr if not supported)
     */
    virtual ISpatialAudio* get_spatial_audio() = 0;
    
    /**
     * @brief Set master volume
     * @param volume Master volume level (0.0-1.0)
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_master_volume(float volume) = 0;
    
    /**
     * @brief Get master volume
     * @return Current master volume level
     */
    virtual float get_master_volume() const = 0;
    
    /**
     * @brief Set device event callback
     * @param callback Callback for device events
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_device_event_callback(AudioDeviceEventCallback callback) = 0;
    
    // === Format Conversion ===
    
    /**
     * @brief Convert audio format
     * @param input_data Input audio data
     * @param input_frames Number of input frames
     * @param input_format Input audio format
     * @param input_channels Input channel count
     * @param output_data Output buffer for converted data
     * @param output_frames Number of output frames
     * @param output_format Output audio format
     * @param output_channels Output channel count
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> convert_format(const void* input_data,
                                          size_t input_frames,
                                          AudioFormat input_format,
                                          uint16_t input_channels,
                                          void* output_data,
                                          size_t output_frames,
                                          AudioFormat output_format,
                                          uint16_t output_channels) = 0;
    
    /**
     * @brief Get bytes per sample for audio format
     * @param format Audio format to query
     * @return Number of bytes per sample
     */
    virtual uint32_t get_bytes_per_sample(AudioFormat format) const = 0;
    
    // === Capability Queries ===
    
    /**
     * @brief Check if audio capability is supported
     * @param capability Audio capability to check
     * @return true if capability is supported
     */
    virtual bool supports_audio_capability(AudioCapability capability) const = 0;
    
    /**
     * @brief Check if audio format is supported
     * @param format Audio format to check
     * @return true if format is supported
     */
    virtual bool supports_format(AudioFormat format) const = 0;
    
    /**
     * @brief Check if sample rate is supported
     * @param sample_rate Sample rate to check
     * @return true if sample rate is supported
     */
    virtual bool supports_sample_rate(uint32_t sample_rate) const = 0;
    
    /**
     * @brief Get supported sample rates
     * @return Vector of supported sample rates
     */
    virtual std::vector<uint32_t> get_supported_sample_rates() const = 0;
    
    /**
     * @brief Get supported audio formats
     * @return Vector of supported audio formats
     */
    virtual std::vector<AudioFormat> get_supported_formats() const = 0;
    
    /**
     * @brief Get maximum simultaneous audio sources
     * @return Maximum number of audio sources
     */
    virtual uint32_t get_max_sources() const = 0;
    
    /**
     * @brief Get minimum buffer size
     * @return Minimum buffer size in frames
     */
    virtual uint32_t get_min_buffer_size() const = 0;
    
    /**
     * @brief Get maximum buffer size
     * @return Maximum buffer size in frames
     */
    virtual uint32_t get_max_buffer_size() const = 0;
    
    // === Performance and Statistics ===
    
    /**
     * @brief Get audio performance statistics
     * @return Performance statistics structure
     */
    virtual AudioStats get_stats() const = 0;
    
    /**
     * @brief Reset performance statistics
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> reset_stats() = 0;
    
    /**
     * @brief Get audio memory usage
     * @return HALResult containing memory usage in bytes
     */
    virtual HALResult<size_t> get_audio_memory_usage() const = 0;
    
    /**
     * @brief Get available audio memory
     * @return HALResult containing available memory in bytes
     */
    virtual HALResult<size_t> get_available_audio_memory() const = 0;
    
    // === Platform-Specific Extensions ===
    
    /**
     * @brief Get platform-specific extension interface
     * @param extension_name Name of extension to query
     * @return Pointer to extension interface (nullptr if not supported)
     */
    virtual void* get_extension_interface(std::string_view extension_name) = 0;
    
    /**
     * @brief Dreamcast AICA specific: Get AICA extension interface
     * @return Pointer to AICA extension interface (Dreamcast only)
     */
    virtual void* get_aica_extension() = 0;
    
    /**
     * @brief Web Audio specific: Get Web Audio extension interface
     * @return Pointer to Web Audio extension interface (browser only)
     */
    virtual void* get_web_audio_extension() = 0;
    
    /**
     * @brief Core Audio specific: Get Core Audio extension interface
     * @return Pointer to Core Audio extension interface (macOS only)
     */
    virtual void* get_core_audio_extension() = 0;
    
    // === IHALInterface implementation ===
    std::string_view get_interface_name() const override {
        return "audio";
    }
};

// === Platform-Specific Extension Interfaces ===

/**
 * @brief Dreamcast AICA extension interface
 * 
 * Provides access to AICA-specific features like hardware DSP,
 * ADPCM compression, and 2MB audio RAM management.
 */
class IAICAExtension {
public:
    virtual ~IAICAExtension() = default;
    
    /**
     * @brief Upload data to AICA audio RAM
     * @param data Audio data to upload
     * @param size Size of data in bytes
     * @param offset Offset in audio RAM
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> upload_to_audio_ram(const void* data, size_t size, size_t offset = 0) = 0;
    
    /**
     * @brief Configure ADPCM parameters for channel
     * @param channel AICA channel number (0-63)
     * @param start_address Start address in audio RAM
     * @param loop_address Loop point address
     * @param end_address End address
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_adpcm_parameters(uint32_t channel,
                                                 uint32_t start_address,
                                                 uint32_t loop_address,
                                                 uint32_t end_address) = 0;
    
    /**
     * @brief Configure hardware DSP effect
     * @param effect_slot DSP effect slot (0-15)
     * @param effect_type Type of DSP effect
     * @param parameters Effect parameters
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> configure_dsp_effect(uint32_t effect_slot,
                                                 uint32_t effect_type,
                                                 const std::map<std::string, float>& parameters) = 0;
    
    /**
     * @brief Get AICA audio RAM usage
     * @return Current audio RAM usage in bytes
     */
    virtual size_t get_audio_ram_usage() const = 0;
    
    /**
     * @brief Get available AICA audio RAM
     * @return Available audio RAM in bytes
     */
    virtual size_t get_available_audio_ram() const = 0;
};

/**
 * @brief Web Audio API extension interface
 * 
 * Provides access to Web Audio specific features like AudioWorklets,
 * AudioContext management, and browser-specific optimizations.
 */
class IWebAudioExtension {
public:
    virtual ~IWebAudioExtension() = default;
    
    /**
     * @brief Create Audio Worklet processor
     * @param worklet_code JavaScript code for the worklet
     * @param processor_name Name of the processor
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> create_worklet_processor(std::string_view worklet_code,
                                                    std::string_view processor_name) = 0;
    
    /**
     * @brief Set worklet parameters
     * @param processor_name Name of the worklet processor
     * @param parameters Parameter map to set
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_worklet_parameters(std::string_view processor_name,
                                                   const std::map<std::string, float>& parameters) = 0;
    
    /**
     * @brief Get AudioContext sample rate
     * @return Current AudioContext sample rate
     */
    virtual uint32_t get_context_sample_rate() const = 0;
    
    /**
     * @brief Get AudioContext state
     * @return Current AudioContext state
     */
    virtual std::string get_context_state() const = 0;
};

/**
 * @brief Core Audio extension interface
 * 
 * Provides access to macOS Core Audio specific features like Audio Units,
 * MIDI integration, and professional audio features.
 */
class ICoreAudioExtension {
public:
    virtual ~ICoreAudioExtension() = default;
    
    /**
     * @brief Load Audio Unit
     * @param component_type Audio Unit component type
     * @param component_subtype Audio Unit component subtype
     * @param manufacturer Audio Unit manufacturer
     * @return HALResult with Audio Unit handle on success
     */
    virtual HALResult<uint32_t> load_audio_unit(uint32_t component_type,
                                               uint32_t component_subtype,
                                               uint32_t manufacturer) = 0;
    
    /**
     * @brief Set Audio Unit parameters
     * @param audio_unit Audio Unit handle
     * @param parameter_id Parameter identifier
     * @param value Parameter value
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_audio_unit_parameter(uint32_t audio_unit,
                                                    uint32_t parameter_id,
                                                    float value) = 0;
    
    /**
     * @brief Get current I/O latency
     * @return Current I/O latency in frames
     */
    virtual uint32_t get_io_latency() const = 0;
    
    /**
     * @brief Set preferred I/O buffer size
     * @param buffer_size Preferred buffer size in frames
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_preferred_buffer_size(uint32_t buffer_size) = 0;
};

// === Utility Functions ===

/**
 * @brief Audio utility functions
 */
namespace audio {

/**
 * @brief Convert audio capability to string
 * @param capability Audio capability to convert
 * @return String representation
 */
const char* to_string(AudioCapability capability);

/**
 * @brief Convert audio format to string
 * @param format Audio format to convert
 * @return String representation
 */
const char* to_string(AudioFormat format);

/**
 * @brief Convert audio effect type to string
 * @param effect_type Audio effect type to convert
 * @return String representation
 */
const char* to_string(AudioEffectType effect_type);

/**
 * @brief Convert distance model to string
 * @param model Distance model to convert
 * @return String representation
 */
const char* to_string(DistanceModel model);

/**
 * @brief Get bytes per sample for audio format
 * @param format Audio format
 * @return Number of bytes per sample
 */
uint32_t get_bytes_per_sample(AudioFormat format);

/**
 * @brief Check if audio format is compressed
 * @param format Audio format to check
 * @return true if format is compressed
 */
bool is_compressed_format(AudioFormat format);

/**
 * @brief Check if audio format is floating point
 * @param format Audio format to check
 * @return true if format uses floating point samples
 */
bool is_floating_point_format(AudioFormat format);

/**
 * @brief Calculate buffer size in bytes
 * @param frame_count Number of audio frames
 * @param channels Number of audio channels
 * @param format Audio sample format
 * @return Total buffer size in bytes
 */
size_t calculate_buffer_size(size_t frame_count, uint16_t channels, AudioFormat format);

/**
 * @brief Create default audio buffer descriptor
 * @param frame_count Number of frames
 * @param sample_rate Sample rate in Hz
 * @param channels Number of channels
 * @param format Audio format
 * @return Configured AudioBufferDescriptor
 */
AudioBufferDescriptor make_buffer_descriptor(size_t frame_count,
                                            uint32_t sample_rate = 44100,
                                            uint16_t channels = 2,
                                            AudioFormat format = AudioFormat::PCM_F32);

/**
 * @brief Create default audio stream descriptor
 * @param source_path Path to audio source
 * @param volume Initial volume
 * @return Configured AudioStreamDescriptor
 */
AudioStreamDescriptor make_stream_descriptor(std::string_view source_path,
                                            float volume = 1.0f);

/**
 * @brief Create default audio device descriptor
 * @param sample_rate Desired sample rate
 * @param channels Desired channel count
 * @param buffer_size Desired buffer size
 * @return Configured AudioDeviceDescriptor
 */
AudioDeviceDescriptor make_device_descriptor(uint32_t sample_rate = 44100,
                                            uint16_t channels = 2,
                                            uint32_t buffer_size = 1024);

/**
 * @brief Create default audio listener
 * @return Default AudioListener configuration
 */
AudioListener make_default_listener();

/**
 * @brief Create default 3D audio source
 * @param position Source position
 * @return Configured Audio3DSource
 */
Audio3DSource make_3d_source(const Vector3& position = Vector3{});

/**
 * @brief Convert samples between formats
 * @param input_data Input sample data
 * @param input_format Input sample format
 * @param output_data Output sample buffer
 * @param output_format Output sample format
 * @param sample_count Number of samples to convert
 */
void convert_samples(const void* input_data,
                    AudioFormat input_format,
                    void* output_data,
                    AudioFormat output_format,
                    size_t sample_count);

/**
 * @brief Interleave planar audio channels
 * @param planar_data Array of planar channel data
 * @param channel_count Number of channels
 * @param frame_count Number of frames per channel
 * @param interleaved_data Output buffer for interleaved data
 * @param bytes_per_sample Bytes per sample
 */
void interleave_channels(const void* const* planar_data,
                        uint16_t channel_count,
                        size_t frame_count,
                        void* interleaved_data,
                        uint32_t bytes_per_sample);

/**
 * @brief Deinterleave interleaved audio channels
 * @param interleaved_data Interleaved audio data
 * @param channel_count Number of channels
 * @param frame_count Number of frames
 * @param planar_data Array of output buffers for planar data
 * @param bytes_per_sample Bytes per sample
 */
void deinterleave_channels(const void* interleaved_data,
                          uint16_t channel_count,
                          size_t frame_count,
                          void* const* planar_data,
                          uint32_t bytes_per_sample);

/**
 * @brief Apply volume to audio samples
 * @param samples Audio sample data
 * @param sample_count Number of samples
 * @param format Sample format
 * @param volume Volume multiplier (0.0-1.0)
 */
void apply_volume(void* samples,
                 size_t sample_count,
                 AudioFormat format,
                 float volume);

/**
 * @brief Mix two audio buffers together
 * @param source1 First source buffer
 * @param source2 Second source buffer
 * @param destination Output buffer
 * @param sample_count Number of samples
 * @param format Sample format
 * @param mix_ratio Mix ratio (0.0 = source1 only, 1.0 = source2 only)
 */
void mix_buffers(const void* source1,
                const void* source2,
                void* destination,
                size_t sample_count,
                AudioFormat format,
                float mix_ratio = 0.5f);

} // namespace audio

} // namespace flight::hal
