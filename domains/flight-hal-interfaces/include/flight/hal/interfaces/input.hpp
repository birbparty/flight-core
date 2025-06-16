/**
 * @file input.hpp
 * @brief Flight HAL Elite Input Interface
 * 
 * Unified input interface spanning from Dreamcast controllers with VMU support
 * to modern multi-touch, gesture recognition, and cross-platform input handling.
 * Provides both event-driven and polling access patterns with flexible input
 * mapping and platform-specific optimizations.
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
#include <chrono>

namespace flight::hal {

/**
 * @brief Input-specific capability enumeration
 * 
 * Runtime detection of input features for graceful degradation across platforms
 * from Dreamcast VMU controllers to modern multi-touch interfaces.
 */
enum class InputCapability : uint32_t {
    // Core input types
    DigitalButtons = 1 << 0,        ///< Digital button input support
    AnalogSticks = 1 << 1,          ///< Analog stick/joystick support
    AnalogTriggers = 1 << 2,        ///< Analog trigger/shoulder button support
    KeyboardInput = 1 << 3,         ///< Keyboard input support
    MouseInput = 1 << 4,            ///< Mouse input support
    TouchInput = 1 << 5,            ///< Touch screen input support
    
    // Advanced input features
    MultiTouch = 1 << 6,            ///< Multi-touch gesture support
    GestureRecognition = 1 << 7,    ///< Touch gesture recognition
    HapticFeedback = 1 << 8,        ///< Haptic/rumble feedback
    MotionSensors = 1 << 9,         ///< Accelerometer/gyroscope support
    LightGun = 1 << 10,             ///< Light gun input support
    
    // Platform-specific controllers
    VMUSupport = 1 << 11,           ///< Dreamcast VMU expansion support
    DualShockController = 1 << 12,  ///< PlayStation-style controllers
    XInputController = 1 << 13,     ///< Xbox controller support
    WebGamepad = 1 << 14,           ///< Web Gamepad API support
    PSPAnalogMode = 1 << 15,        ///< PSP analog/digital mode switching
    
    // Input processing modes
    EventDriven = 1 << 16,          ///< Event-driven input callbacks
    PollingMode = 1 << 17,          ///< Frame-based polling support
    InputMapping = 1 << 18,         ///< Configurable input mapping
    InputRecording = 1 << 19,       ///< Input recording/playback
    InputLatencyOptimization = 1 << 20, ///< Low-latency input processing
    
    // Hardware features
    HardwareAcceleration = 1 << 21, ///< Hardware-accelerated input processing
    BuiltinCalibration = 1 << 22,   ///< Hardware calibration support
    CustomDeadZones = 1 << 23,      ///< Configurable analog dead zones
    PressureSensitive = 1 << 24,    ///< Pressure-sensitive input
    
    // Networking and multiplayer
    NetworkInput = 1 << 25,         ///< Network input synchronization
    InputSharing = 1 << 26,         ///< Input sharing between devices
    RemoteInput = 1 << 27,          ///< Remote input control
    
    // System integration
    SystemHotkeys = 1 << 28,        ///< System-level hotkey support
    AccessibilityFeatures = 1 << 29, ///< Accessibility input features
    PowerManagement = 1 << 30,      ///< Power-aware input handling
    PlugAndPlay = 1U << 31          ///< Hot-plug device detection
};

/**
 * @brief Input resource types
 */
enum class InputResourceType : uint8_t {
    InputDevice = 0,    ///< Physical input device
    InputMapper,        ///< Input mapping configuration
    InputRecorder,      ///< Input recording session
    GestureRecognizer,  ///< Gesture recognition engine
    HapticController,   ///< Haptic feedback controller
    InputBuffer,        ///< Input event buffer
    CalibrationData,    ///< Device calibration data
    InputProfile,       ///< User input profile/settings
    VMUDevice,          ///< Dreamcast VMU device
    TouchSurface        ///< Touch input surface
};

/**
 * @brief Input device types
 */
enum class InputDeviceType : uint8_t {
    Unknown = 0,        ///< Unknown device type
    Gamepad,            ///< Game controller/gamepad
    Keyboard,           ///< Keyboard device
    Mouse,              ///< Mouse device
    TouchScreen,        ///< Touch screen
    Joystick,           ///< Analog joystick
    LightGun,           ///< Light gun device
    MotionController,   ///< Motion sensing controller
    TrackPad,           ///< Track pad device
    Stylus,             ///< Stylus/pen input
    
    // Platform-specific devices
    DreamcastController = 100, ///< Dreamcast controller
    DreamcastVMU,       ///< Dreamcast Visual Memory Unit
    PSPController,      ///< PSP controller
    WebGamepadDevice    ///< Web Gamepad API device
};

/**
 * @brief Input event types
 */
enum class InputEventType : uint8_t {
    ButtonDown = 0,     ///< Button press event
    ButtonUp,           ///< Button release event
    ButtonRepeat,       ///< Button repeat event
    AnalogChange,       ///< Analog value change
    MouseMove,          ///< Mouse movement
    MouseWheel,         ///< Mouse wheel scroll
    TouchDown,          ///< Touch begin
    TouchUp,            ///< Touch end
    TouchMove,          ///< Touch movement
    GestureBegin,       ///< Gesture start
    GestureEnd,         ///< Gesture end
    GestureUpdate,      ///< Gesture update
    DeviceConnected,    ///< Device connection event
    DeviceDisconnected, ///< Device disconnection event
    CalibrationChanged, ///< Calibration update
    InputModeChanged,   ///< Input mode change (PSP analog/digital)
    HotKeyPressed       ///< System hotkey activation
};

/**
 * @brief Standard button identifiers
 */
enum class InputButton : uint16_t {
    // Face buttons (most common layout)
    ButtonA = 0,        ///< A button (Xbox) / Cross (PlayStation)
    ButtonB,            ///< B button (Xbox) / Circle (PlayStation)
    ButtonX,            ///< X button (Xbox) / Square (PlayStation)
    ButtonY,            ///< Y button (Xbox) / Triangle (PlayStation)
    
