/**
 * @file mock_input_driver.hpp
 * @brief Mock Input Driver for Flight HAL Testing
 * 
 * Mock implementation of input devices for testing the unified input interface.
 * Supports simulation of gamepads, keyboards, mice, touch screens, and 
 * platform-specific devices like Dreamcast controllers and PSP controllers.
 */

#pragma once

#include "../../include/flight/hal/interfaces/input.hpp"
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <random>
#include <mutex>
#include <iostream>

namespace flight::hal::mock {

/**
 * @brief Mock input device implementation
 * 
 * Simulates various input device types with configurable behavior
 * for comprehensive testing of the input interface.
 */
class MockInputDevice : public IInputDevice {
private:
    InputDeviceDescriptor descriptor_;
    InputResourceHandle handle_;
    CalibrationData calibration_;
    InputDeviceState current_state_;
    InputEventCallback event_callback_;
    bool enabled_;
    bool connected_;
    mutable std::mutex state_mutex_;

public:
    explicit MockInputDevice(const InputDeviceDescriptor& desc) 
        : descriptor_(desc), enabled_(true), connected_(true) {
        
        // Generate unique handle
        static std::atomic<uint32_t> next_id{1};
        handle_.id = next_id++;
        handle_.type = InputResourceType::InputDevice;
        handle_.generation = 1;
        
        // Initialize calibration
        calibration_ = input::make_default_calibration(handle_);
        
        // Initialize device state
        current_state_.device = handle_;
        current_state_.timestamp = std::chrono::high_resolution_clock::now();
        current_state_.button_states.fill(0);
        current_state_.axis_values.fill(0.0f);
        for (auto& touch : current_state_.touch_points) {
            touch.active = false;
            touch.position = Vector2(0.0f, 0.0f);
            touch.pressure = 0.0f;
            touch.id = 0;
        }
        current_state_.accelerometer = Vector3(0.0f, 0.0f, -9.8f); // Gravity
        current_state_.gyroscope = Vector3(0.0f, 0.0f, 0.0f);
        current_state_.platform_state = nullptr;
    }
    
    ~MockInputDevice() override = default;
    
    // IInputDevice implementation
    const InputDeviceDescriptor& get_descriptor() const override {
        return descriptor_;
    }
    
    InputResourceHandle get_handle() const override {
        return handle_;
    }
    
    bool is_connected() const override {
        return connected_;
    }
    
    InputDeviceState get_current_state() const override {
        std::lock_guard<std::mutex> lock(state_mutex_);
        auto state = current_state_;
        state.timestamp = std::chrono::high_resolution_clock::now();
        return state;
    }
    
    bool is_button_pressed(InputButton button) const override {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return input::is_button_pressed(current_state_, button);
    }
    
    float get_axis_value(InputAxis axis) const override {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return input::get_axis_value(current_state_, axis);
    }
    
    HALResult<void> set_event_callback(InputEventCallback callback) override {
        event_callback_ = callback;
        return HALResult<void>::success();
    }
    
    HALResult<void> set_enabled(bool enabled) override {
        enabled_ = enabled;
        return HALResult<void>::success();
    }
    
    bool is_enabled() const override {
        return enabled_;
    }
    
    HALResult<void> set_calibration(const CalibrationData& calibration) override {
        calibration_ = calibration;
        return HALResult<void>::success();
    }
    
    const CalibrationData& get_calibration() const override {
        return calibration_;
    }
    
    HALResult<void> play_haptic_effect(const HapticEffect& effect) override {
        if (!descriptor_.supports_haptic) {
            return HALResult<void>::error(errors::feature_not_supported(1, "Device does not support haptic feedback"));
        }
        return HALResult<void>::success();
    }
    
    HALResult<void> stop_haptic_effects() override {
        if (!descriptor_.supports_haptic) {
            return HALResult<void>::error(errors::feature_not_supported(1, "Device does not support haptic feedback"));
        }
        return HALResult<void>::success();
    }
    
    HALResult<void> update(std::chrono::duration<float> delta_time) override {
        if (!enabled_ || !connected_) {
            return HALResult<void>::success();
        }
        
        // Simulate some input changes based on device type
        simulate_input_changes(delta_time);
        
        return HALResult<void>::success();
    }
    
    // Mock-specific methods
    void set_connected(bool connected) {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (connected_ != connected) {
            connected_ = connected;
            if (event_callback_) {
                InputEvent event = {};
                event.event_type = connected ? InputEventType::DeviceConnected : InputEventType::DeviceDisconnected;
                event.device = handle_;
                event.timestamp = std::chrono::high_resolution_clock::now();
                event.device_event.device_type = descriptor_.device_type;
                event.device_event.device_name = descriptor_.device_name.c_str();
                event.modifier_flags = 0;
                event.platform_data = nullptr;
                
                event_callback_(event);
            }
        }
    }
    
