/**
 * @file comprehensive_event_example.cpp
 * @brief Comprehensive HAL Event System Usage Example
 * 
 * Demonstrates event publishing, subscription, filtering, and platform integration
 * with the Flight HAL event system.
 */

#include "../../include/flight/hal/core/event_system.hpp"
#include "../../include/flight/hal/core/hal_logging.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <memory>

using namespace flight::hal::core;
using namespace flight::hal;

/**
 * @brief Example event subscriber for system monitoring
 */
class SystemMonitor : public IEventSubscriber {
public:
    SystemMonitor() = default;
    
    void on_event(const Event& event) override {
        std::cout << "[SystemMonitor] Received event ID: " << event.metadata().event_id
                  << ", Category: " << static_cast<int>(event.metadata().category)
                  << ", Severity: " << static_cast<int>(event.metadata().severity)
                  << ", Source: " << event.metadata().source_id
                  << ", Description: " << event.metadata().description << std::endl;
        
        // Handle specific event types
        if (auto* hw_data = event.data_as<HardwareEventData>()) {
            handle_hardware_event(*hw_data);
        } else if (auto* sys_data = event.data_as<SystemEventData>()) {
            handle_system_event(*sys_data);
        } else if (auto* driver_data = event.data_as<DriverEventData>()) {
            handle_driver_event(*driver_data);
        }
    }
    
    std::string get_subscriber_id() const override {
        return "SystemMonitor";
    }
    
    EventFilter get_event_filter() const override {
        EventFilter filter;
        filter.add_category(EventCategory::System);
        filter.add_category(EventCategory::Hardware);
        filter.set_min_severity(EventSeverity::Warning);
        return filter;
    }
    
    bool wants_async_delivery() const override {
        return true;
    }

private:
    void handle_hardware_event(const HardwareEventData& data) {
        switch (data.type) {
            case HardwareEventType::ThermalCritical:
                std::cout << "  [ALERT] Critical thermal event on device: " << data.device_name << std::endl;
                break;
            case HardwareEventType::PowerCritical:
                std::cout << "  [ALERT] Critical power event detected!" << std::endl;
                break;
            case HardwareEventType::DeviceDisconnected:
                std::cout << "  [INFO] Device disconnected: " << data.device_name << std::endl;
                break;
            default:
                std::cout << "  [INFO] Hardware event type: " << static_cast<int>(data.type) << std::endl;
                break;
        }
    }
    
    void handle_system_event(const SystemEventData& data) {
        switch (data.type) {
            case SystemEventType::MemoryCritical:
                std::cout << "  [ALERT] Critical memory condition!" << std::endl;
                break;
            case SystemEventType::CPULoadHigh:
                std::cout << "  [WARNING] High CPU load detected" << std::endl;
                break;
            default:
                std::cout << "  [INFO] System event type: " << static_cast<int>(data.type) << std::endl;
                break;
        }
    }
    
    void handle_driver_event(const DriverEventData& data) {
        std::cout << "  [INFO] Driver event: " << data.driver_name 
                  << ", Type: " << static_cast<int>(data.type) << std::endl;
    }
};

/**
 * @brief Example performance monitor for high-frequency events
 */
class PerformanceMonitor : public IEventSubscriber {
public:
    PerformanceMonitor() = default;
    
    void on_event(const Event& event) override {
        event_count_++;
        
        if (event_count_ % 100 == 0) {
            std::cout << "[PerformanceMonitor] Processed " << event_count_ 
                      << " performance events" << std::endl;
        }
    }
    
    std::string get_subscriber_id() const override {
        return "PerformanceMonitor";
    }
    
    EventFilter get_event_filter() const override {
        EventFilter filter;
        filter.add_category(EventCategory::Performance);
        return filter;
    }
    
    bool wants_async_delivery() const override {
        return true;
    }
    
    uint64_t get_event_count() const { return event_count_; }

private:
    std::atomic<uint64_t> event_count_{0};
};

/**
 * @brief Mock platform event source for demonstration
 */
class MockPlatformEventSource : public IPlatformEventSource {
public:
    MockPlatformEventSource() = default;
    
    HALResult<void> initialize() override {
        std::cout << "[MockPlatformSource] Initializing platform event source" << std::endl;
        return HALResult<void>::success();
    }
    
    HALResult<void> shutdown() override {
        std::cout << "[MockPlatformSource] Shutting down platform event source" << std::endl;
        stop_monitoring();
        return HALResult<void>::success();
    }
    
    HALResult<void> start_monitoring() override {
        std::cout << "[MockPlatformSource] Starting event monitoring" << std::endl;
        monitoring_ = true;
        
        // Start background thread to generate mock events
        monitor_thread_ = std::thread([this]() {
            int event_counter = 0;
            while (monitoring_) {
                std::this_thread::sleep_for(std::chrono::seconds(2));
                
                if (!monitoring_) break;
                
                // Generate different types of events
                if (event_counter % 3 == 0) {
                    generate_thermal_event();
                } else if (event_counter % 3 == 1) {
                    generate_system_event();
                } else {
                    generate_driver_event();
                }
                
                event_counter++;
            }
        });
        
        return HALResult<void>::success();
    }
    
