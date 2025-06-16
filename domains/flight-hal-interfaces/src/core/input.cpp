/**
 * @file input.cpp
 * @brief Flight HAL Elite Input Interface Implementation
 * 
 * Implementation of input utility functions and helper methods for the
 * unified input interface system.
 */

#include "../../include/flight/hal/interfaces/input.hpp"
#include "../../include/flight/hal/core/hal_logging.hpp"
#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_map>

namespace flight::hal {

namespace input {

// === String Conversion Functions ===

const char* to_string(InputCapability capability) {
    switch (capability) {
        case InputCapability::DigitalButtons: return "DigitalButtons";
        case InputCapability::AnalogSticks: return "AnalogSticks";
        case InputCapability::AnalogTriggers: return "AnalogTriggers";
        case InputCapability::KeyboardInput: return "KeyboardInput";
        case InputCapability::MouseInput: return "MouseInput";
        case InputCapability::TouchInput: return "TouchInput";
        case InputCapability::MultiTouch: return "MultiTouch";
        case InputCapability::GestureRecognition: return "GestureRecognition";
        case InputCapability::HapticFeedback: return "HapticFeedback";
        case InputCapability::MotionSensors: return "MotionSensors";
        case InputCapability::LightGun: return "LightGun";
        case InputCapability::VMUSupport: return "VMUSupport";
        case InputCapability::DualShockController: return "DualShockController";
        case InputCapability::XInputController: return "XInputController";
        case InputCapability::WebGamepad: return "WebGamepad";
        case InputCapability::PSPAnalogMode: return "PSPAnalogMode";
        case InputCapability::EventDriven: return "EventDriven";
        case InputCapability::PollingMode: return "PollingMode";
        case InputCapability::InputMapping: return "InputMapping";
        case InputCapability::InputRecording: return "InputRecording";
        case InputCapability::InputLatencyOptimization: return "InputLatencyOptimization";
        case InputCapability::HardwareAcceleration: return "HardwareAcceleration";
        case InputCapability::BuiltinCalibration: return "BuiltinCalibration";
        case InputCapability::CustomDeadZones: return "CustomDeadZones";
        case InputCapability::PressureSensitive: return "PressureSensitive";
        case InputCapability::NetworkInput: return "NetworkInput";
        case InputCapability::InputSharing: return "InputSharing";
        case InputCapability::RemoteInput: return "RemoteInput";
        case InputCapability::SystemHotkeys: return "SystemHotkeys";
        case InputCapability::AccessibilityFeatures: return "AccessibilityFeatures";
        case InputCapability::PowerManagement: return "PowerManagement";
        case InputCapability::PlugAndPlay: return "PlugAndPlay";
        default: return "Unknown";
    }
}

const char* to_string(InputDeviceType device_type) {
    switch (device_type) {
        case InputDeviceType::Unknown: return "Unknown";
        case InputDeviceType::Gamepad: return "Gamepad";
        case InputDeviceType::Keyboard: return "Keyboard";
        case InputDeviceType::Mouse: return "Mouse";
        case InputDeviceType::TouchScreen: return "TouchScreen";
        case InputDeviceType::Joystick: return "Joystick";
        case InputDeviceType::LightGun: return "LightGun";
        case InputDeviceType::MotionController: return "MotionController";
        case InputDeviceType::TrackPad: return "TrackPad";
        case InputDeviceType::Stylus: return "Stylus";
        case InputDeviceType::DreamcastController: return "DreamcastController";
        case InputDeviceType::DreamcastVMU: return "DreamcastVMU";
        case InputDeviceType::PSPController: return "PSPController";
        case InputDeviceType::WebGamepadDevice: return "WebGamepadDevice";
        default: return "Unknown";
    }
}

const char* to_string(InputEventType event_type) {
    switch (event_type) {
        case InputEventType::ButtonDown: return "ButtonDown";
        case InputEventType::ButtonUp: return "ButtonUp";
        case InputEventType::ButtonRepeat: return "ButtonRepeat";
        case InputEventType::AnalogChange: return "AnalogChange";
        case InputEventType::MouseMove: return "MouseMove";
        case InputEventType::MouseWheel: return "MouseWheel";
        case InputEventType::TouchDown: return "TouchDown";
        case InputEventType::TouchUp: return "TouchUp";
        case InputEventType::TouchMove: return "TouchMove";
        case InputEventType::GestureBegin: return "GestureBegin";
        case InputEventType::GestureEnd: return "GestureEnd";
        case InputEventType::GestureUpdate: return "GestureUpdate";
        case InputEventType::DeviceConnected: return "DeviceConnected";
        case InputEventType::DeviceDisconnected: return "DeviceDisconnected";
        case InputEventType::CalibrationChanged: return "CalibrationChanged";
        case InputEventType::InputModeChanged: return "InputModeChanged";
        case InputEventType::HotKeyPressed: return "HotKeyPressed";
        default: return "Unknown";
    }
}

const char* to_string(InputButton button) {
    switch (button) {
        // Face buttons
        case InputButton::ButtonA: return "ButtonA";
        case InputButton::ButtonB: return "ButtonB";
        case InputButton::ButtonX: return "ButtonX";
        case InputButton::ButtonY: return "ButtonY";
        
        // Shoulder buttons
        case InputButton::LeftShoulder: return "LeftShoulder";
        case InputButton::RightShoulder: return "RightShoulder";
        case InputButton::LeftTrigger: return "LeftTrigger";
        case InputButton::RightTrigger: return "RightTrigger";
        
        // D-pad
        case InputButton::DPadUp: return "DPadUp";
        case InputButton::DPadDown: return "DPadDown";
        case InputButton::DPadLeft: return "DPadLeft";
        case InputButton::DPadRight: return "DPadRight";
        
        // Analog stick buttons
        case InputButton::LeftStickButton: return "LeftStickButton";
        case InputButton::RightStickButton: return "RightStickButton";
        
        // System buttons
        case InputButton::Start: return "Start";
        case InputButton::Select: return "Select";
        case InputButton::Home: return "Home";
        
        // Mouse buttons
        case InputButton::MouseLeft: return "MouseLeft";
        case InputButton::MouseRight: return "MouseRight";
        case InputButton::MouseMiddle: return "MouseMiddle";
        case InputButton::MouseX1: return "MouseX1";
        case InputButton::MouseX2: return "MouseX2";
        
        // Keyboard keys
        case InputButton::KeyEscape: return "KeyEscape";
        case InputButton::KeyEnter: return "KeyEnter";
        case InputButton::KeySpace: return "KeySpace";
        case InputButton::KeyTab: return "KeyTab";
        case InputButton::KeyBackspace: return "KeyBackspace";
        case InputButton::KeyDelete: return "KeyDelete";
        case InputButton::KeyInsert: return "KeyInsert";
        case InputButton::KeyHome: return "KeyHome";
        case InputButton::KeyEnd: return "KeyEnd";
        case InputButton::KeyPageUp: return "KeyPageUp";
        case InputButton::KeyPageDown: return "KeyPageDown";
        
        // Arrow keys
        case InputButton::KeyUp: return "KeyUp";
        case InputButton::KeyDown: return "KeyDown";
        case InputButton::KeyLeft: return "KeyLeft";
        case InputButton::KeyRight: return "KeyRight";
        
        // Function keys
        case InputButton::KeyF1: return "KeyF1";
        case InputButton::KeyF2: return "KeyF2";
        case InputButton::KeyF3: return "KeyF3";
        case InputButton::KeyF4: return "KeyF4";
        case InputButton::KeyF5: return "KeyF5";
        case InputButton::KeyF6: return "KeyF6";
        case InputButton::KeyF7: return "KeyF7";
        case InputButton::KeyF8: return "KeyF8";
        case InputButton::KeyF9: return "KeyF9";
        case InputButton::KeyF10: return "KeyF10";
        case InputButton::KeyF11: return "KeyF11";
        case InputButton::KeyF12: return "KeyF12";
        
        // Modifier keys
        case InputButton::KeyLeftShift: return "KeyLeftShift";
        case InputButton::KeyRightShift: return "KeyRightShift";
        case InputButton::KeyLeftCtrl: return "KeyLeftCtrl";
        case InputButton::KeyRightCtrl: return "KeyRightCtrl";
        case InputButton::KeyLeftAlt: return "KeyLeftAlt";
        case InputButton::KeyRightAlt: return "KeyRightAlt";
        case InputButton::KeyLeftSuper: return "KeyLeftSuper";
        case InputButton::KeyRightSuper: return "KeyRightSuper";
        
        // Platform-specific buttons
        case InputButton::DreamcastStart: return "DreamcastStart";
        case InputButton::DreamcastA: return "DreamcastA";
        case InputButton::DreamcastB: return "DreamcastB";
        case InputButton::DreamcastX: return "DreamcastX";
        case InputButton::DreamcastY: return "DreamcastY";
        
        case InputButton::PSPTriangle: return "PSPTriangle";
        case InputButton::PSPSquare: return "PSPSquare";
        case InputButton::PSPCross: return "PSPCross";
        case InputButton::PSPCircle: return "PSPCircle";
        case InputButton::PSPSelect: return "PSPSelect";
        case InputButton::PSPStart: return "PSPStart";
        case InputButton::PSPL: return "PSPL";
        case InputButton::PSPR: return "PSPR";
        
        // Custom buttons
        case InputButton::Custom1: return "Custom1";
        case InputButton::Custom2: return "Custom2";
        case InputButton::Custom3: return "Custom3";
        case InputButton::Custom4: return "Custom4";
        case InputButton::Custom5: return "Custom5";
        case InputButton::Custom6: return "Custom6";
        case InputButton::Custom7: return "Custom7";
        case InputButton::Custom8: return "Custom8";
        
        default: return "Unknown";
    }
}

const char* to_string(InputAxis axis) {
    switch (axis) {
        case InputAxis::LeftStickX: return "LeftStickX";
        case InputAxis::LeftStickY: return "LeftStickY";
        case InputAxis::RightStickX: return "RightStickX";
        case InputAxis::RightStickY: return "RightStickY";
        case InputAxis::LeftTrigger: return "LeftTrigger";
        case InputAxis::RightTrigger: return "RightTrigger";
        case InputAxis::MouseX: return "MouseX";
        case InputAxis::MouseY: return "MouseY";
        case InputAxis::MouseWheel: return "MouseWheel";
        case InputAxis::AccelerometerX: return "AccelerometerX";
        case InputAxis::AccelerometerY: return "AccelerometerY";
        case InputAxis::AccelerometerZ: return "AccelerometerZ";
        case InputAxis::GyroscopeX: return "GyroscopeX";
        case InputAxis::GyroscopeY: return "GyroscopeY";
        case InputAxis::GyroscopeZ: return "GyroscopeZ";
        case InputAxis::TouchX: return "TouchX";
        case InputAxis::TouchY: return "TouchY";
        case InputAxis::TouchPressure: return "TouchPressure";
        case InputAxis::PSPAnalogX: return "PSPAnalogX";
        case InputAxis::PSPAnalogY: return "PSPAnalogY";
        case InputAxis::Custom1: return "Custom1";
        case InputAxis::Custom2: return "Custom2";
        case InputAxis::Custom3: return "Custom3";
        case InputAxis::Custom4: return "Custom4";
        default: return "Unknown";
    }
}

const char* to_string(GestureType gesture) {
    switch (gesture) {
        case GestureType::None: return "None";
        case GestureType::Tap: return "Tap";
        case GestureType::DoubleTap: return "DoubleTap";
        case GestureType::LongPress: return "LongPress";
        case GestureType::Swipe: return "Swipe";
        case GestureType::Pinch: return "Pinch";
        case GestureType::Rotate: return "Rotate";
        case GestureType::Pan: return "Pan";
        case GestureType::Custom: return "Custom";
        default: return "Unknown";
    }
}

const char* to_string(HapticType haptic) {
    switch (haptic) {
        case HapticType::None: return "None";
        case HapticType::Click: return "Click";
        case HapticType::Buzz: return "Buzz";
        case HapticType::Rumble: return "Rumble";
        case HapticType::Pulse: return "Pulse";
        case HapticType::CustomWaveform: return "CustomWaveform";
        default: return "Unknown";
    }
}

// === Device State Manipulation ===

bool is_button_pressed(const InputDeviceState& state, InputButton button) {
    uint32_t button_index = static_cast<uint32_t>(button);
    uint32_t array_index = button_index / 64;
    uint32_t bit_index = button_index % 64;
    
    if (array_index >= state.button_states.size()) {
        return false;
    }
    
    return (state.button_states[array_index] & (1ULL << bit_index)) != 0;
}

void set_button_state(InputDeviceState& state, InputButton button, bool pressed) {
    uint32_t button_index = static_cast<uint32_t>(button);
    uint32_t array_index = button_index / 64;
    uint32_t bit_index = button_index % 64;
    
    if (array_index >= state.button_states.size()) {
        return;
    }
    
    if (pressed) {
        state.button_states[array_index] |= (1ULL << bit_index);
    } else {
        state.button_states[array_index] &= ~(1ULL << bit_index);
    }
}

float get_axis_value(const InputDeviceState& state, InputAxis axis) {
    uint32_t axis_index = static_cast<uint32_t>(axis);
    
    if (axis_index >= state.axis_values.size()) {
        return 0.0f;
    }
    
    return state.axis_values[axis_index];
}

void set_axis_value(InputDeviceState& state, InputAxis axis, float value) {
    uint32_t axis_index = static_cast<uint32_t>(axis);
    
    if (axis_index >= state.axis_values.size()) {
        return;
    }
    
    // Clamp value to valid range
    value = std::clamp(value, -1.0f, 1.0f);
    state.axis_values[axis_index] = value;
}

// === Factory Functions ===

InputDeviceDescriptor make_device_descriptor(InputDeviceType device_type, 
                                            const std::string& device_name) {
    InputDeviceDescriptor descriptor = {};
    descriptor.device_type = device_type;
    descriptor.device_name = device_name;
    descriptor.vendor_name = "Unknown";
    descriptor.product_id = "Unknown";
    descriptor.capability_mask = 0;
    descriptor.button_count = 0;
    descriptor.axis_count = 0;
    descriptor.touch_point_count = 0;
    descriptor.supports_haptic = false;
    descriptor.supports_motion = false;
    descriptor.hot_pluggable = true;
    descriptor.requires_calibration = false;
    descriptor.has_vmu_slots = false;
    descriptor.supports_analog_mode = false;
    descriptor.debug_name = device_name;
    
    // Set defaults based on device type
    switch (device_type) {
        case InputDeviceType::Gamepad:
            descriptor.capability_mask = static_cast<uint32_t>(InputCapability::DigitalButtons) |
                                       static_cast<uint32_t>(InputCapability::AnalogSticks) |
                                       static_cast<uint32_t>(InputCapability::AnalogTriggers);
            descriptor.button_count = 16;
            descriptor.axis_count = 6;
            descriptor.supports_haptic = true;
            break;
            
        case InputDeviceType::Keyboard:
            descriptor.capability_mask = static_cast<uint32_t>(InputCapability::KeyboardInput);
            descriptor.button_count = 256;
            descriptor.axis_count = 0;
            break;
            
        case InputDeviceType::Mouse:
            descriptor.capability_mask = static_cast<uint32_t>(InputCapability::MouseInput);
            descriptor.button_count = 8;
            descriptor.axis_count = 3;
            break;
            
        case InputDeviceType::TouchScreen:
            descriptor.capability_mask = static_cast<uint32_t>(InputCapability::TouchInput) |
                                       static_cast<uint32_t>(InputCapability::MultiTouch) |
                                       static_cast<uint32_t>(InputCapability::GestureRecognition);
            descriptor.button_count = 0;
            descriptor.axis_count = 3;
            descriptor.touch_point_count = 10;
            break;
            
        case InputDeviceType::DreamcastController:
            descriptor.capability_mask = static_cast<uint32_t>(InputCapability::DigitalButtons) |
                                       static_cast<uint32_t>(InputCapability::AnalogSticks) |
                                       static_cast<uint32_t>(InputCapability::AnalogTriggers) |
                                       static_cast<uint32_t>(InputCapability::VMUSupport);
            descriptor.button_count = 8;
            descriptor.axis_count = 4;
            descriptor.has_vmu_slots = true;
            descriptor.supports_haptic = true;
            break;
            
        case InputDeviceType::PSPController:
            descriptor.capability_mask = static_cast<uint32_t>(InputCapability::DigitalButtons) |
                                       static_cast<uint32_t>(InputCapability::AnalogSticks) |
                                       static_cast<uint32_t>(InputCapability::PSPAnalogMode);
            descriptor.button_count = 10;
            descriptor.axis_count = 2;
            descriptor.supports_analog_mode = true;
            break;
            
        default:
            break;
    }
    
    return descriptor;
}

InputMapping make_default_mapping(const std::string& mapping_name, 
                                 const std::string& context) {
    InputMapping mapping = {};
    mapping.mapping_name = mapping_name;
    mapping.context = context;
    mapping.is_active = true;
    mapping.priority = 0;
    
    // Add some default button mappings
    mapping.button_actions[InputButton::ButtonA] = "action_primary";
    mapping.button_actions[InputButton::ButtonB] = "action_secondary";
    mapping.button_actions[InputButton::ButtonX] = "action_tertiary";
    mapping.button_actions[InputButton::ButtonY] = "action_quaternary";
    mapping.button_actions[InputButton::Start] = "menu_start";
    mapping.button_actions[InputButton::Select] = "menu_select";
    mapping.button_actions[InputButton::KeyEscape] = "menu_escape";
    mapping.button_actions[InputButton::KeyEnter] = "menu_confirm";
    mapping.button_actions[InputButton::KeySpace] = "action_jump";
    
    // Add default axis mappings
    mapping.axis_actions[InputAxis::LeftStickX] = "move_horizontal";
    mapping.axis_actions[InputAxis::LeftStickY] = "move_vertical";
    mapping.axis_actions[InputAxis::RightStickX] = "look_horizontal";
    mapping.axis_actions[InputAxis::RightStickY] = "look_vertical";
    mapping.axis_actions[InputAxis::LeftTrigger] = "action_left_trigger";
    mapping.axis_actions[InputAxis::RightTrigger] = "action_right_trigger";
    
    // Set default dead zones
    mapping.dead_zones[InputAxis::LeftStickX] = 0.1f;
    mapping.dead_zones[InputAxis::LeftStickY] = 0.1f;
    mapping.dead_zones[InputAxis::RightStickX] = 0.1f;
    mapping.dead_zones[InputAxis::RightStickY] = 0.1f;
    mapping.dead_zones[InputAxis::LeftTrigger] = 0.05f;
    mapping.dead_zones[InputAxis::RightTrigger] = 0.05f;
    
    // Set default sensitivities
    mapping.sensitivities[InputAxis::LeftStickX] = 1.0f;
    mapping.sensitivities[InputAxis::LeftStickY] = 1.0f;
    mapping.sensitivities[InputAxis::RightStickX] = 1.0f;
    mapping.sensitivities[InputAxis::RightStickY] = 1.0f;
    mapping.sensitivities[InputAxis::LeftTrigger] = 1.0f;
    mapping.sensitivities[InputAxis::RightTrigger] = 1.0f;
    
    // Set default inversion flags
    mapping.invert_flags[InputAxis::LeftStickX] = false;
    mapping.invert_flags[InputAxis::LeftStickY] = false;
    mapping.invert_flags[InputAxis::RightStickX] = false;
    mapping.invert_flags[InputAxis::RightStickY] = false;
    
    // Add default gesture mappings
    mapping.gesture_actions[GestureType::Tap] = "gesture_tap";
    mapping.gesture_actions[GestureType::DoubleTap] = "gesture_double_tap";
    mapping.gesture_actions[GestureType::LongPress] = "gesture_long_press";
    mapping.gesture_actions[GestureType::Swipe] = "gesture_swipe";
    mapping.gesture_actions[GestureType::Pinch] = "gesture_pinch";
    
    return mapping;
}

InputEvent make_button_event(InputResourceHandle device, InputButton button, bool pressed) {
    InputEvent event = {};
    event.event_type = pressed ? InputEventType::ButtonDown : InputEventType::ButtonUp;
    event.device = device;
    event.timestamp = std::chrono::high_resolution_clock::now();
    event.button_event.button = button;
    event.button_event.pressed = pressed;
    event.button_event.repeat = false;
    event.modifier_flags = 0;
    event.platform_data = nullptr;
    
    return event;
}

InputEvent make_analog_event(InputResourceHandle device, InputAxis axis, 
                           float value, float previous_value) {
    InputEvent event = {};
    event.event_type = InputEventType::AnalogChange;
    event.device = device;
    event.timestamp = std::chrono::high_resolution_clock::now();
    event.analog_event.axis = axis;
    event.analog_event.value = value;
    event.analog_event.previous_value = previous_value;
    event.modifier_flags = 0;
    event.platform_data = nullptr;
    
    return event;
}

InputEvent make_touch_event(InputResourceHandle device, InputEventType event_type, 
                          uint32_t touch_id, Vector2 position, float pressure) {
    InputEvent event = {};
    event.event_type = event_type;
    event.device = device;
    event.timestamp = std::chrono::high_resolution_clock::now();
    event.touch_event.touch_id = touch_id;
    event.touch_event.position = position;
    event.touch_event.pressure = pressure;
    event.modifier_flags = 0;
    event.platform_data = nullptr;
    
    return event;
}

HapticEffect make_haptic_effect(HapticType type, float intensity, 
                               std::chrono::duration<float> duration) {
    HapticEffect effect = {};
    effect.type = type;
    effect.intensity = std::clamp(intensity, 0.0f, 1.0f);
    effect.duration = duration;
    effect.delay = std::chrono::duration<float>(0.0f);
    effect.repeating = false;
    effect.repeat_count = 1;
    effect.sample_rate = 44100.0f;
    
    return effect;
}

CalibrationData make_default_calibration(InputResourceHandle device) {
    CalibrationData calibration = {};
    calibration.device = device;
    calibration.touch_minimum = Vector2(-1.0f, -1.0f);
    calibration.touch_maximum = Vector2(1.0f, 1.0f);
    calibration.accelerometer_bias = Vector3(0.0f, 0.0f, 0.0f);
    calibration.gyroscope_bias = Vector3(0.0f, 0.0f, 0.0f);
    calibration.is_valid = true;
    calibration.calibration_time = std::chrono::system_clock::now();
    
    // Set default axis calibration ranges
    const std::array<InputAxis, 6> analog_axes = {
        InputAxis::LeftStickX, InputAxis::LeftStickY,
        InputAxis::RightStickX, InputAxis::RightStickY,
        InputAxis::LeftTrigger, InputAxis::RightTrigger
    };
    
    for (InputAxis axis : analog_axes) {
        calibration.axis_minimums[axis] = -1.0f;
        calibration.axis_maximums[axis] = 1.0f;
        calibration.axis_centers[axis] = 0.0f;
        calibration.axis_ranges[axis] = 2.0f;
    }
    
    // Triggers have different ranges (0.0 to 1.0)
    calibration.axis_minimums[InputAxis::LeftTrigger] = 0.0f;
    calibration.axis_minimums[InputAxis::RightTrigger] = 0.0f;
    calibration.axis_centers[InputAxis::LeftTrigger] = 0.0f;
    calibration.axis_centers[InputAxis::RightTrigger] = 0.0f;
    calibration.axis_ranges[InputAxis::LeftTrigger] = 1.0f;
    calibration.axis_ranges[InputAxis::RightTrigger] = 1.0f;
    
    return calibration;
}

// === Utility Functions ===

float apply_dead_zone(float value, float dead_zone) {
    if (dead_zone <= 0.0f) {
        return value;
    }
    
    float abs_value = std::abs(value);
    if (abs_value < dead_zone) {
        return 0.0f;
    }
    
    // Rescale value to maintain full range outside dead zone
    float sign = (value >= 0.0f) ? 1.0f : -1.0f;
    float scaled_value = (abs_value - dead_zone) / (1.0f - dead_zone);
    return sign * std::clamp(scaled_value, 0.0f, 1.0f);
}

float apply_sensitivity(float value, float sensitivity) {
    if (sensitivity <= 0.0f) {
        return 0.0f;
    }
    
    return std::clamp(value * sensitivity, -1.0f, 1.0f);
}

Vector2 normalize_touch_position(const Vector2& position, const Vector2& surface_size) {
    if (surface_size.x <= 0.0f || surface_size.y <= 0.0f) {
        return Vector2(0.0f, 0.0f);
    }
    
    return Vector2(
        std::clamp(position.x / surface_size.x, 0.0f, 1.0f),
        std::clamp(position.y / surface_size.y, 0.0f, 1.0f)
    );
}

float calculate_touch_distance(const Vector2& point1, const Vector2& point2) {
    Vector2 diff = point2 - point1;
    return diff.length();
}

Vector2 calculate_gesture_velocity(const Vector2& start_position, const Vector2& end_position, 
                                  std::chrono::duration<float> duration) {
    if (duration.count() <= 0.0f) {
        return Vector2(0.0f, 0.0f);
    }
    
    Vector2 displacement = end_position - start_position;
    return displacement * (1.0f / duration.count());
}

bool are_events_equivalent(const InputEvent& event1, const InputEvent& event2) {
    if (event1.event_type != event2.event_type) {
        return false;
    }
    
    if (event1.device.id != event2.device.id) {
        return false;
    }
    
    switch (event1.event_type) {
        case InputEventType::ButtonDown:
        case InputEventType::ButtonUp:
        case InputEventType::ButtonRepeat:
            return event1.button_event.button == event2.button_event.button &&
                   event1.button_event.pressed == event2.button_event.pressed;
            
        case InputEventType::AnalogChange:
            return event1.analog_event.axis == event2.analog_event.axis &&
                   std::abs(event1.analog_event.value - event2.analog_event.value) < 0.001f;
            
        case InputEventType::TouchDown:
        case InputEventType::TouchUp:
        case InputEventType::TouchMove:
            return event1.touch_event.touch_id == event2.touch_event.touch_id &&
                   std::abs(event1.touch_event.position.x - event2.touch_event.position.x) < 0.001f &&
                   std::abs(event1.touch_event.position.y - event2.touch_event.position.y) < 0.001f;
            
        case InputEventType::GestureBegin:
        case InputEventType::GestureEnd:
        case InputEventType::GestureUpdate:
            return event1.gesture_event.gesture == event2.gesture_event.gesture &&
                   std::abs(event1.gesture_event.position.x - event2.gesture_event.position.x) < 0.001f &&
                   std::abs(event1.gesture_event.position.y - event2.gesture_event.position.y) < 0.001f;
            
        case InputEventType::DeviceConnected:
        case InputEventType::DeviceDisconnected:
            return event1.device_event.device_type == event2.device_event.device_type;
            
        default:
            return false;
    }
}

} // namespace input

} // namespace flight::hal
