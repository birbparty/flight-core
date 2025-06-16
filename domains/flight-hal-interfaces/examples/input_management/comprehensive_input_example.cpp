/**
 * @file comprehensive_input_example.cpp
 * @brief Comprehensive Input Interface Example
 * 
 * Demonstrates the unified input interface capabilities including:
 * - Device enumeration and management
 * - Event-driven and polling input handling
 * - Platform-specific device support (Dreamcast, PSP)
 * - Touch and gesture recognition
 * - Input mapping and configuration
 * - Haptic feedback
 * - Performance monitoring
 */

#include "../../include/flight/hal/interfaces/input.hpp"
#include "../drivers/mock_input_driver.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>

using namespace flight::hal;
using namespace std::chrono_literals;

class InputExample {
private:
    std::unique_ptr<mock::MockInputInterface> input_interface_;
    bool running_;
    
public:
    InputExample() : running_(false) {
        input_interface_ = std::make_unique<mock::MockInputInterface>();
    }
    
    void run() {
        std::cout << "=== Flight HAL Input Interface Example ===\n\n";
        
        // Initialize the input system
        if (!initialize_input_system()) {
            return;
        }
        
        // Demonstrate various input features
        demonstrate_device_enumeration();
        demonstrate_capability_detection();
        demonstrate_event_driven_input();
        demonstrate_polling_input();
        demonstrate_platform_specific_features();
        demonstrate_input_mapping();
        demonstrate_haptic_feedback();
        demonstrate_performance_monitoring();
        
        // Clean shutdown
        shutdown_input_system();
        
        std::cout << "\n=== Example completed successfully ===\n";
    }

private:
    bool initialize_input_system() {
        std::cout << "1. Initializing Input System\n";
        std::cout << "   - Setting up unified input interface...\n";
        
        auto result = input_interface_->initialize();
        if (!result.is_success()) {
            std::cout << "   ERROR: Failed to initialize input system: " 
                      << result.error().message() << "\n";
            return false;
        }
        
        std::cout << "   ✓ Input system initialized successfully\n\n";
        return true;
    }
    
    void demonstrate_device_enumeration() {
        std::cout << "2. Device Enumeration and Management\n";
        
        // Get connected devices
        auto devices_result = input_interface_->get_connected_devices();
        if (devices_result.is_success()) {
            const auto& devices = devices_result.value();
            std::cout << "   - Found " << devices.size() << " connected devices:\n";
            
            for (size_t i = 0; i < devices.size(); ++i) {
                auto device_result = input_interface_->get_device(devices[i]);
                if (device_result.is_success()) {
                    auto* device = device_result.value();
                    const auto& desc = device->get_descriptor();
                    
                    std::cout << "     [" << i << "] " << desc.device_name 
                              << " (" << input::to_string(desc.device_type) << ")\n";
                    std::cout << "         - Buttons: " << desc.button_count 
                              << ", Axes: " << desc.axis_count << "\n";
                    std::cout << "         - Haptic: " << (desc.supports_haptic ? "Yes" : "No")
                              << ", Motion: " << (desc.supports_motion ? "Yes" : "No") << "\n";
                    
                    if (desc.has_vmu_slots) {
                        std::cout << "         - VMU Support: Available\n";
                    }
                    if (desc.supports_analog_mode) {
                        std::cout << "         - Analog Mode: Supported\n";
                    }
                }
            }
        }
        std::cout << "\n";
    }
    
    void demonstrate_capability_detection() {
        std::cout << "3. Capability Detection\n";
        
        // Check various input capabilities
        const InputCapability capabilities[] = {
            InputCapability::DigitalButtons,
            InputCapability::AnalogSticks,
            InputCapability::TouchInput,
            InputCapability::GestureRecognition,
            InputCapability::HapticFeedback,
            InputCapability::VMUSupport,
            InputCapability::PSPAnalogMode,
            InputCapability::EventDriven,
            InputCapability::PollingMode
        };
        
        std::cout << "   - Supported capabilities:\n";
        for (auto cap : capabilities) {
            bool supported = input_interface_->supports_input_capability(cap);
            std::cout << "     " << input::to_string(cap) << ": " 
                      << (supported ? "✓" : "✗") << "\n";
        }
        
        // Check supported device types
        auto supported_types = input_interface_->get_supported_device_types();
        std::cout << "   - Supported device types: ";
        for (size_t i = 0; i < supported_types.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << input::to_string(supported_types[i]);
        }
        std::cout << "\n\n";
    }
    
