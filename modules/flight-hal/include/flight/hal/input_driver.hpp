#ifndef FLIGHT_HAL_INPUT_DRIVER_HPP
#define FLIGHT_HAL_INPUT_DRIVER_HPP

#include "driver.hpp"
#include "result.hpp"
#include <cstdint>
#include <vector>
#include <functional>

namespace flight
{
    namespace hal
    {

        // Input device types
        enum class InputDeviceType : uint32_t
        {
            None,
            Keyboard,
            Mouse,
            Gamepad,
            Joystick,
            Touchscreen,
            Touchpad,
            Accelerometer,
            Gyroscope,
            LightGun,    // Dreamcast
            AnalogStick, // PSP/Vita
            VMU          // Dreamcast Visual Memory Unit
        };

        // Button states
        enum class ButtonState : uint8_t
        {
            Released = 0,
            Pressed = 1,
            Held = 2 // Pressed for multiple frames
        };

        // Standard gamepad buttons (RetroArch mapping)
        enum class GamepadButton : uint32_t
        {
            // Face buttons
            A,
            B,
            X,
            Y,

            // Shoulder buttons
            LeftShoulder,  // L1
            RightShoulder, // R1
            LeftTrigger,   // L2
            RightTrigger,  // R2

            // Control buttons
            Select,     // Back
            Start,      // Options
            LeftStick,  // L3
            RightStick, // R3

            // D-Pad
            DPadUp,
            DPadDown,
            DPadLeft,
            DPadRight,

            // System buttons
            Guide, // Home/Xbox button

            // Platform-specific
            Extra1, // Platform-specific button 1
            Extra2, // Platform-specific button 2
            Extra3, // Platform-specific button 3
            Extra4, // Platform-specific button 4

            Count
        };

        // Analog axes
        enum class GamepadAxis : uint32_t
        {
            LeftX,
            LeftY,
            RightX,
            RightY,
            LeftTrigger,
            RightTrigger,

            // Platform-specific
            Extra1,
            Extra2,

            Count
        };

        // Keyboard keys (subset for gaming)
        enum class KeyCode : uint32_t
        {
            Unknown = 0,

            // Letters
            A,
            B,
            C,
            D,
            E,
            F,
            G,
            H,
            I,
            J,
            K,
            L,
            M,
            N,
            O,
            P,
            Q,
            R,
            S,
            T,
            U,
            V,
            W,
            X,
            Y,
            Z,

            // Numbers
            Num0,
            Num1,
            Num2,
            Num3,
            Num4,
            Num5,
            Num6,
            Num7,
            Num8,
            Num9,

            // Function keys
            F1,
            F2,
            F3,
            F4,
            F5,
            F6,
            F7,
            F8,
            F9,
            F10,
            F11,
            F12,

            // Control keys
            Escape,
            Enter,
            Space,
            Tab,
            Backspace,
            Delete,
            Left,
            Right,
            Up,
            Down,
            PageUp,
            PageDown,
            Home,
            End,
            Insert,

            // Modifiers
            LeftShift,
            RightShift,
            LeftCtrl,
            RightCtrl,
            LeftAlt,
            RightAlt,
            LeftSuper,
            RightSuper, // Windows/Command key

            Count
        };

        // Mouse buttons
        enum class MouseButton : uint32_t
        {
            Left,
            Right,
            Middle,
            Extra1,
            Extra2,

            Count
        };

        // Touch point
        struct TouchPoint
        {
            uint32_t id;
            float x, y;     // Normalized 0-1
            float pressure; // 0-1, if supported
            bool is_active;
        };

        // Input event types
        enum class InputEventType : uint32_t
        {
            None,
            ButtonDown,
            ButtonUp,
            AxisMotion,
            MouseMotion,
            MouseWheel,
            TouchBegin,
            TouchMove,
            TouchEnd,
            DeviceConnected,
            DeviceDisconnected
        };

        // Input event
        struct InputEvent
        {
            InputEventType type;
            InputDeviceType device_type;
            uint32_t device_id;
            uint64_t timestamp; // Platform-specific timestamp

            union
            {
                struct
                {
                    uint32_t button;
                    ButtonState state;
                } button;

                struct
                {
                    uint32_t axis;
                    float value; // -1.0 to 1.0 for sticks, 0.0 to 1.0 for triggers
                } axis;

                struct
                {
                    int32_t x, y;
                    int32_t dx, dy; // Delta movement
                } mouse;

                struct
                {
                    float x, y; // Wheel delta
                } wheel;

                struct
                {
                    uint32_t id;
                    float x, y;
                    float pressure;
                } touch;
            } data;
        };

        // Device info
        struct InputDeviceInfo
        {
            InputDeviceType type;
            uint32_t id;
            const char *name;
            uint32_t button_count;
            uint32_t axis_count;
            bool is_connected;
            bool has_rumble;
            bool has_gyroscope;
            bool has_accelerometer;
        };

        // Input capabilities
        struct InputCapabilities
        {
            uint32_t max_gamepads;
            uint32_t max_touch_points;
            bool has_keyboard;
            bool has_mouse;
            bool has_touch;
            bool has_accelerometer;
            bool has_gyroscope;
            bool has_rumble;
            bool has_analog_triggers;
            bool has_pressure_sensitive_buttons; // PS2/PS3
        };

        // Input callback
        using InputCallback = std::function<void(const InputEvent &event)>;