    // Shoulder buttons
    LeftShoulder,       ///< Left shoulder button (L1/LB)
    RightShoulder,      ///< Right shoulder button (R1/RB)
    LeftTrigger,        ///< Left trigger (L2/LT)
    RightTrigger,       ///< Right trigger (R2/RT)
    
    // D-pad
    DPadUp,             ///< D-pad up
    DPadDown,           ///< D-pad down
    DPadLeft,           ///< D-pad left
    DPadRight,          ///< D-pad right
    
    // Analog stick buttons
    LeftStickButton,    ///< Left analog stick click (L3/LS)
    RightStickButton,   ///< Right analog stick click (R3/RS)
    
    // System buttons
    Start,              ///< Start button
    Select,             ///< Select/Back button
    Home,               ///< Home/Guide button
    
    // Mouse buttons
    MouseLeft = 100,    ///< Left mouse button
    MouseRight,         ///< Right mouse button
    MouseMiddle,        ///< Middle mouse button
    MouseX1,            ///< Mouse X1 button
    MouseX2,            ///< Mouse X2 button
    
    // Keyboard special keys
    KeyEscape = 200,    ///< Escape key
    KeyEnter,           ///< Enter/Return key
    KeySpace,           ///< Space bar
    KeyTab,             ///< Tab key
    KeyBackspace,       ///< Backspace key
    KeyDelete,          ///< Delete key
    KeyInsert,          ///< Insert key
    KeyHome,            ///< Home key
    KeyEnd,             ///< End key
    KeyPageUp,          ///< Page Up key
    KeyPageDown,        ///< Page Down key
    
    // Arrow keys
    KeyUp,              ///< Up arrow key
    KeyDown,            ///< Down arrow key
    KeyLeft,            ///< Left arrow key
    KeyRight,           ///< Right arrow key
    
    // Function keys
    KeyF1,              ///< F1 key
    KeyF2,              ///< F2 key
    KeyF3,              ///< F3 key
    KeyF4,              ///< F4 key
    KeyF5,              ///< F5 key
    KeyF6,              ///< F6 key
    KeyF7,              ///< F7 key
    KeyF8,              ///< F8 key
    KeyF9,              ///< F9 key
    KeyF10,             ///< F10 key
    KeyF11,             ///< F11 key
    KeyF12,             ///< F12 key
    
    // Modifier keys
    KeyLeftShift = 300, ///< Left Shift key
    KeyRightShift,      ///< Right Shift key
    KeyLeftCtrl,        ///< Left Control key
    KeyRightCtrl,       ///< Right Control key
    KeyLeftAlt,         ///< Left Alt key
    KeyRightAlt,        ///< Right Alt key
    KeyLeftSuper,       ///< Left Super/Windows key
    KeyRightSuper,      ///< Right Super/Windows key
    
    // Platform-specific buttons
    DreamcastStart = 1000, ///< Dreamcast Start button
    DreamcastA,         ///< Dreamcast A button
    DreamcastB,         ///< Dreamcast B button
    DreamcastX,         ///< Dreamcast X button
    DreamcastY,         ///< Dreamcast Y button
    
    PSPTriangle = 1100, ///< PSP Triangle button
    PSPSquare,          ///< PSP Square button
    PSPCross,           ///< PSP Cross button
    PSPCircle,          ///< PSP Circle button
    PSPSelect,          ///< PSP Select button
    PSPStart,           ///< PSP Start button
    PSPL,               ///< PSP L shoulder button
    PSPR,               ///< PSP R shoulder button
    
    // Custom/Extended buttons
    Custom1 = 2000,     ///< Custom button 1
    Custom2,            ///< Custom button 2
    Custom3,            ///< Custom button 3
    Custom4,            ///< Custom button 4
    Custom5,            ///< Custom button 5
    Custom6,            ///< Custom button 6
    Custom7,            ///< Custom button 7
    Custom8             ///< Custom button 8
};

/**
 * @brief Analog input identifiers
 */
enum class InputAxis : uint8_t {
    LeftStickX = 0,     ///< Left analog stick X axis
    LeftStickY,         ///< Left analog stick Y axis
    RightStickX,        ///< Right analog stick X axis
    RightStickY,        ///< Right analog stick Y axis
    LeftTrigger,        ///< Left trigger analog value
    RightTrigger,       ///< Right trigger analog value
    
    // Mouse axes
    MouseX,             ///< Mouse X movement
    MouseY,             ///< Mouse Y movement
    MouseWheel,         ///< Mouse wheel scroll
    
    // Motion sensors
    AccelerometerX,     ///< Accelerometer X axis
    AccelerometerY,     ///< Accelerometer Y axis
    AccelerometerZ,     ///< Accelerometer Z axis
    GyroscopeX,         ///< Gyroscope X axis
    GyroscopeY,         ///< Gyroscope Y axis
    GyroscopeZ,         ///< Gyroscope Z axis
    
    // Touch input
    TouchX,             ///< Touch X coordinate
    TouchY,             ///< Touch Y coordinate
    TouchPressure,      ///< Touch pressure
    
    // Platform-specific
    PSPAnalogX,         ///< PSP analog stick X
    PSPAnalogY,         ///< PSP analog stick Y
    
    // Custom axes
    Custom1,            ///< Custom axis 1
    Custom2,            ///< Custom axis 2
    Custom3,            ///< Custom axis 3
    Custom4             ///< Custom axis 4
};

/**
 * @brief Touch gesture types
 */
enum class GestureType : uint8_t {
    None = 0,           ///< No gesture
    Tap,                ///< Single tap
    DoubleTap,          ///< Double tap
    LongPress,          ///< Long press/hold
    Swipe,              ///< Swipe gesture
    Pinch,              ///< Pinch to zoom
    Rotate,             ///< Rotation gesture
    Pan,                ///< Pan/drag gesture
    Custom              ///< Custom gesture
};

/**
 * @brief Haptic feedback types
 */
enum class HapticType : uint8_t {
    None = 0,           ///< No haptic feedback
    Click,              ///< Click feedback
    Buzz,               ///< Buzz/vibration
    Rumble,             ///< Controller rumble
    Pulse,              ///< Pulse feedback
    CustomWaveform      ///< Custom waveform
};