    void simulate_button_press(InputButton button, bool pressed) {
        std::lock_guard<std::mutex> lock(state_mutex_);
        bool was_pressed = input::is_button_pressed(current_state_, button);
        
        if (was_pressed != pressed) {
            input::set_button_state(current_state_, button, pressed);
            
            if (event_callback_) {
                InputEvent event = input::make_button_event(handle_, button, pressed);
                event_callback_(event);
            }
        }
    }
    
    void simulate_axis_change(InputAxis axis, float value) {
        std::lock_guard<std::mutex> lock(state_mutex_);
        float previous_value = input::get_axis_value(current_state_, axis);
        
        if (std::abs(previous_value - value) > 0.001f) {
            input::set_axis_value(current_state_, axis, value);
            
            if (event_callback_) {
                InputEvent event = input::make_analog_event(handle_, axis, value, previous_value);
                event_callback_(event);
            }
        }
    }
    
    void simulate_touch(uint32_t touch_id, Vector2 position, float pressure, InputEventType event_type) {
        std::lock_guard<std::mutex> lock(state_mutex_);
        
        if (touch_id < current_state_.touch_points.size()) {
            auto& touch_point = current_state_.touch_points[touch_id];
            
            switch (event_type) {
                case InputEventType::TouchDown:
                    touch_point.active = true;
                    touch_point.position = position;
                    touch_point.pressure = pressure;
                    touch_point.id = touch_id;
                    break;
                    
                case InputEventType::TouchUp:
                    touch_point.active = false;
                    touch_point.pressure = 0.0f;
                    break;
                    
                case InputEventType::TouchMove:
                    if (touch_point.active) {
                        touch_point.position = position;
                        touch_point.pressure = pressure;
                    }
                    break;
                    
                default:
                    return;
            }
            
            if (event_callback_) {
                InputEvent event = input::make_touch_event(handle_, event_type, touch_id, position, pressure);
                event_callback_(event);
            }
        }
    }

private:
    void simulate_input_changes(std::chrono::duration<float> delta_time) {
        // Simple simulation based on device type
        static std::mt19937 rng(std::random_device{}());
        static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        
        switch (descriptor_.device_type) {
            case InputDeviceType::Gamepad:
            case InputDeviceType::DreamcastController:
                // Simulate slight analog stick drift
                if (dist(rng) > 0.98f) {
                    simulate_axis_change(InputAxis::LeftStickX, dist(rng) * 0.1f);
                    simulate_axis_change(InputAxis::LeftStickY, dist(rng) * 0.1f);
                }
                break;
                
            case InputDeviceType::PSPController:
                // Simulate PSP analog stick movement
                if (dist(rng) > 0.99f) {
                    simulate_axis_change(InputAxis::PSPAnalogX, dist(rng) * 0.05f);
                    simulate_axis_change(InputAxis::PSPAnalogY, dist(rng) * 0.05f);
                }
                break;
                
            case InputDeviceType::MotionController:
                // Simulate motion sensor noise
                {
                    std::lock_guard<std::mutex> lock(state_mutex_);
                    current_state_.accelerometer.x += dist(rng) * 0.01f;
                    current_state_.accelerometer.y += dist(rng) * 0.01f;
                    current_state_.gyroscope.x += dist(rng) * 0.01f;
                    current_state_.gyroscope.y += dist(rng) * 0.01f;
                    current_state_.gyroscope.z += dist(rng) * 0.01f;
                }
                break;
                
            default:
                // No automatic simulation for other device types
                break;
        }
    }
};

/**
 * @brief Mock input interface implementation
 * 
 * Provides a complete mock implementation of the input interface
 * for testing and development purposes.
 */
class MockInputInterface : public IInputInterface {
private:
    std::vector<std::unique_ptr<MockInputDevice>> devices_;
    InputEventCallback global_event_callback_;
    DeviceConnectionCallback device_connection_callback_;
    bool initialized_;
    bool hot_plug_enabled_;
    InputStats stats_;
    mutable std::mutex interface_mutex_;

public:
    MockInputInterface() : initialized_(false), hot_plug_enabled_(true) {
        // Initialize stats
        stats_ = {};
    }
    
    ~MockInputInterface() override {
        if (initialized_) {
            shutdown();
        }
    }
    