    void demonstrate_event_driven_input() {
        std::cout << "4. Event-Driven Input Handling\n";
        std::cout << "   - Setting up global event callback...\n";
        
        // Set up event callback
        input_interface_->set_global_event_callback([this](const InputEvent& event) {
            handle_input_event(event);
        });
        
        // Simulate some input events using mock devices
        std::cout << "   - Simulating input events...\n";
        
        auto gamepad = input_interface_->get_mock_device(0); // First device (Mock Gamepad)
        if (gamepad) {
            // Simulate button presses
            gamepad->simulate_button_press(InputButton::ButtonA, true);
            std::this_thread::sleep_for(50ms);
            gamepad->simulate_button_press(InputButton::ButtonA, false);
            
            // Simulate analog stick movement
            gamepad->simulate_axis_change(InputAxis::LeftStickX, 0.8f);
            gamepad->simulate_axis_change(InputAxis::LeftStickY, -0.5f);
            std::this_thread::sleep_for(50ms);
            gamepad->simulate_axis_change(InputAxis::LeftStickX, 0.0f);
            gamepad->simulate_axis_change(InputAxis::LeftStickY, 0.0f);
        }
        
        // Get touch screen device and simulate touch
        auto touch_result = input_interface_->get_device_by_type(InputDeviceType::TouchScreen, 0);
        if (touch_result.is_success()) {
            auto* touch_device = static_cast<mock::MockInputDevice*>(touch_result.value());
            
            // Simulate touch sequence
            touch_device->simulate_touch(0, Vector2(0.5f, 0.3f), 0.8f, InputEventType::TouchDown);
            std::this_thread::sleep_for(50ms);
            touch_device->simulate_touch(0, Vector2(0.6f, 0.4f), 0.9f, InputEventType::TouchMove);
            std::this_thread::sleep_for(50ms);
            touch_device->simulate_touch(0, Vector2(0.6f, 0.4f), 0.0f, InputEventType::TouchUp);
        }
        
        std::cout << "   ✓ Event handling demonstration completed\n\n";
    }
    
    void demonstrate_polling_input() {
        std::cout << "5. Polling Input Mode\n";
        std::cout << "   - Demonstrating frame-based input polling...\n";
        
        auto gamepad_result = input_interface_->get_device_by_type(InputDeviceType::Gamepad, 0);
        if (gamepad_result.is_success()) {
            auto* gamepad = gamepad_result.value();
            
            // Simulate some input state
            auto* mock_gamepad = static_cast<mock::MockInputDevice*>(gamepad);
            mock_gamepad->simulate_button_press(InputButton::Start, true);
            mock_gamepad->simulate_axis_change(InputAxis::RightStickX, 0.3f);
            
            // Poll current state
            auto state = gamepad->get_current_state();
            
            std::cout << "   - Current gamepad state:\n";
            std::cout << "     Start button: " 
                      << (input::is_button_pressed(state, InputButton::Start) ? "Pressed" : "Released") << "\n";
            std::cout << "     Right stick X: " 
                      << std::fixed << std::setprecision(2) 
                      << input::get_axis_value(state, InputAxis::RightStickX) << "\n";
            
            // Clean up
            mock_gamepad->simulate_button_press(InputButton::Start, false);
            mock_gamepad->simulate_axis_change(InputAxis::RightStickX, 0.0f);
        }
        
        std::cout << "   ✓ Polling demonstration completed\n\n";
    }
    
    void demonstrate_platform_specific_features() {
        std::cout << "6. Platform-Specific Features\n";
        
        // Dreamcast controller features
        auto dreamcast_result = input_interface_->get_device_by_type(InputDeviceType::DreamcastController, 0);
        if (dreamcast_result.is_success()) {
            auto* dreamcast = dreamcast_result.value();
            const auto& desc = dreamcast->get_descriptor();
            
            std::cout << "   - Dreamcast Controller detected:\n";
            std::cout << "     VMU slots: " << (desc.has_vmu_slots ? "Available" : "Not available") << "\n";
            
            if (desc.has_vmu_slots) {
                auto* vmu_ext = input_interface_->get_vmu_extension();
                if (vmu_ext) {
                    std::cout << "     VMU extension interface: Available\n";
                } else {
                    std::cout << "     VMU extension interface: Not implemented in mock\n";
                }
            }
        }
        
        // PSP controller features
        auto psp_result = input_interface_->get_device_by_type(InputDeviceType::PSPController, 0);
        if (psp_result.is_success()) {
            auto* psp = psp_result.value();
            const auto& desc = psp->get_descriptor();
            
            std::cout << "   - PSP Controller detected:\n";
            std::cout << "     Analog mode: " << (desc.supports_analog_mode ? "Supported" : "Not supported") << "\n";
            
            if (desc.supports_analog_mode) {
                auto* psp_ext = input_interface_->get_psp_extension();
                if (psp_ext) {
                    std::cout << "     PSP extension interface: Available\n";
                } else {
                    std::cout << "     PSP extension interface: Not implemented in mock\n";
                }
            }
        }
        
        std::cout << "   ✓ Platform-specific features checked\n\n";
    }
    
    void demonstrate_input_mapping() {
        std::cout << "7. Input Mapping and Configuration\n";
        
        // Create default mapping
        auto default_mapping = input::make_default_mapping("Game Controls", "gameplay");
        
        std::cout << "   - Created default input mapping: \"" << default_mapping.mapping_name << "\"\n";
        std::cout << "   - Context: " << default_mapping.context << "\n";
        std::cout << "   - Button mappings:\n";
        
        for (const auto& [button, action] : default_mapping.button_actions) {
            std::cout << "     " << input::to_string(button) << " -> " << action << "\n";
        }
        
        std::cout << "   - Axis mappings:\n";
        for (const auto& [axis, action] : default_mapping.axis_actions) {
            std::cout << "     " << input::to_string(axis) << " -> " << action << "\n";
        }
        
        std::cout << "   - Dead zones configured for analog inputs\n";
        std::cout << "   ✓ Input mapping demonstration completed\n\n";
    }
    