// Forward declarations
class IInputDevice;
class IInputMapper;
class IInputRecorder;
class IGestureRecognizer;
class IHapticController;

/**
 * @brief 2D vector for input coordinates and motion
 */
struct Vector2 {
    float x, y;
    
    Vector2(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}
    
    Vector2 operator+(const Vector2& other) const { return {x + other.x, y + other.y}; }
    Vector2 operator-(const Vector2& other) const { return {x - other.x, y - other.y}; }
    Vector2 operator*(float scalar) const { return {x * scalar, y * scalar}; }
    
    float length() const { return std::sqrt(x*x + y*y); }
    Vector2 normalize() const { float len = length(); return len > 0.0f ? *this * (1.0f/len) : Vector2{}; }
};

/**
 * @brief 3D vector for motion sensor data
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
 * @brief Input resource handle
 */
struct InputResourceHandle {
    uint32_t id;                    ///< Unique resource identifier
    InputResourceType type;         ///< Resource type
    uint32_t generation;            ///< Generation counter for validation
    
    bool is_valid() const { return id != 0; }
    void invalidate() { id = 0; generation = 0; }
};

/**
 * @brief Input device description and capabilities
 */
struct InputDeviceDescriptor {
    InputDeviceType device_type;    ///< Type of input device
    std::string device_name;        ///< Human-readable device name
    std::string vendor_name;        ///< Device vendor/manufacturer
    std::string product_id;         ///< Product identifier
    uint32_t capability_mask;       ///< Supported InputCapability bitmask
    
    // Button/axis counts
    uint32_t button_count;          ///< Number of digital buttons
    uint32_t axis_count;            ///< Number of analog axes
    uint32_t touch_point_count;     ///< Number of simultaneous touch points
    
    // Device properties
    bool supports_haptic;           ///< Haptic feedback support
    bool supports_motion;           ///< Motion sensor support
    bool hot_pluggable;             ///< Device can be hot-plugged
    bool requires_calibration;      ///< Device requires calibration
    
    // Platform-specific properties
    bool has_vmu_slots;             ///< Has VMU expansion slots (Dreamcast)
    bool supports_analog_mode;      ///< Supports analog mode switching (PSP)
    
    std::string_view debug_name;    ///< Debug name for profiling
};

/**
 * @brief Input event data
 */
struct InputEvent {
    InputEventType event_type;      ///< Type of input event
    InputResourceHandle device;     ///< Source device handle
    std::chrono::high_resolution_clock::time_point timestamp; ///< Event timestamp
    
    // Event-specific data
    union {
        struct {
            InputButton button;     ///< Button identifier
            bool pressed;           ///< Button state (true = pressed)
            bool repeat;            ///< Key repeat event
        } button_event;
        
        struct {
            InputAxis axis;         ///< Axis identifier
            float value;            ///< Analog value (-1.0 to 1.0)
            float previous_value;   ///< Previous analog value
        } analog_event;
        
        struct {
            Vector2 position;       ///< Mouse position
            Vector2 delta;          ///< Movement delta
        } mouse_event;
        
        struct {
            float delta;            ///< Wheel scroll delta
        } wheel_event;
        
        struct {
            uint32_t touch_id;      ///< Touch point identifier
            Vector2 position;       ///< Touch position (0-1 normalized)
            float pressure;         ///< Touch pressure (0-1)
        } touch_event;
        
        struct {
            GestureType gesture;    ///< Gesture type
            Vector2 position;       ///< Gesture center position
            Vector2 delta;          ///< Gesture movement delta
            float scale;            ///< Scale factor (pinch)
            float rotation;         ///< Rotation angle (radians)
        } gesture_event;
        
        struct {
            InputDeviceType device_type; ///< Connected device type
            const char* device_name;     ///< Device name (pointer to string)
        } device_event;
    };
    
    // Additional metadata
    uint32_t modifier_flags;        ///< Modifier key state (Ctrl, Alt, Shift, etc.)
    void* platform_data;           ///< Platform-specific event data
};

/**
 * @brief Input mapping configuration
 */
struct InputMapping {
    std::string mapping_name;       ///< Name of this mapping
    std::string context;            ///< Context where mapping applies
    
    // Button mappings
    std::map<InputButton, std::string> button_actions; ///< Button to action mappings
    std::map<InputAxis, std::string> axis_actions;     ///< Axis to action mappings
    
    // Analog settings
    std::map<InputAxis, float> dead_zones;      ///< Analog dead zone settings
    std::map<InputAxis, float> sensitivities;   ///< Analog sensitivity settings
    std::map<InputAxis, bool> invert_flags;     ///< Analog inversion flags
    
    // Gesture mappings
    std::map<GestureType, std::string> gesture_actions; ///< Gesture to action mappings
    
    // Combination inputs
    std::map<std::vector<InputButton>, std::string> combo_actions; ///< Button combo mappings
    
    bool is_active;                 ///< Whether mapping is currently active
    int32_t priority;               ///< Mapping priority (higher = more important)
};

/**
 * @brief Touch gesture data
 */
struct GestureData {
    GestureType type;               ///< Type of gesture
    Vector2 start_position;         ///< Initial position
    Vector2 current_position;       ///< Current position
    Vector2 velocity;               ///< Gesture velocity
    float scale;                    ///< Scale factor (pinch gestures)
    float rotation;                 ///< Rotation angle (rotation gestures)
    std::chrono::duration<float> duration; ///< Gesture duration
    uint32_t touch_count;           ///< Number of touches involved
    bool is_complete;               ///< Whether gesture is complete
};

/**
 * @brief Haptic feedback configuration
 */
struct HapticEffect {
    HapticType type;                ///< Type of haptic effect
    float intensity;                ///< Effect intensity (0.0-1.0)
    std::chrono::duration<float> duration; ///< Effect duration
    std::chrono::duration<float> delay;    ///< Delay before effect starts
    bool repeating;                 ///< Whether effect repeats
    uint32_t repeat_count;          ///< Number of repetitions
    
    // Custom waveform data (for CustomWaveform type)
    std::vector<float> waveform_data; ///< Custom waveform samples
    float sample_rate;              ///< Waveform sample rate
};