    HALResult<void> stop_monitoring() override {
        monitoring_ = false;
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
        return HALResult<void>::success();
    }
    
    std::unordered_set<EventCategory> get_supported_categories() const override {
        return {EventCategory::Hardware, EventCategory::System, EventCategory::Driver};
    }
    
    void set_event_callback(std::function<void(const Event&)> callback) override {
        event_callback_ = callback;
    }

private:
    void generate_thermal_event() {
        HardwareEventData data;
        data.type = HardwareEventType::ThermalThreshold;
        data.device_id = "cpu0";
        data.device_name = "Intel Core i7";
        data.value = 75.5; // Temperature in Celsius
        data.unit = "°C";
        
        Event event = create_hardware_event("platform.thermal", data.type, 
                                           EventSeverity::Warning, 
                                           "CPU temperature approaching threshold");
        event.set_data(data);
        
        if (event_callback_) {
            event_callback_(event);
        }
    }
    
    void generate_system_event() {
        SystemEventData data;
        data.type = SystemEventType::MemoryPressure;
        data.subsystem = "memory";
        data.value = static_cast<int64_t>(85); // Memory usage percentage
        data.unit = "%";
        data.system_info["available_mb"] = "2048";
        data.system_info["total_mb"] = "16384";
        
        Event event = create_system_event("platform.memory", data.type,
                                         EventSeverity::Warning,
                                         "Memory usage is high");
        event.set_data(data);
        
        if (event_callback_) {
            event_callback_(event);
        }
    }
    
    void generate_driver_event() {
        DriverEventData data;
        data.type = DriverEventType::LatencySpike;
        data.driver_id = "graphics_driver";
        data.driver_name = "NVIDIA RTX Driver";
        data.metric_value = 16.7; // Latency in milliseconds
        
        Event event = create_driver_event("nvidia.driver", data.type,
                                         EventSeverity::Info,
                                         "Frame time spike detected");
        event.set_data(data);
        
        if (event_callback_) {
            event_callback_(event);
        }
    }
    
    std::atomic<bool> monitoring_{false};
    std::thread monitor_thread_;
    std::function<void(const Event&)> event_callback_;
};

/**
 * @brief Demonstrate basic event system usage
 */
void demonstrate_basic_events() {
    std::cout << "\n=== Basic Event System Demo ===" << std::endl;
    
    auto& event_system = HALEventSystem::instance();
    
    // Create and subscribe monitors
    auto system_monitor = std::make_shared<SystemMonitor>();
    auto perf_monitor = std::make_shared<PerformanceMonitor>();
    
    auto sys_subscription = event_system.subscribe(system_monitor);
    auto perf_subscription = event_system.subscribe(perf_monitor);
    
    if (!sys_subscription.is_ok()) {
        std::cout << "Failed to subscribe system monitor: " << sys_subscription.error().message() << std::endl;
        return;
    }
    
    if (!perf_subscription.is_ok()) {
        std::cout << "Failed to subscribe performance monitor: " << perf_subscription.error().message() << std::endl;
        return;
    }
    
    std::cout << "Subscribed monitors successfully" << std::endl;
    
    // Publish various events
    std::cout << "\nPublishing hardware events..." << std::endl;
    
    HardwareEventData hw_data;
    hw_data.type = HardwareEventType::ThermalCritical;
    hw_data.device_id = "gpu0";
    hw_data.device_name = "NVIDIA RTX 4090";
    hw_data.value = 95.0;
    hw_data.unit = "°C";
    
    event_system.publish_hardware_event("gpu.thermal", hw_data, EventSeverity::Critical,
                                        "GPU overheating detected");
    
    std::cout << "\nPublishing system events..." << std::endl;
    
    SystemEventData sys_data;
    sys_data.type = SystemEventType::MemoryCritical;
    sys_data.subsystem = "system_memory";
    sys_data.value = static_cast<int64_t>(95);
    sys_data.unit = "%";
    
    event_system.publish_system_event("system.memory", sys_data, EventSeverity::Critical,
                                      "System memory critically low");
    
    std::cout << "\nPublishing driver events..." << std::endl;
    
    DriverEventData driver_data;
    driver_data.type = DriverEventType::DriverError;
    driver_data.driver_id = "audio_driver";
    driver_data.driver_name = "Realtek Audio";
    driver_data.error_message = "Device initialization failed";
    
    event_system.publish_driver_event("audio.driver", driver_data, EventSeverity::Error,
                                      "Audio driver encountered an error");
    
    // Give events time to process
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Show statistics
    auto stats = event_system.get_stats();
    std::cout << "\nEvent System Statistics:" << std::endl;
    std::cout << "  Events Generated: " << stats.events_generated << std::endl;
    std::cout << "  Events Dispatched: " << stats.events_dispatched << std::endl;
    std::cout << "  Active Subscriptions: " << stats.active_subscriptions << std::endl;
    std::cout << "  Average Dispatch Time: " << stats.average_dispatch_time_ms << " ms" << std::endl;
    
    // Clean up
    event_system.unsubscribe(sys_subscription.value());
    event_system.unsubscribe(perf_subscription.value());
}