    // IInputInterface implementation
    HALResult<void> initialize() override {
        std::lock_guard<std::mutex> lock(interface_mutex_);
        
        if (initialized_) {
            return HALResult<void>::error(errors::invalid_state(1, "Input interface already initialized"));
        }
        
        // Create some default mock devices
        create_default_devices();
        
        initialized_ = true;
        return HALResult<void>::success();
    }
    
    HALResult<void> shutdown() override {
        std::lock_guard<std::mutex> lock(interface_mutex_);
        
        if (!initialized_) {
            return HALResult<void>::error(errors::invalid_state(2, "Input interface not initialized"));
        }
        
        devices_.clear();
        global_event_callback_ = nullptr;
        device_connection_callback_ = nullptr;
        initialized_ = false;
        
        return HALResult<void>::success();
    }
    
    bool is_initialized() const override {
        return initialized_;
    }
    
    HALResult<void> update(std::chrono::duration<float> delta_time) override {
        std::lock_guard<std::mutex> lock(interface_mutex_);
        
        if (!initialized_) {
            return HALResult<void>::error(errors::invalid_state(3, "Input interface not initialized"));
        }
        
        // Update all devices
        for (auto& device : devices_) {
            device->update(delta_time);
        }
        
        stats_.polling_cycles++;
        return HALResult<void>::success();
    }
    
    HALResult<std::vector<InputResourceHandle>> get_connected_devices() const override {
        std::lock_guard<std::mutex> lock(interface_mutex_);
        
        std::vector<InputResourceHandle> handles;
        for (const auto& device : devices_) {
            if (device->is_connected()) {
                handles.push_back(device->get_handle());
            }
        }
        
        return HALResult<std::vector<InputResourceHandle>>::success(std::move(handles));
    }
    
    HALResult<IInputDevice*> get_device(InputResourceHandle device) override {
        std::lock_guard<std::mutex> lock(interface_mutex_);
        
        for (auto& dev : devices_) {
            if (dev->get_handle().id == device.id) {
                return HALResult<IInputDevice*>::success(dev.get());
            }
        }
        
        return HALResult<IInputDevice*>::error(errors::device_not_found(1, "Device not found"));
    }
    
    HALResult<IInputDevice*> get_device_by_type(InputDeviceType device_type, uint32_t device_index) override {
        std::lock_guard<std::mutex> lock(interface_mutex_);
        
        uint32_t current_index = 0;
        for (auto& device : devices_) {
            if (device->get_descriptor().device_type == device_type && device->is_connected()) {
                if (current_index == device_index) {
                    return HALResult<IInputDevice*>::success(device.get());
                }
                current_index++;
            }
        }
        
        return HALResult<IInputDevice*>::error(errors::device_not_found(2, "Device not found"));
    }
    
    HALResult<void> set_device_connection_callback(DeviceConnectionCallback callback) override {
        device_connection_callback_ = callback;
        return HALResult<void>::success();
    }
    
    HALResult<void> set_hot_plug_enabled(bool enabled) override {
        hot_plug_enabled_ = enabled;
        return HALResult<void>::success();
    }
    
    HALResult<void> set_global_event_callback(InputEventCallback callback) override {
        global_event_callback_ = callback;
        
        // Set callback for all existing devices
        std::lock_guard<std::mutex> lock(interface_mutex_);
        for (auto& device : devices_) {
            device->set_event_callback([this](const InputEvent& event) {
                if (global_event_callback_) {
                    global_event_callback_(event);
                }
                stats_.total_events_processed++;
                
                switch (event.event_type) {
                    case InputEventType::ButtonDown:
                    case InputEventType::ButtonUp:
                    case InputEventType::ButtonRepeat:
                        stats_.button_events++;
                        break;
                    case InputEventType::AnalogChange:
                        stats_.analog_events++;
                        break;
                    case InputEventType::TouchDown:
                    case InputEventType::TouchUp:
                    case InputEventType::TouchMove:
                        stats_.touch_events++;
                        break;
                    case InputEventType::GestureBegin:
                    case InputEventType::GestureEnd:
                    case InputEventType::GestureUpdate:
                        stats_.gesture_events++;
                        break;
                    default:
                        break;
                }
            });
        }
        
        return HALResult<void>::success();
    }
    
    HALResult<uint32_t> poll_events(std::vector<InputEvent>& events, uint32_t max_events) override {
        // Mock implementation - events are delivered via callbacks
        events.clear();
        return HALResult<uint32_t>::success(0);
    }
    
    HALResult<void> clear_event_queue() override {
        return HALResult<void>::success();
    }
    
    HALResult<void> set_event_queue_size(uint32_t size) override {
        return HALResult<void>::success();
    }
    
    // Capability queries
    bool supports_input_capability(InputCapability capability) const override {
        return true; // Mock supports all capabilities
    }
    