/**
 * @brief Device calibration data
 */
struct CalibrationData {
    InputResourceHandle device;     ///< Device being calibrated
    
    // Analog calibration
    std::map<InputAxis, float> axis_minimums;  ///< Minimum values for each axis
    std::map<InputAxis, float> axis_maximums;  ///< Maximum values for each axis
    std::map<InputAxis, float> axis_centers;   ///< Center/neutral values
    std::map<InputAxis, float> axis_ranges;    ///< Full range values
    
    // Touch calibration
    Vector2 touch_minimum;          ///< Touch surface minimum coordinates
    Vector2 touch_maximum;          ///< Touch surface maximum coordinates
    
    // Motion sensor calibration
    Vector3 accelerometer_bias;     ///< Accelerometer bias correction
    Vector3 gyroscope_bias;         ///< Gyroscope bias correction
    
    bool is_valid;                  ///< Whether calibration data is valid
    std::chrono::system_clock::time_point calibration_time; ///< When calibrated
};

/**
 * @brief Input device state snapshot
 */
struct InputDeviceState {
    InputResourceHandle device;     ///< Device handle
    std::chrono::high_resolution_clock::time_point timestamp; ///< State timestamp
    
    // Button states (bitfield for efficiency)
    std::array<uint64_t, 8> button_states; ///< Button state bitfield
    
    // Analog values
    std::array<float, 32> axis_values;      ///< Analog axis values
    
    // Touch states
    struct TouchPoint {
        bool active;                ///< Touch point is active
        Vector2 position;           ///< Touch position
        float pressure;             ///< Touch pressure
        uint32_t id;                ///< Touch point ID
    };
    std::array<TouchPoint, 10> touch_points; ///< Touch point states
    
    // Motion sensor data
    Vector3 accelerometer;          ///< Accelerometer readings
    Vector3 gyroscope;              ///< Gyroscope readings
    
    // Device-specific data
    void* platform_state;          ///< Platform-specific state data
};

/**
 * @brief Input system performance statistics
 */
struct InputStats {
    uint64_t total_events_processed; ///< Total input events processed
    uint64_t events_per_second;     ///< Current events per second
    uint64_t button_events;         ///< Button event count
    uint64_t analog_events;         ///< Analog event count
    uint64_t touch_events;          ///< Touch event count
    uint64_t gesture_events;        ///< Gesture event count
    
    // Latency statistics
    double average_input_latency_ms; ///< Average input latency
    double peak_input_latency_ms;   ///< Peak input latency
    uint64_t missed_events;         ///< Number of missed/dropped events
    
    // Device statistics
    uint32_t connected_devices;     ///< Number of connected devices
    uint32_t active_devices;        ///< Number of active devices
    uint32_t device_disconnections; ///< Device disconnection count
    
    // Memory usage
    size_t event_buffer_usage;      ///< Event buffer memory usage
    size_t total_input_memory;      ///< Total input system memory usage
    
    // Platform-specific stats
    uint64_t polling_cycles;        ///< Number of polling cycles
    uint64_t callback_invocations;  ///< Number of callback invocations
};

/**
 * @brief Input event callback function type
 */
using InputEventCallback = std::function<void(const InputEvent& event)>;

/**
 * @brief Input action callback function type
 */
using InputActionCallback = std::function<void(const std::string& action, float value, const InputEvent& source_event)>;

/**
 * @brief Device connection callback function type
 */
using DeviceConnectionCallback = std::function<void(InputResourceHandle device, bool connected)>;

/**
 * @brief Gesture recognition callback function type
 */
using GestureCallback = std::function<void(const GestureData& gesture)>;

/**
 * @brief Input device interface
 * 
 * Represents a physical input device with polling and event-driven access.
 */
class IInputDevice {
public:
    virtual ~IInputDevice() = default;
    
    /**
     * @brief Get device descriptor
     * @return Device configuration and capabilities
     */
    virtual const InputDeviceDescriptor& get_descriptor() const = 0;
    
    /**
     * @brief Get device resource handle
     * @return Handle for this device
     */
    virtual InputResourceHandle get_handle() const = 0;
    
    /**
     * @brief Check if device is connected
     * @return true if device is connected and responsive
     */
    virtual bool is_connected() const = 0;
    
    /**
     * @brief Get current device state
     * @return Current input state snapshot
     */
    virtual InputDeviceState get_current_state() const = 0;
    
    /**
     * @brief Check if button is currently pressed
     * @param button Button to check
     * @return true if button is pressed
     */
    virtual bool is_button_pressed(InputButton button) const = 0;
    
    /**
     * @brief Get analog axis value
     * @param axis Axis to query
     * @return Analog value (-1.0 to 1.0)
     */
    virtual float get_axis_value(InputAxis axis) const = 0;
    
    /**
     * @brief Set event callback for this device
     * @param callback Callback function for input events
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_event_callback(InputEventCallback callback) = 0;
    
    /**
     * @brief Enable or disable the device
     * @param enabled Whether device should be active
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_enabled(bool enabled) = 0;
    
    /**
     * @brief Check if device is enabled
     * @return true if device is enabled
     */
    virtual bool is_enabled() const = 0;
    
    /**
     * @brief Set device calibration data
     * @param calibration Calibration data to apply
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_calibration(const CalibrationData& calibration) = 0;
    
    /**
     * @brief Get device calibration data
     * @return Current calibration data
     */
    virtual const CalibrationData& get_calibration() const = 0;
    
    /**
     * @brief Trigger haptic feedback (if supported)
     * @param effect Haptic effect to play
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> play_haptic_effect(const HapticEffect& effect) = 0;
    
    /**
     * @brief Stop all haptic effects
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> stop_haptic_effects() = 0;
    
    /**
     * @brief Update device (called by input system)
     * @param delta_time Time since last update
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> update(std::chrono::duration<float> delta_time) = 0;
};

/**
 * @brief Input mapper interface
 * 
 * Handles mapping between physical inputs and logical game actions.
 */
class IInputMapper {
public:
    virtual ~IInputMapper() = default;
    
    /**
     * @brief Get mapper resource handle
     * @return Handle for this mapper
     */
    virtual InputResourceHandle get_handle() const = 0;
    