    void demonstrate_haptic_feedback() {
        std::cout << "8. Haptic Feedback\n";
        
        auto gamepad_result = input_interface_->get_device_by_type(InputDeviceType::Gamepad, 0);
        if (gamepad_result.is_success()) {
            auto* gamepad = gamepad_result.value();
            const auto& desc = gamepad->get_descriptor();
            
            if (desc.supports_haptic) {
                std::cout << "   - Haptic feedback supported on gamepad\n";
                
                // Create haptic effects
                auto rumble_effect = input::make_haptic_effect(HapticType::Rumble, 0.8f, 500ms);
                auto pulse_effect = input::make_haptic_effect(HapticType::Pulse, 0.6f, 200ms);
                
                std::cout << "   - Playing rumble effect (intensity: " 
                          << rumble_effect.intensity << ", duration: " 
                          << rumble_effect.duration.count() << "ms)\n";
                gamepad->play_haptic_effect(rumble_effect);
                
                std::this_thread::sleep_for(100ms);
                
                std::cout << "   - Playing pulse effect (intensity: " 
                          << pulse_effect.intensity << ", duration: " 
                          << pulse_effect.duration.count() << "ms)\n";
                gamepad->play_haptic_effect(pulse_effect);
                
                std::this_thread::sleep_for(100ms);
                gamepad->stop_haptic_effects();
                std::cout << "   - Stopped all haptic effects\n";
            } else {
                std::cout << "   - Haptic feedback not supported on this device\n";
            }
        }
        
        std::cout << "   ✓ Haptic feedback demonstration completed\n\n";
    }
    
    void demonstrate_performance_monitoring() {
        std::cout << "9. Performance Monitoring\n";
        
        // Update the input system a few times to generate stats
        for (int i = 0; i < 5; ++i) {
            input_interface_->update(16.67ms); // Simulate 60 FPS
            std::this_thread::sleep_for(5ms);
        }
        
        // Get performance statistics
        auto stats = input_interface_->get_stats();
        auto latency_result = input_interface_->get_input_latency();
        
        std::cout << "   - Input Performance Statistics:\n";
        std::cout << "     Total events processed: " << stats.total_events_processed << "\n";
        std::cout << "     Button events: " << stats.button_events << "\n";
        std::cout << "     Analog events: " << stats.analog_events << "\n";
        std::cout << "     Touch events: " << stats.touch_events << "\n";
        std::cout << "     Gesture events: " << stats.gesture_events << "\n";
        std::cout << "     Polling cycles: " << stats.polling_cycles << "\n";
        std::cout << "     Connected devices: " << stats.connected_devices << "\n";
        
        if (latency_result.is_success()) {
            std::cout << "     Average input latency: " 
                      << std::fixed << std::setprecision(1) 
                      << latency_result.value() << "ms\n";
        }
        
        std::cout << "   ✓ Performance monitoring completed\n\n";
    }
    
    void handle_input_event(const InputEvent& event) {
        static size_t event_count = 0;
        
        if (event_count < 10) { // Limit output to first 10 events
            std::cout << "     Event [" << ++event_count << "]: " 
                      << input::to_string(event.event_type);
            
            switch (event.event_type) {
                case InputEventType::ButtonDown:
                case InputEventType::ButtonUp:
                    std::cout << " - " << input::to_string(event.button_event.button)
                              << " (" << (event.button_event.pressed ? "pressed" : "released") << ")";
                    break;
                    
                case InputEventType::AnalogChange:
                    std::cout << " - " << input::to_string(event.analog_event.axis)
                              << " = " << std::fixed << std::setprecision(2) 
                              << event.analog_event.value;
                    break;
                    
                case InputEventType::TouchDown:
                case InputEventType::TouchUp:
                case InputEventType::TouchMove:
                    std::cout << " - Touch " << event.touch_event.touch_id
                              << " at (" << std::fixed << std::setprecision(2)
                              << event.touch_event.position.x << ", " 
                              << event.touch_event.position.y << ")";
                    break;
                    
                case InputEventType::DeviceConnected:
                case InputEventType::DeviceDisconnected:
                    std::cout << " - " << event.device_event.device_name;
                    break;
                    
                default:
                    break;
            }
            
            std::cout << "\n";
        }
    }
    
    void shutdown_input_system() {
        std::cout << "10. Shutting Down Input System\n";
        
        auto result = input_interface_->shutdown();
        if (result.is_success()) {
            std::cout << "    ✓ Input system shut down successfully\n";
        } else {
            std::cout << "    ERROR: Failed to shutdown input system: " 
                      << result.error().message() << "\n";
        }
    }
};

int main() {
    try {
        InputExample example;
        example.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}
