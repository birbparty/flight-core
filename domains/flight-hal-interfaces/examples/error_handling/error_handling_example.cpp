/**
 * @file error_handling_example.cpp
 * @brief Comprehensive Example of Flight HAL Error Handling Patterns
 * 
 * Demonstrates the zero-allocation, union-based error handling system
 * with various patterns and use cases.
 */

#include "../../include/flight/hal/core/hal_result.hpp"
#include "../../include/flight/hal/core/hal_error.hpp"
#include "../../include/flight/hal/core/hal_error_macros.hpp"
#include "../../include/flight/hal/core/hal_logging.hpp"
#include <iostream>
#include <memory>

using namespace flight::hal;

// Example device interface using new error handling
class ExampleDevice {
public:
    struct DeviceStatus {
        int temperature;
        int voltage;
        bool is_healthy;
    };

    /**
     * @brief Initialize the device
     * @return HALResult indicating success or failure
     */
    HALVoidResult initialize() {
        // Simulate initialization that might fail
        if (initialized_) {
            return HALVoidResult::error(
                errors::invalid_state(1, "Device already initialized"));
        }
        
        // Simulate hardware check
        if (!hardware_present()) {
            return HALVoidResult::error(
                errors::device_not_found(2, "Hardware not detected"));
        }
        
        initialized_ = true;
        HAL_LOG_MESSAGE(LogLevel::Info, "Device initialized successfully");
        return HALVoidResult::success();
    }
    
    /**
     * @brief Read data from device
     * @param address Memory address to read from
     * @return HALResult containing data or error
     */
    HALResult<uint32_t> read_data(uint32_t address) {
        // Manual validation instead of macros for different return types
        if (!initialized_) {
            return HALResult<uint32_t>::error(
                HALError(HALErrorCategory::Validation, 10, "Device not initialized"));
        }
        if (address >= 0x1000) {
            return HALResult<uint32_t>::error(
                HALError(HALErrorCategory::Configuration, 11, "Address out of range"));
        }
        
        // Simulate reading data
        if (address == 0x500) {
            // Simulate hardware error
            return HALResult<uint32_t>::error(
                errors::device_busy(20, "Hardware timeout"));
        }
        
        // Success case
        uint32_t data = 0x12345678 + address;
        return HALResult<uint32_t>::success(data);
    }
    
    /**
     * @brief Write data to device
     * @param address Memory address to write to
     * @param data Data to write
     * @return HALResult indicating success or failure
     */
    HALVoidResult write_data(uint32_t address, uint32_t data) {
        HAL_ENSURE_HAL(initialized_, HALErrorCategory::Validation, 10, "Device not initialized");
        HAL_ENSURE_HAL(address < 0x1000, HALErrorCategory::Configuration, 11, "Address out of range");
        
        // Simulate write operation
        if (data == 0xDEADBEEF) {
            return HALVoidResult::error(
                errors::parameter_out_of_range(30, "Invalid data value"));
        }
        
        return HALVoidResult::success();
    }
    
    /**
     * @brief Get device status
     * @return HALResult containing status information
     */
    HALResult<DeviceStatus> get_status() const {
        if (!initialized_) {
            return HALResult<DeviceStatus>::error(
                errors::invalid_state(40, "Device not initialized"));
        }
        
        DeviceStatus status{
            .temperature = 45,
            .voltage = 3300,
            .is_healthy = true
        };
        
        return HALResult<DeviceStatus>::success(std::move(status));
    }

private:
    bool initialized_ = false;
    
    bool hardware_present() const {
        // Simulate hardware detection
        return true;
    }
};

// Example service that uses multiple operations with error handling
class DeviceService {
public:
    explicit DeviceService(std::shared_ptr<ExampleDevice> device) 
        : device_(std::move(device)) {}
    
    /**
     * @brief Initialize and configure the device
     * @return HALResult indicating success or failure
     */
    HALVoidResult setup_device() {
        // Use HAL_TRY_VOID for error propagation
        HAL_TRY_VOID(device_->initialize());
        
        // Configure device with multiple operations
        auto config_result = configure_device();
        if (config_result.is_err()) {
            HAL_LOG_ERROR(config_result.error());
            return config_result;
        }
        
        HAL_LOG_MESSAGE(LogLevel::Info, "Device setup completed successfully");
        return HALVoidResult::success();
    }
    
    /**
     * @brief Perform bulk data operations
     * @param addresses List of addresses to read
     * @return HALResult containing read data or error
     */
    HALResult<std::vector<uint32_t>> bulk_read(const std::vector<uint32_t>& addresses) {
        std::vector<uint32_t> results;
        results.reserve(addresses.size());
        
        for (uint32_t addr : addresses) {
            auto read_result = device_->read_data(addr);
            
            // Pattern matching for error handling
            HAL_MATCH(std::move(read_result))
                .on_success([&](auto&& data) {
                    results.push_back(data);
                })
                .on_error([&](auto&& error) {
                    HAL_LOG_WARNING(error);
                    // Continue with default value for non-critical errors
                    if (error.category() == HALErrorCategory::Hardware) {
                        results.push_back(0xFFFFFFFF);  // Default value
                    }
                });
            
            // For critical errors, stop processing
            if (read_result.is_err() && 
                read_result.error().category() == HALErrorCategory::Validation) {
                return HALResult<std::vector<uint32_t>>::error(std::move(read_result.error()));
            }
        }
        
        return HALResult<std::vector<uint32_t>>::success(std::move(results));
    }
    