    /**
     * @brief Load input mapping configuration
     * @param mapping Mapping configuration to load
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> load_mapping(const InputMapping& mapping) = 0;
    
    /**
     * @brief Save current mapping configuration
     * @param mapping_name Name for the mapping
     * @return HALResult with saved mapping on success
     */
    virtual HALResult<InputMapping> save_mapping(const std::string& mapping_name) = 0;
    
    /**
     * @brief Map input event to action
     * @param event Input event to map
     * @return HALResult with mapped action name on success
     */
    virtual HALResult<std::string> map_event_to_action(const InputEvent& event) = 0;
    
    /**
     * @brief Set action callback
     * @param callback Callback function for mapped actions
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_action_callback(InputActionCallback callback) = 0;
    
    /**
     * @brief Get current active mapping
     * @return Current input mapping configuration
     */
    virtual const InputMapping& get_current_mapping() const = 0;
    
    /**
     * @brief Set mapping context
     * @param context Context name to activate
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_context(const std::string& context) = 0;
    
    /**
     * @brief Get available contexts
     * @return Vector of available context names
     */
    virtual std::vector<std::string> get_available_contexts() const = 0;
    
    /**
     * @brief Check if action is currently active
     * @param action Action name to check
     * @return true if action is active
     */
    virtual bool is_action_active(const std::string& action) const = 0;
    
    /**
     * @brief Get action value
     * @param action Action name to query
     * @return Action value (0.0-1.0 for buttons, -1.0 to 1.0 for axes)
     */
    virtual float get_action_value(const std::string& action) const = 0;
};

/**
 * @brief Input recorder interface
 * 
 * Records and plays back input sequences for testing and automation.
 */
class IInputRecorder {
public:
    virtual ~IInputRecorder() = default;
    
    /**
     * @brief Get recorder resource handle
     * @return Handle for this recorder
     */
    virtual InputResourceHandle get_handle() const = 0;
    
    /**
     * @brief Start recording input events
     * @param session_name Name for the recording session
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> start_recording(const std::string& session_name) = 0;
    
    /**
     * @brief Stop recording input events
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> stop_recording() = 0;
    
    /**
     * @brief Play back recorded input sequence
     * @param session_name Name of session to play back
     * @param loop Whether to loop the playback
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> play_recording(const std::string& session_name, bool loop = false) = 0;
    
    /**
     * @brief Stop current playback
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> stop_playback() = 0;
    
    /**
     * @brief Save recording to file
     * @param session_name Session to save
     * @param file_path Path to save recording
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> save_recording(const std::string& session_name, const std::string& file_path) = 0;
    
    /**
     * @brief Load recording from file  
     * @param session_name Name for loaded session
     * @param file_path Path to recording file
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> load_recording(const std::string& session_name, const std::string& file_path) = 0;
    
    /**
     * @brief Check if currently recording
     * @return true if recording is active
     */
    virtual bool is_recording() const = 0;
    
    /**
     * @brief Check if currently playing back
     * @return true if playback is active
     */
    virtual bool is_playing() const = 0;
    
    /**
     * @brief Get available recording sessions
     * @return Vector of session names
     */
    virtual std::vector<std::string> get_available_sessions() const = 0;
};

/**
 * @brief Gesture recognizer interface
 * 
 * Recognizes touch gestures and converts them to input events.
 */
class IGestureRecognizer {
public:
    virtual ~IGestureRecognizer() = default;
    
    /**
     * @brief Get recognizer resource handle
     * @return Handle for this recognizer
     */
    virtual InputResourceHandle get_handle() const = 0;
    
    /**
     * @brief Process touch events for gesture recognition
     * @param touch_events Vector of touch events to process
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> process_touch_events(const std::vector<InputEvent>& touch_events) = 0;
    
    /**
     * @brief Set gesture callback
     * @param callback Callback function for recognized gestures
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_gesture_callback(GestureCallback callback) = 0;
    
    /**
     * @brief Configure gesture recognition parameters
     * @param gesture_type Type of gesture to configure
     * @param parameters Configuration parameters
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> configure_gesture(GestureType gesture_type, const std::map<std::string, float>& parameters) = 0;
    
    /**
     * @brief Enable or disable gesture type
     * @param gesture_type Gesture type to enable/disable
     * @param enabled Whether gesture should be recognized
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_gesture_enabled(GestureType gesture_type, bool enabled) = 0;
    
    /**
     * @brief Check if gesture type is enabled
     * @param gesture_type Gesture type to check
     * @return true if gesture type is enabled
     */
    virtual bool is_gesture_enabled(GestureType gesture_type) const = 0;
    
    /**
     * @brief Get supported gesture types
     * @return Vector of supported gesture types
     */
    virtual std::vector<GestureType> get_supported_gestures() const = 0;
    
    /**
     * @brief Reset gesture recognition state
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> reset() = 0;
};

/**
 * @brief Haptic controller interface
 * 
 * Controls haptic feedback devices for tactile response.
 */
class IHapticController {
public:
    virtual ~IHapticController() = default;
    
    /**
     * @brief Get controller resource handle
     * @return Handle for this controller
     */
    virtual InputResourceHandle get_handle() const = 0;
    
    /**
     * @brief Play haptic effect
     * @param effect Effect configuration to play
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> play_effect(const HapticEffect& effect) = 0;
    
    /**
     * @brief Stop all haptic effects
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> stop_all_effects() = 0;
    
    /**
     * @brief Stop specific haptic effect
     * @param effect_id ID of effect to stop
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> stop_effect(uint32_t effect_id) = 0;
    
    /**
     * @brief Set global haptic intensity
     * @param intensity Intensity multiplier (0.0-1.0)
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_intensity(float intensity) = 0;
    
    /**
     * @brief Get current haptic intensity
     * @return Current intensity level
     */
    virtual float get_intensity() const = 0;
    
    /**
     * @brief Check if haptic feedback is supported
     * @return true if haptic feedback is available
     */
    virtual bool is_haptic_supported() const = 0;
    