/**
 * @brief Demonstrate platform event source integration
 */
void demonstrate_platform_integration() {
    std::cout << "\n=== Platform Integration Demo ===" << std::endl;
    
    auto& event_system = HALEventSystem::instance();
    
    // Create and subscribe a monitor
    auto system_monitor = std::make_shared<SystemMonitor>();
    auto subscription = event_system.subscribe(system_monitor);
    
    if (!subscription.is_ok()) {
        std::cout << "Failed to subscribe monitor: " << subscription.error().message() << std::endl;
        return;
    }
    
    // Register mock platform source
    auto platform_source = std::make_unique<MockPlatformEventSource>();
    auto register_result = event_system.register_platform_source(std::move(platform_source));
    
    if (!register_result.is_ok()) {
        std::cout << "Failed to register platform source: " << register_result.error().message() << std::endl;
        return;
    }
    
    std::cout << "Platform source registered successfully" << std::endl;
    
    // Let the platform source generate events for a while
    std::cout << "Monitoring platform events for 10 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(10));
    
    // Show final statistics
    auto stats = event_system.get_stats();
    std::cout << "\nFinal Statistics:" << std::endl;
    std::cout << "  Platform Events: " << stats.platform_events << std::endl;
    std::cout << "  Total Events Generated: " << stats.events_generated << std::endl;
    std::cout << "  Events Dispatched: " << stats.events_dispatched << std::endl;
    
    // Clean up
    event_system.unsubscribe(subscription.value());
}

/**
 * @brief Demonstrate event filtering capabilities
 */
void demonstrate_event_filtering() {
    std::cout << "\n=== Event Filtering Demo ===" << std::endl;
    
    auto& event_system = HALEventSystem::instance();
    
    // Create filter for critical events only
    EventFilter critical_filter;
    critical_filter.set_min_severity(EventSeverity::Critical);
    critical_filter.add_category(EventCategory::Hardware);
    critical_filter.add_category(EventCategory::System);
    
    // Create monitor with filter
    auto monitor = std::make_shared<SystemMonitor>();
    auto subscription = event_system.subscribe(monitor, critical_filter);
    
    if (!subscription.is_ok()) {
        std::cout << "Failed to subscribe with filter: " << subscription.error().message() << std::endl;
        return;
    }
    
    std::cout << "Subscribed with critical-only filter" << std::endl;
    
    // Publish events of different severities
    HardwareEventData hw_data;
    hw_data.type = HardwareEventType::DeviceConnected;
    hw_data.device_name = "USB Device";
    
    // This should be filtered out (Info severity)
    event_system.publish_hardware_event("usb.device", hw_data, EventSeverity::Info,
                                        "USB device connected");
    
    // This should be filtered out (Warning severity)
    hw_data.type = HardwareEventType::ThermalThreshold;
    event_system.publish_hardware_event("cpu.thermal", hw_data, EventSeverity::Warning,
                                        "CPU temperature warning");
    
    // This should pass through (Critical severity)
    hw_data.type = HardwareEventType::ThermalCritical;
    event_system.publish_hardware_event("cpu.thermal", hw_data, EventSeverity::Critical,
                                        "CPU critical temperature");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    auto stats = event_system.get_stats();
    std::cout << "Events filtered: " << stats.events_filtered << std::endl;
    
    event_system.unsubscribe(subscription.value());
}

int main() {
    std::cout << "Flight HAL Event System Comprehensive Example" << std::endl;
    
    // Initialize the event system
    auto& event_system = HALEventSystem::instance();
    auto init_result = event_system.initialize();
    
    if (!init_result.is_ok()) {
        std::cout << "Failed to initialize event system: " << init_result.error().message() << std::endl;
        return 1;
    }
    
    std::cout << "Event system initialized successfully" << std::endl;
    
    // Enable event batching for better performance
    event_system.set_batching_enabled(true, 5, 10);
    
    try {
        // Run demonstrations
        demonstrate_basic_events();
        demonstrate_event_filtering();
        demonstrate_platform_integration();
        
    } catch (const std::exception& e) {
        std::cout << "Exception during demo: " << e.what() << std::endl;
    }
    
    // Shutdown the event system
    auto shutdown_result = event_system.shutdown();
    if (!shutdown_result.is_ok()) {
        std::cout << "Failed to shutdown event system: " << shutdown_result.error().message() << std::endl;
        return 1;
    }
    
    std::cout << "\nEvent system shutdown successfully" << std::endl;
    std::cout << "Demo completed!" << std::endl;
    
    return 0;
}