        // Input driver interface (RetroArch pattern)
        class InputDriver : public Driver
        {
        public:
            // Driver interface
            DriverType type() const override { return DriverType::Input; }

            // Capability queries
            virtual InputCapabilities capabilities() const = 0;
            virtual std::vector<InputDeviceInfo> get_connected_devices() const = 0;
            virtual bool is_device_connected(InputDeviceType type, uint32_t id) const = 0;

            // Polling interface (for platforms without events)
            virtual void poll_input() = 0;

            // Button/key state queries
            virtual ButtonState get_button_state(uint32_t device_id, GamepadButton button) const = 0;
            virtual float get_axis_value(uint32_t device_id, GamepadAxis axis) const = 0;
            virtual ButtonState get_key_state(KeyCode key) const = 0;
            virtual ButtonState get_mouse_button_state(MouseButton button) const = 0;

            // Mouse state
            virtual void get_mouse_position(int32_t &x, int32_t &y) const = 0;
            virtual void set_mouse_position(int32_t x, int32_t y) = 0;
            virtual void show_mouse_cursor(bool show) = 0;
            virtual void capture_mouse(bool capture) = 0;

            // Touch state (if supported)
            virtual uint32_t get_touch_point_count() const = 0;
            virtual std::vector<TouchPoint> get_touch_points() const = 0;

            // Event-based interface
            virtual void set_event_callback(InputCallback callback) = 0;
            virtual void remove_event_callback() = 0;

            // Rumble/haptic feedback
            virtual bool supports_rumble(uint32_t device_id) const = 0;
            virtual Result<void> set_rumble(uint32_t device_id, float low_freq, float high_freq, uint32_t duration_ms) = 0;
            virtual Result<void> stop_rumble(uint32_t device_id) = 0;

            // Motion sensors (if supported)
            virtual bool supports_motion(uint32_t device_id) const = 0;
            virtual Result<void> get_accelerometer(uint32_t device_id, float &x, float &y, float &z) = 0;
            virtual Result<void> get_gyroscope(uint32_t device_id, float &x, float &y, float &z) = 0;

            // Platform-specific features

            // Dreamcast VMU
            virtual bool has_vmu(uint32_t device_id) const { return false; }
            virtual Result<void> draw_vmu_screen(uint32_t device_id, const uint8_t *bitmap_48x32) { return ErrorCode::NotSupported; }

            // PSP/Vita analog stick calibration
            virtual Result<void> calibrate_analog_stick(uint32_t device_id) { return ErrorCode::NotSupported; }

            // Light gun calibration (Dreamcast)
            virtual Result<void> calibrate_light_gun(uint32_t device_id) { return ErrorCode::NotSupported; }

            // Input mapping/remapping
            virtual Result<void> remap_button(uint32_t device_id, GamepadButton from, GamepadButton to) = 0;
            virtual Result<void> reset_mappings(uint32_t device_id) = 0;

            // Dead zone configuration
            virtual void set_axis_deadzone(GamepadAxis axis, float deadzone) = 0;
            virtual float get_axis_deadzone(GamepadAxis axis) const = 0;
        };

        // Helper functions
        inline const char *gamepad_button_to_string(GamepadButton button)
        {
            switch (button)
            {
            case GamepadButton::A:
                return "A";
            case GamepadButton::B:
                return "B";
            case GamepadButton::X:
                return "X";
            case GamepadButton::Y:
                return "Y";
            case GamepadButton::LeftShoulder:
                return "L1";
            case GamepadButton::RightShoulder:
                return "R1";
            case GamepadButton::LeftTrigger:
                return "L2";
            case GamepadButton::RightTrigger:
                return "R2";
            case GamepadButton::Select:
                return "Select";
            case GamepadButton::Start:
                return "Start";
            case GamepadButton::LeftStick:
                return "L3";
            case GamepadButton::RightStick:
                return "R3";
            case GamepadButton::DPadUp:
                return "DPad Up";
            case GamepadButton::DPadDown:
                return "DPad Down";
            case GamepadButton::DPadLeft:
                return "DPad Left";
            case GamepadButton::DPadRight:
                return "DPad Right";
            case GamepadButton::Guide:
                return "Guide";
            default:
                return "Unknown";
            }
        }

        inline const char *gamepad_axis_to_string(GamepadAxis axis)
        {
            switch (axis)
            {
            case GamepadAxis::LeftX:
                return "Left X";
            case GamepadAxis::LeftY:
                return "Left Y";
            case GamepadAxis::RightX:
                return "Right X";
            case GamepadAxis::RightY:
                return "Right Y";
            case GamepadAxis::LeftTrigger:
                return "Left Trigger";
            case GamepadAxis::RightTrigger:
                return "Right Trigger";
            default:
                return "Unknown";
            }
        }

        // Platform-specific default dead zones
        namespace input_defaults
        {
            constexpr float DEFAULT_STICK_DEADZONE = 0.15f;
            constexpr float DEFAULT_TRIGGER_DEADZONE = 0.05f;

            // Dreamcast has very precise analog stick
            constexpr float DREAMCAST_STICK_DEADZONE = 0.10f;

            // PSP analog nub needs larger dead zone
            constexpr float PSP_STICK_DEADZONE = 0.20f;

            // Modern controllers are more precise
            constexpr float MODERN_STICK_DEADZONE = 0.08f;
        }

    } // namespace hal
} // namespace flight

#endif // FLIGHT_HAL_INPUT_DRIVER_HPP