    /**
     * @brief Get supported haptic effect types
     * @return Vector of supported effect types
     */
    virtual std::vector<HapticType> get_supported_effects() const = 0;
    
    /**
     * @brief Load custom waveform
     * @param waveform_data Waveform sample data
     * @param sample_rate Sample rate of waveform
     * @return HALResult with waveform ID on success
     */
    virtual HALResult<uint32_t> load_waveform(const std::vector<float>& waveform_data, float sample_rate) = 0;
};

/**
 * @brief Enhanced input interface
 * 
 * Comprehensive input system supporting gamepads, keyboard, mouse, touch,
 * and gesture recognition across platforms from Dreamcast to modern systems.
 * Provides both event-driven and polling access patterns with flexible
 * input mapping and platform-specific optimizations.
 */
class IInputInterface : public IHALInterface, public ICapabilityProvider {
public:
    virtual ~IInputInterface() = default;
    
    // === System Management ===
    
    /**
     * @brief Initialize input system
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> initialize() = 0;
    
    /**
     * @brief Shutdown input system
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> shutdown() = 0;
    
    /**
     * @brief Check if input system is initialized
     * @return true if initialized and ready
     */
    virtual bool is_initialized() const = 0;
    
    /**
     * @brief Update input system (call once per frame)
     * @param delta_time Time since last update
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> update(std::chrono::duration<float> delta_time) = 0;
    
    // === Device Management ===
    
    /**
     * @brief Get connected input devices
     * @return HALResult containing vector of device handles
     */
    virtual HALResult<std::vector<InputResourceHandle>> get_connected_devices() const = 0;
    
    /**
     * @brief Get input device by handle
     * @param device Device handle
     * @return HALResult containing device pointer on success
     */
    virtual HALResult<IInputDevice*> get_device(InputResourceHandle device) = 0;
    
    /**
     * @brief Get input device by type
     * @param device_type Type of device to find
     * @param device_index Index of device (0 for first device of type)
     * @return HALResult containing device pointer on success
     */
    virtual HALResult<IInputDevice*> get_device_by_type(InputDeviceType device_type, uint32_t device_index = 0) = 0;
    
    /**
     * @brief Set device connection callback
     * @param callback Callback for device connection events
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_device_connection_callback(DeviceConnectionCallback callback) = 0;
    
    /**
     * @brief Enable or disable device hot-plug detection
     * @param enabled Whether to detect device connections
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_hot_plug_enabled(bool enabled) = 0;
    
    // === Event Handling ===
    
    /**
     * @brief Set global input event callback
     * @param callback Callback function for all input events
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_global_event_callback(InputEventCallback callback) = 0;
    
    /**
     * @brief Poll for input events
     * @param events Vector to receive events
     * @param max_events Maximum number of events to retrieve
     * @return HALResult containing number of events retrieved
     */
    virtual HALResult<uint32_t> poll_events(std::vector<InputEvent>& events, uint32_t max_events = 1000) = 0;
    
    /**
     * @brief Clear event queue
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> clear_event_queue() = 0;
    
    /**
     * @brief Set event queue size
     * @param size Maximum number of events to queue
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_event_queue_size(uint32_t size) = 0;
    
    // === Input Mapping ===
    
    /**
     * @brief Create input mapper
     * @return HALResult containing mapper pointer on success
     */
    virtual HALResult<std::unique_ptr<IInputMapper>> create_input_mapper() = 0;
    
    /**
     * @brief Get global input mapper
     * @return Pointer to global mapper (nullptr if not available)
     */
    virtual IInputMapper* get_global_mapper() = 0;
    
    /**
     * @brief Load input mapping from file
     * @param file_path Path to mapping file
     * @return HALResult containing loaded mapping on success
     */
    virtual HALResult<InputMapping> load_mapping_from_file(const std::string& file_path) = 0;
    
    /**
     * @brief Save input mapping to file
     * @param mapping Mapping to save
     * @param file_path Path to save mapping
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> save_mapping_to_file(const InputMapping& mapping, const std::string& file_path) = 0;
    
    // === Gesture Recognition ===
    
    /**
     * @brief Create gesture recognizer
     * @return HALResult containing recognizer pointer on success
     */
    virtual HALResult<std::unique_ptr<IGestureRecognizer>> create_gesture_recognizer() = 0;
    
    /**
     * @brief Get global gesture recognizer
     * @return Pointer to global recognizer (nullptr if not available)
     */
    virtual IGestureRecognizer* get_global_gesture_recognizer() = 0;
    
    // === Haptic Feedback ===
    
    /**
     * @brief Create haptic controller
     * @param device Target device for haptic feedback
     * @return HALResult containing controller pointer on success
     */
    virtual HALResult<std::unique_ptr<IHapticController>> create_haptic_controller(InputResourceHandle device) = 0;
    
    /**
     * @brief Get haptic controller for device
     * @param device Device handle
     * @return Pointer to haptic controller (nullptr if not supported)
     */
    virtual IHapticController* get_haptic_controller(InputResourceHandle device) = 0;
    
    // === Recording and Playback ===
    
    /**
     * @brief Create input recorder
     * @return HALResult containing recorder pointer on success
     */
    virtual HALResult<std::unique_ptr<IInputRecorder>> create_input_recorder() = 0;
    
    /**
     * @brief Get global input recorder
     * @return Pointer to global recorder (nullptr if not available)
     */
    virtual IInputRecorder* get_global_recorder() = 0;
    
    // === Capability Queries ===
    
    /**
     * @brief Check if input capability is supported
     * @param capability Input capability to check
     * @return true if capability is supported
     */
    virtual bool supports_input_capability(InputCapability capability) const = 0;
    
    /**
     * @brief Check if device type is supported
     * @param device_type Device type to check
     * @return true if device type is supported
     */
    virtual bool supports_device_type(InputDeviceType device_type) const = 0;
    
    /**
     * @brief Get supported device types
     * @return Vector of supported device types
     */
    virtual std::vector<InputDeviceType> get_supported_device_types() const = 0;
    
    /**
     * @brief Get maximum simultaneous devices
     * @param device_type Device type to query
     * @return Maximum number of simultaneous devices
     */
    virtual uint32_t get_max_devices(InputDeviceType device_type) const = 0;
    