    /**
     * @brief Demonstrate different error handling patterns
     */
    void demonstrate_error_patterns() {
        std::cout << "\n=== Error Handling Patterns Demo ===\n";
        
        // Pattern 1: Simple error checking
        std::cout << "\n1. Simple error checking:\n";
        auto status_result = device_->get_status();
        if (status_result.is_err()) {
            std::cout << "Status check failed: " << status_result.error().message() << "\n";
        } else {
            std::cout << "Device status retrieved successfully\n";
        }
        
        // Pattern 2: Using value_or for defaults
        std::cout << "\n2. Using value_or for defaults:\n";
        auto data_result = device_->read_data(0x100);
        auto data = value_or(data_result, static_cast<uint32_t>(0xDEFAB1E));
        std::cout << "Data (with default): 0x" << std::hex << data << std::dec << "\n";
        
        // Pattern 3: Conditional execution
        std::cout << "\n3. Conditional execution:\n";
        HAL_IF_OK(device_->read_data(0x200)) {
            std::cout << "Read operation succeeded, data: 0x" 
                      << std::hex << __hal_result.value() << std::dec << "\n";
        }
        
        HAL_IF_ERR(device_->read_data(0x500)) {  // This will fail
            std::cout << "Read operation failed: " << __hal_result.error().message() << "\n";
        }
        
        // Pattern 4: Error tracking with RAII
        std::cout << "\n4. Error tracking with RAII:\n";
        {
            auto tracked = HAL_TRACK_ERRORS(device_->read_data(0x500));
            // Error will be automatically logged when tracked goes out of scope
        }
        
        // Pattern 5: Monadic operations
        std::cout << "\n5. Monadic operations:\n";
        auto transformed = device_->read_data(0x300)
            .map([](uint32_t data) { return data * 2; })
            .map([](uint32_t data) { return data + 100; });
        
        if (transformed.is_ok()) {
            std::cout << "Transformed data: " << transformed.value() << "\n";
        }
    }

private:
    std::shared_ptr<ExampleDevice> device_;
    
    /**
     * @brief Configure device with initial settings
     * @return HALResult indicating success or failure
     */
    HALVoidResult configure_device() {
        // Write some configuration values
        HAL_TRY_VOID(device_->write_data(0x10, 0x1234));
        HAL_TRY_VOID(device_->write_data(0x20, 0x5678));
        HAL_TRY_VOID(device_->write_data(0x30, 0x9ABC));
        
        return HALVoidResult::success();
    }
};

/**
 * @brief Demonstrate error categorization and logging
 */
void demonstrate_error_categories() {
    std::cout << "\n=== Error Categories Demo ===\n";
    
    // Different error categories
    std::vector<HALError> example_errors = {
        errors::device_not_found(1, "GPU"),
        errors::driver_incompatible(2, "v1.0"),
        errors::invalid_parameter(3, "buffer_size"),
        errors::out_of_memory(4, "16MB heap"),
        errors::platform_not_supported(5, "Dreamcast"),
        errors::connection_failed(6, "192.168.1.1"),
        errors::validation_failed(7, "checksum"),
        errors::internal_error(8, "mutex_lock")
    };
    
    for (const auto& error : example_errors) {
        std::cout << "Category: " << category_to_string(error.category())
                  << ", Code: " << error.code()
                  << ", Message: " << error.message();
        if (error.context()) {
            std::cout << ", Context: " << error.context();
        }
        std::cout << "\n";
    }
}

/**
 * @brief Performance demonstration - zero allocation guarantee
 */
void demonstrate_zero_allocation() {
    std::cout << "\n=== Zero Allocation Demo ===\n";
    
    // All of these operations use stack-only storage
    auto device = std::make_shared<ExampleDevice>();
    
    // Initialize device
    auto init_result = device->initialize();
    std::cout << "Initialize result size: " << sizeof(init_result) << " bytes\n";
    
    // Read operation
    auto read_result = device->read_data(0x100);
    std::cout << "Read result size: " << sizeof(read_result) << " bytes\n";
    
    // Error creation
    auto error = errors::device_not_found(1, "test");
    std::cout << "Error size: " << sizeof(error) << " bytes\n";
    
    std::cout << "All error handling uses fixed-size, stack-allocated storage!\n";
}

int main() {
    // Set up logging
    HALLogger::set_log_level(LogLevel::Debug);
    
    std::cout << "=== Flight HAL Error Handling Example ===\n";
    
    // Create device and service
    auto device = std::make_shared<ExampleDevice>();
    DeviceService service(device);
    
    // Demonstrate setup with error handling
    std::cout << "\n--- Device Setup ---\n";
    auto setup_result = service.setup_device();
    if (setup_result.is_err()) {
        std::cout << "Setup failed: " << setup_result.error().message() << "\n";
        return 1;
    }
    
    // Demonstrate bulk operations
    std::cout << "\n--- Bulk Operations ---\n";
    std::vector<uint32_t> addresses = {0x100, 0x200, 0x300, 0x500, 0x600};  // 0x500 will fail
    auto bulk_result = service.bulk_read(addresses);
    
    if (bulk_result.is_ok()) {
        std::cout << "Bulk read completed. Results:\n";
        const auto& data = bulk_result.value();
        for (size_t i = 0; i < data.size(); ++i) {
            std::cout << "  [" << i << "] 0x" << std::hex << data[i] << std::dec << "\n";
        }
    } else {
        std::cout << "Bulk read failed: " << bulk_result.error().message() << "\n";
    }
    
    // Demonstrate various error handling patterns
    service.demonstrate_error_patterns();
    
    // Demonstrate error categories
    demonstrate_error_categories();
    
    // Demonstrate zero allocation
    demonstrate_zero_allocation();
    
    std::cout << "\n=== Example Completed Successfully ===\n";
    return 0;
}