    bool supports_device_type(InputDeviceType device_type) const override {
        return true; // Mock supports all device types
    }
    
    std::vector<InputDeviceType> get_supported_device_types() const override {
        return {
            InputDeviceType::Gamepad,
            InputDeviceType::Keyboard,
            InputDeviceType::Mouse,
            InputDeviceType::TouchScreen,
            InputDeviceType::DreamcastController,
            InputDeviceType::PSPController,
            InputDeviceType::MotionController
        };
    }
    
    uint32_t get_max_devices(InputDeviceType device_type) const override {
        return 8; // Mock supports up to 8 devices of each type
    }
    
    InputStats get_stats() const override {
        std::lock_guard<std::mutex> lock(interface_mutex_);
        return stats_;
    }
    
    HALResult<void> reset_stats() override {
        std::lock_guard<std::mutex> lock(interface_mutex_);
        stats_ = {};
        return HALResult<void>::success();
    }
    
    HALResult<double> get_input_latency() const override {
        return HALResult<double>::success(1.0); // 1ms mock latency
    }
    
    HALResult<void> set_low_latency_mode(bool enabled) override {
        return HALResult<void>::success();
    }
    
    // Unimplemented interface methods (return appropriate defaults)
    HALResult<std::unique_ptr<IInputMapper>> create_input_mapper() override {
        return HALResult<std::unique_ptr<IInputMapper>>::error(errors::not_implemented(1, "Mock mapper not implemented"));
    }
    
    IInputMapper* get_global_mapper() override { return nullptr; }
    
    HALResult<InputMapping> load_mapping_from_file(const std::string& file_path) override {
        return HALResult<InputMapping>::error(errors::not_implemented(2, "Not implemented in mock"));
    }
    
    HALResult<void> save_mapping_to_file(const InputMapping& mapping, const std::string& file_path) override {
        return HALResult<void>::error(errors::not_implemented(3, "Not implemented in mock"));
    }
    
    HALResult<std::unique_ptr<IGestureRecognizer>> create_gesture_recognizer() override {
        return HALResult<std::unique_ptr<IGestureRecognizer>>::error(errors::not_implemented(4, "Mock gesture recognizer not implemented"));
    }
    
    IGestureRecognizer* get_global_gesture_recognizer() override { return nullptr; }
    
    HALResult<std::unique_ptr<IHapticController>> create_haptic_controller(InputResourceHandle device) override {
        return HALResult<std::unique_ptr<IHapticController>>::error(errors::not_implemented(5, "Mock haptic controller not implemented"));
    }
    
    IHapticController* get_haptic_controller(InputResourceHandle device) override { return nullptr; }
    
    HALResult<std::unique_ptr<IInputRecorder>> create_input_recorder() override {
        return HALResult<std::unique_ptr<IInputRecorder>>::error(errors::not_implemented(6, "Mock recorder not implemented"));
    }
    
    IInputRecorder* get_global_recorder() override { return nullptr; }
    
    void* get_extension_interface(std::string_view extension_name) override { return nullptr; }
    void* get_vmu_extension() override { return nullptr; }
    void* get_psp_extension() override { return nullptr; }
    void* get_web_gamepad_extension() override { return nullptr; }
    
    // Mock-specific methods
    void add_mock_device(InputDeviceType device_type, const std::string& device_name) {
        std::lock_guard<std::mutex> lock(interface_mutex_);
        
        auto descriptor = input::make_device_descriptor(device_type, device_name);
        auto device = std::make_unique<MockInputDevice>(descriptor);
        
        // Set up event forwarding
        if (global_event_callback_) {
            device->set_event_callback([this](const InputEvent& event) {
                if (global_event_callback_) {
                    global_event_callback_(event);
                }
            });
        }
        
        devices_.push_back(std::move(device));
    }
    
    MockInputDevice* get_mock_device(size_t index) {
        std::lock_guard<std::mutex> lock(interface_mutex_);
        if (index < devices_.size()) {
            return devices_[index].get();
        }
        return nullptr;
    }

private:
    void create_default_devices() {
        add_mock_device(InputDeviceType::Gamepad, "Mock Gamepad");
        add_mock_device(InputDeviceType::Keyboard, "Mock Keyboard");
        add_mock_device(InputDeviceType::Mouse, "Mock Mouse");
        add_mock_device(InputDeviceType::TouchScreen, "Mock Touch Screen");
        add_mock_device(InputDeviceType::DreamcastController, "Mock Dreamcast Controller");
        add_mock_device(InputDeviceType::PSPController, "Mock PSP Controller");
    }
};

} // namespace flight::hal::mock