    // === Performance and Statistics ===
    
    /**
     * @brief Get input performance statistics
     * @return Performance statistics structure
     */
    virtual InputStats get_stats() const = 0;
    
    /**
     * @brief Reset performance statistics
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> reset_stats() = 0;
    
    /**
     * @brief Get input latency
     * @return HALResult containing average latency in milliseconds
     */
    virtual HALResult<double> get_input_latency() const = 0;
    
    /**
     * @brief Set input latency optimization
     * @param enabled Whether to enable low-latency mode
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_low_latency_mode(bool enabled) = 0;
    
    // === Platform-Specific Extensions ===
    
    /**
     * @brief Get platform-specific extension interface
     * @param extension_name Name of extension to query
     * @return Pointer to extension interface (nullptr if not supported)
     */
    virtual void* get_extension_interface(std::string_view extension_name) = 0;
    
    /**
     * @brief Dreamcast specific: Get VMU extension interface
     * @return Pointer to VMU extension interface (Dreamcast only)
     */
    virtual void* get_vmu_extension() = 0;
    
    /**
     * @brief PSP specific: Get PSP extension interface
     * @return Pointer to PSP extension interface (PSP only)
     */
    virtual void* get_psp_extension() = 0;
    
    /**
     * @brief Web specific: Get Web Gamepad extension interface
     * @return Pointer to Web Gamepad extension interface (browser only)
     */
    virtual void* get_web_gamepad_extension() = 0;
    
    // === IHALInterface implementation ===
    std::string_view get_interface_name() const override {
        return "input";
    }
};

// === Platform-Specific Extension Interfaces ===

/**
 * @brief Dreamcast VMU extension interface
 * 
 * Provides access to VMU-specific features like memory card functionality,
 * mini-games, and LCD display.
 */
class IVMUExtension {
public:
    virtual ~IVMUExtension() = default;
    
    /**
     * @brief Get VMU device information
     * @param vmu_slot VMU slot number (0-7)
     * @return HALResult containing VMU device info
     */
    virtual HALResult<InputDeviceDescriptor> get_vmu_info(uint32_t vmu_slot) = 0;
    
    /**
     * @brief Read data from VMU memory
     * @param vmu_slot VMU slot number
     * @param address Memory address to read from
     * @param data Buffer to receive data
     * @param size Number of bytes to read
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> read_vmu_memory(uint32_t vmu_slot, uint32_t address, void* data, size_t size) = 0;
    
    /**
     * @brief Write data to VMU memory
     * @param vmu_slot VMU slot number
     * @param address Memory address to write to
     * @param data Data to write
     * @param size Number of bytes to write
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> write_vmu_memory(uint32_t vmu_slot, uint32_t address, const void* data, size_t size) = 0;
    
    /**
     * @brief Display image on VMU LCD
     * @param vmu_slot VMU slot number
     * @param image_data LCD image data (48x32 pixels, 1 bit per pixel)
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> display_vmu_image(uint32_t vmu_slot, const uint8_t* image_data) = 0;
    
    /**
     * @brief Set VMU beep sound
     * @param vmu_slot VMU slot number
     * @param frequency Beep frequency in Hz
     * @param duration Beep duration in milliseconds
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> vmu_beep(uint32_t vmu_slot, uint32_t frequency, uint32_t duration) = 0;
    
    /**
     * @brief Check if VMU is connected
     * @param vmu_slot VMU slot number
     * @return true if VMU is connected
     */
    virtual bool is_vmu_connected(uint32_t vmu_slot) const = 0;
    
    /**
     * @brief Get VMU free memory
     * @param vmu_slot VMU slot number
     * @return HALResult containing free memory in bytes
     */
    virtual HALResult<uint32_t> get_vmu_free_memory(uint32_t vmu_slot) const = 0;
};

/**
 * @brief PSP input extension interface
 * 
 * Provides access to PSP-specific input features like analog mode switching
 * and system button handling.
 */
class IPSPInputExtension {
public:
    virtual ~IPSPInputExtension() = default;
    
    /**
     * @brief Set analog mode
     * @param enabled Whether analog mode should be enabled
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_analog_mode(bool enabled) = 0;
    
    /**
     * @brief Check if analog mode is enabled
     * @return true if analog mode is active
     */
    virtual bool is_analog_mode_enabled() const = 0;
    
    /**
     * @brief Get PSP system button state
     * @return Current system button state
     */
    virtual uint32_t get_system_button_state() const = 0;
    
    /**
     * @brief Set button repeat parameters
     * @param initial_delay Initial delay before repeat starts
     * @param repeat_delay Delay between repeats
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_button_repeat(std::chrono::milliseconds initial_delay,
                                             std::chrono::milliseconds repeat_delay) = 0;
    
    /**
     * @brief Get current latch mode
     * @return Current latch mode setting
     */
    virtual uint32_t get_latch_mode() const = 0;
    
    /**
     * @brief Set latch mode
     * @param mode Latch mode to set
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_latch_mode(uint32_t mode) = 0;
};

/**
 * @brief Web Gamepad extension interface
 * 
 * Provides access to Web Gamepad API specific features and browser
 * compatibility handling.
 */
class IWebGamepadExtension {
public:
    virtual ~IWebGamepadExtension() = default;
    
    /**
     * @brief Get gamepad mapping string
     * @param gamepad_index Gamepad index
     * @return HALResult containing mapping string
     */
    virtual HALResult<std::string> get_gamepad_mapping(uint32_t gamepad_index) = 0;
    
    /**
     * @brief Check if gamepad is connected
     * @param gamepad_index Gamepad index
     * @return true if gamepad is connected
     */
    virtual bool is_gamepad_connected(uint32_t gamepad_index) const = 0;
    
    /**
     * @brief Get gamepad timestamp
     * @param gamepad_index Gamepad index
     * @return HALResult containing timestamp
     */
    virtual HALResult<double> get_gamepad_timestamp(uint32_t gamepad_index) = 0;
    
    /**
     * @brief Enable gamepad vibration
     * @param gamepad_index Gamepad index
     * @param enabled Whether vibration should be enabled
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_gamepad_vibration_enabled(uint32_t gamepad_index, bool enabled) = 0;
    
    /**
     * @brief Trigger gamepad vibration
     * @param gamepad_index Gamepad index
     * @param strong_magnitude Strong actuator magnitude (0.0-1.0)
     * @param weak_magnitude Weak actuator magnitude (0.0-1.0)
     * @param duration Vibration duration in milliseconds
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> vibrate_gamepad(uint32_t gamepad_index, 
                                           float strong_magnitude, 
                                           float weak_magnitude, 
                                           uint32_t duration) = 0;
    
    /**
     * @brief Get maximum number of gamepads
     * @return Maximum supported gamepads
     */
    virtual uint32_t get_max_gamepads() const = 0;
};

// === Utility Functions ===

/**
 * @brief Input utility functions
 */
namespace input {

/**
 * @brief Convert input capability to string
 * @param capability Input capability to convert
 * @return String representation
 */
const char* to_string(InputCapability capability);

/**
 * @brief Convert device type to string
 * @param device_type Device type to convert
 * @return String representation
 */
const char* to_string(InputDeviceType device_type);

/**
 * @brief Convert event type to string
 * @param event_type Event type to convert
 * @return String representation
 */
const char* to_string(InputEventType event_type);

/**
 * @brief Convert button to string
 * @param button Button to convert
 * @return String representation
 */
const char* to_string(InputButton button);

/**
 * @brief Convert axis to string
 * @param axis Axis to convert
 * @return String representation
 */
const char* to_string(InputAxis axis);

/**
 * @brief Convert gesture type to string
 * @param gesture Gesture type to convert
 * @return String representation
 */
const char* to_string(GestureType gesture);

/**
 * @brief Convert haptic type to string
 * @param haptic Haptic type to convert
 * @return String representation
 */
const char* to_string(HapticType haptic);

/**
 * @brief Check if button is pressed in device state
 * @param state Device state to check
 * @param button Button to check
 * @return true if button is pressed
 */
bool is_button_pressed(const InputDeviceState& state, InputButton button);

/**
 * @brief Set button state in device state
 * @param state Device state to modify
 * @param button Button to set
 * @param pressed Whether button is pressed
 */
void set_button_state(InputDeviceState& state, InputButton button, bool pressed);

/**
 * @brief Get axis value from device state
 * @param state Device state to query
 * @param axis Axis to get value for
 * @return Axis value (-1.0 to 1.0)
 */
float get_axis_value(const InputDeviceState& state, InputAxis axis);

/**
 * @brief Set axis value in device state
 * @param state Device state to modify
 * @param axis Axis to set
 * @param value Axis value to set
 */
void set_axis_value(InputDeviceState& state, InputAxis axis, float value);

/**
 * @brief Create default device descriptor
 * @param device_type Type of device
 * @param device_name Name of device
 * @return Configured InputDeviceDescriptor
 */
InputDeviceDescriptor make_device_descriptor(InputDeviceType device_type, 
                                            const std::string& device_name);

/**
 * @brief Create default input mapping
 * @param mapping_name Name for the mapping
 * @param context Context for the mapping
 * @return Configured InputMapping
 */
InputMapping make_default_mapping(const std::string& mapping_name, 
                                 const std::string& context = "default");

/**
 * @brief Create button input event
 * @param device Source device handle
 * @param button Button that changed
 * @param pressed Whether button is pressed
 * @return Configured InputEvent
 */
InputEvent make_button_event(InputResourceHandle device, InputButton button, bool pressed);

/**
 * @brief Create analog input event
 * @param device Source device handle
 * @param axis Axis that changed
 * @param value New axis value
 * @param previous_value Previous axis value
 * @return Configured InputEvent
 */
InputEvent make_analog_event(InputResourceHandle device, InputAxis axis, float value, float previous_value);

/**
 * @brief Create touch input event
 * @param device Source device handle
 * @param event_type Type of touch event
 * @param touch_id Touch point identifier
 * @param position Touch position
 * @param pressure Touch pressure
 * @return Configured InputEvent
 */
InputEvent make_touch_event(InputResourceHandle device, InputEventType event_type, 
                           uint32_t touch_id, Vector2 position, float pressure);

/**
 * @brief Create default haptic effect
 * @param type Haptic effect type
 * @param intensity Effect intensity
 * @param duration Effect duration
 * @return Configured HapticEffect
 */
HapticEffect make_haptic_effect(HapticType type, float intensity, std::chrono::duration<float> duration);

/**
 * @brief Create default calibration data
 * @param device Device to calibrate
 * @return Default CalibrationData structure
 */
CalibrationData make_default_calibration(InputResourceHandle device);

/**
 * @brief Apply dead zone to analog value
 * @param value Input value
 * @param dead_zone Dead zone threshold (0.0-1.0)
 * @return Value with dead zone applied
 */
float apply_dead_zone(float value, float dead_zone);

/**
 * @brief Apply sensitivity to analog value
 * @param value Input value
 * @param sensitivity Sensitivity multiplier
 * @return Value with sensitivity applied
 */
float apply_sensitivity(float value, float sensitivity);

/**
 * @brief Normalize touch coordinates
 * @param position Raw touch position
 * @param surface_size Size of touch surface
 * @return Normalized position (0.0-1.0)
 */
Vector2 normalize_touch_position(const Vector2& position, const Vector2& surface_size);

/**
 * @brief Calculate distance between two touch points
 * @param point1 First touch point
 * @param point2 Second touch point
 * @return Distance between points
 */
float calculate_touch_distance(const Vector2& point1, const Vector2& point2);

/**
 * @brief Calculate gesture velocity
 * @param start_position Starting position
 * @param end_position Ending position
 * @param duration Time duration
 * @return Velocity vector
 */
Vector2 calculate_gesture_velocity(const Vector2& start_position, const Vector2& end_position, 
                                  std::chrono::duration<float> duration);

/**
 * @brief Check if two input events are equivalent
 * @param event1 First event
 * @param event2 Second event
 * @return true if events are equivalent
 */
bool are_events_equivalent(const InputEvent& event1, const InputEvent& event2);

} // namespace input

} // namespace flight::hal
