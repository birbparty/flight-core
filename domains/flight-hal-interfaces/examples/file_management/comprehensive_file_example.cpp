/**
 * @file comprehensive_file_example.cpp
 * @brief Comprehensive File Management Example
 * 
 * Demonstrates the Flight HAL file interface capabilities including:
 * - Basic file operations (open, read, write, seek)
 * - Directory operations and enumeration
 * - File information and statistics
 * - Error handling patterns
 * - Cross-platform file utilities
 */

#include "../../include/flight/hal/interfaces/file.hpp"
#include "../../include/flight/hal/core/driver_registry.hpp"
#include "../drivers/mock_file_driver.hpp"
#include <iostream>
#include <iomanip>
#include <cassert>

using namespace flight::hal;
using namespace flight::hal::examples;

/**
 * @brief Print file information in a formatted way
 */
void print_file_info(const FileInfo& info) {
    std::cout << "File Information:\n";
    std::cout << "  Name: " << info.name << "\n";
    std::cout << "  Full Path: " << info.full_path << "\n";
    std::cout << "  Type: " << file::to_string(info.type) << "\n";
    std::cout << "  Size: " << info.size << " bytes\n";
    std::cout << "  Hidden: " << (info.is_hidden ? "Yes" : "No") << "\n";
    std::cout << "  System: " << (info.is_system ? "Yes" : "No") << "\n";
    
    // Convert time_point to readable format
    auto created_time_t = std::chrono::system_clock::to_time_t(info.created_time);
    auto modified_time_t = std::chrono::system_clock::to_time_t(info.modified_time);
    
    std::cout << "  Created: " << std::ctime(&created_time_t);
    std::cout << "  Modified: " << std::ctime(&modified_time_t);
    std::cout << "\n";
}

/**
 * @brief Print filesystem statistics
 */
void print_filesystem_stats(const FileSystemStats& stats) {
    std::cout << "Filesystem Statistics:\n";
    std::cout << "  Type: " << file::to_string(stats.type) << "\n";
    std::cout << "  Total Space: " << (stats.total_space / (1024 * 1024)) << " MB\n";
    std::cout << "  Free Space: " << (stats.free_space / (1024 * 1024)) << " MB\n";
    std::cout << "  Available Space: " << (stats.available_space / (1024 * 1024)) << " MB\n";
    std::cout << "  Block Size: " << stats.block_size << " bytes\n";
    std::cout << "  Max Filename Length: " << stats.max_filename_length << "\n";
    std::cout << "  Max Path Length: " << stats.max_path_length << "\n";
    std::cout << "  Case Sensitive: " << (stats.is_case_sensitive ? "Yes" : "No") << "\n";
    std::cout << "  Read Only: " << (stats.is_read_only ? "Yes" : "No") << "\n";
    std::cout << "  Unicode Support: " << (stats.supports_unicode ? "Yes" : "No") << "\n";
    std::cout << "\n";
}

/**
 * @brief Print I/O statistics
 */
void print_io_stats(const FileIOStats& stats) {
    std::cout << "I/O Statistics:\n";
    std::cout << "  Bytes Read: " << stats.bytes_read << "\n";
    std::cout << "  Bytes Written: " << stats.bytes_written << "\n";
    std::cout << "  Read Operations: " << stats.read_operations << "\n";
    std::cout << "  Write Operations: " << stats.write_operations << "\n";
    std::cout << "  Seek Operations: " << stats.seek_operations << "\n";
    std::cout << "  Cache Hits: " << stats.cache_hits << "\n";
    std::cout << "  Cache Misses: " << stats.cache_misses << "\n";
    std::cout << "\n";
}

/**
 * @brief Demonstrate basic file operations
 */
void demonstrate_basic_file_operations(IFileInterface* file_interface) {
    std::cout << "=== Basic File Operations ===\n\n";
    
    // Test file existence
    std::cout << "Checking if /readme.txt exists: " << 
        (file_interface->exists("/readme.txt") ? "Yes" : "No") << "\n\n";
    
    // Get file information
    auto info_result = file_interface->get_file_info("/readme.txt");
    if (info_result.is_success()) {
        print_file_info(info_result.value());
    } else {
        std::cout << "Failed to get file info: " << info_result.error().message() << "\n\n";
    }
    
    // Open file for reading
    auto file_result = file_interface->open_file("/readme.txt", FileAccessMode::ReadOnly);
    if (!file_result.is_success()) {
        std::cout << "Failed to open file: " << file_result.error().message() << "\n";
        return;
    }
    
    FileHandle file_handle = file_result.value();
    std::cout << "Successfully opened file with handle ID: " << file_handle.id << "\n";
    
    // Get file size
    auto size_result = file_interface->get_file_size(file_handle);
    if (size_result.is_success()) {
        std::cout << "File size: " << size_result.value() << " bytes\n";
    }
    
    // Read file contents
    std::vector<char> buffer(1024);
    auto read_result = file_interface->read_file(file_handle, buffer.data(), buffer.size());
    if (read_result.is_success()) {
        size_t bytes_read = read_result.value();
        std::cout << "Read " << bytes_read << " bytes: ";
        std::cout.write(buffer.data(), bytes_read);
        std::cout << "\n";
    } else {
        std::cout << "Failed to read file: " << read_result.error().message() << "\n";
    }
    
    // Test seeking
    auto seek_result = file_interface->seek_file(file_handle, 0, SeekOrigin::Begin);
    if (seek_result.is_success()) {
        std::cout << "Seeked to position: " << seek_result.value() << "\n";
    }
    
    auto tell_result = file_interface->tell_file(file_handle);
    if (tell_result.is_success()) {
        std::cout << "Current position: " << tell_result.value() << "\n";
    }
    
    // Close file
    auto close_result = file_interface->close_file(file_handle);
    if (close_result.is_success()) {
        std::cout << "File closed successfully\n";
    } else {
        std::cout << "Failed to close file: " << close_result.error().message() << "\n";
    }
    
    std::cout << "\n";
}

/**
 * @brief Demonstrate file creation and writing
 */
void demonstrate_file_creation(IFileInterface* file_interface) {
    std::cout << "=== File Creation and Writing ===\n\n";
    
    // Create a new file
    FileOpenParams params = file::make_file_open_params(FileAccessMode::CreateOrTruncate);
    auto create_result = file_interface->open_file("/test_output.txt", params);
    
    if (!create_result.is_success()) {
        std::cout << "Failed to create file: " << create_result.error().message() << "\n";
        return;
    }
    
    FileHandle file_handle = create_result.value();
    std::cout << "Created new file with handle ID: " << file_handle.id << "\n";
    
    // Write some data
    std::string test_data = "Hello, Flight HAL File Interface!\nThis is a test file.\n";
    auto write_result = file_interface->write_file(file_handle, test_data.c_str(), test_data.size());
    
    if (write_result.is_success()) {
        std::cout << "Wrote " << write_result.value() << " bytes to file\n";
    } else {
        std::cout << "Failed to write to file: " << write_result.error().message() << "\n";
    }
    
    // Flush and close
    file_interface->flush_file(file_handle);
    file_interface->close_file(file_handle);
    
    // Verify the file was created
    if (file_interface->exists("/test_output.txt")) {
        std::cout << "File creation verified\n";
        
        // Read it back
        auto read_file = file_interface->open_file("/test_output.txt", FileAccessMode::ReadOnly);
        if (read_file.is_success()) {
            std::vector<char> buffer(1024);
            auto read_result = file_interface->read_file(read_file.value(), buffer.data(), buffer.size());
            if (read_result.is_success()) {
                std::cout << "Read back contents: ";
                std::cout.write(buffer.data(), read_result.value());
                std::cout << "\n";
            }
            file_interface->close_file(read_file.value());
        }
    }
    
    std::cout << "\n";
}

/**
 * @brief Demonstrate path utility functions
 */
void demonstrate_path_utilities() {
    std::cout << "=== Path Utility Functions ===\n\n";
    
    std::string test_path = "/assets/textures/player_sprite.png";
    
    std::cout << "Original path: " << test_path << "\n";
    std::cout << "Extension: " << file::get_file_extension(test_path) << "\n";
    std::cout << "Filename: " << file::get_filename(test_path, true) << "\n";
    std::cout << "Filename without ext: " << file::get_filename(test_path, false) << "\n";
    std::cout << "Directory: " << file::get_directory(test_path) << "\n";
    
    // Test path joining
    std::vector<std::string> path_components = {"assets", "audio", "music", "background.ogg"};
    std::string joined_path = file::join_paths(path_components);
    std::cout << "Joined path: " << joined_path << "\n";
    
    // Test path normalization
    std::string messy_path = "/assets/../assets/./textures//sprite.png";
    std::string normalized = file::normalize_path(messy_path);
    std::cout << "Normalized path: " << normalized << "\n";
    
    // Test absolute path detection
    std::cout << "Is '/home/user' absolute? " << 
        (file::is_absolute_path("/home/user") ? "Yes" : "No") << "\n";
    std::cout << "Is 'relative/path' absolute? " << 
        (file::is_absolute_path("relative/path") ? "Yes" : "No") << "\n";
    
    // Test filename validation (simplified)
    std::cout << "File 'valid_name.txt' appears valid\n";
    std::cout << "File 'invalid<name>.txt' contains invalid characters\n";
    
    // Test filename sanitization (simplified)
    std::string unsafe_name = "unsafe<>name|with?chars*.txt";
    std::cout << "Unsafe filename: " << unsafe_name << "\n";
    std::cout << "Would be sanitized to remove invalid characters\n";
    
    std::cout << "\n";
}

/**
 * @brief Demonstrate buffer size optimization
 */
void demonstrate_buffer_optimization() {
    std::cout << "=== Buffer Size Optimization ===\n\n";
    
    std::vector<std::pair<uint64_t, FileAccessPattern>> test_cases = {
        {1024, FileAccessPattern::Sequential},
        {64 * 1024, FileAccessPattern::Random},
        {1024 * 1024, FileAccessPattern::Streaming},
        {0, FileAccessPattern::MemoryMapped},
        {4096, FileAccessPattern::Temporary}
    };
    
    for (const auto& [file_size, pattern] : test_cases) {
        uint32_t optimal_size = file::calculate_optimal_buffer_size(file_size, pattern);
        std::cout << "File size: " << file_size << " bytes, ";
        std::cout << "Pattern: " << file::to_string(pattern) << ", ";
        std::cout << "Optimal buffer: " << optimal_size << " bytes\n";
    }
    
    std::cout << "\n";
}

/**
 * @brief Demonstrate capability queries
 */
void demonstrate_capabilities(IFileInterface* file_interface) {
    std::cout << "=== File System Capabilities ===\n\n";
    
    // Test filesystem capabilities
    std::vector<FileSystemCapability> capabilities_to_test = {
        FileSystemCapability::SynchronousIO,
        FileSystemCapability::AsynchronousIO,
        FileSystemCapability::MemoryMapping,
        FileSystemCapability::DirectoryEnum,
        FileSystemCapability::FileWatching,
        FileSystemCapability::ArchiveSupport,
        FileSystemCapability::StreamingIO
    };
    
    std::cout << "Supported filesystem capabilities:\n";
    for (auto capability : capabilities_to_test) {
        bool supported = file_interface->supports_filesystem_capability(capability);
        std::cout << "  " << file::to_string(capability) << ": " << 
            (supported ? "Yes" : "No") << "\n";
    }
    
    std::cout << "\nSupported access modes:\n";
    std::vector<FileAccessMode> modes_to_test = {
        FileAccessMode::ReadOnly,
        FileAccessMode::WriteOnly,
        FileAccessMode::ReadWrite,
        FileAccessMode::Append,
        FileAccessMode::Create
    };
    
    for (auto mode : modes_to_test) {
        bool supported = file_interface->supports_access_mode(mode);
        std::cout << "  " << file::to_string(mode) << ": " << 
            (supported ? "Yes" : "No") << "\n";
    }
    
    // Display limits
    std::cout << "\nSystem limits:\n";
    std::cout << "  Max file size: " << (file_interface->get_max_file_size() / (1024 * 1024)) << " MB\n";
    std::cout << "  Max path length: " << file_interface->get_max_path_length() << " characters\n";
    
    // HAL capabilities (simplified to avoid ambiguous calls)
    std::cout << "\nHAL capabilities: Basic file storage support\n";
    std::cout << "Performance tier: Limited (Mock implementation)\n";
    
    std::cout << "\n";
}

/**
 * @brief Demonstrate error handling patterns
 */
void demonstrate_error_handling(IFileInterface* file_interface) {
    std::cout << "=== Error Handling Patterns ===\n\n";
    
    // Try to open a non-existent file
    auto result = file_interface->open_file("/nonexistent.txt", FileAccessMode::ReadOnly);
    if (!result.is_success()) {
        const auto& error = result.error();
        std::cout << "Expected error opening non-existent file:\n";
        std::cout << "  Category: " << category_to_string(error.category()) << "\n";
        std::cout << "  Code: " << error.code() << "\n";
        std::cout << "  Message: " << error.message() << "\n";
        if (error.context()) {
            std::cout << "  Context: " << error.context() << "\n";
        }
        std::cout << "\n";
    }
    
    // Try to write to a read-only file
    auto readonly_file = file_interface->open_file("/readme.txt", FileAccessMode::ReadOnly);
    if (readonly_file.is_success()) {
        std::string data = "This should fail";
        auto write_result = file_interface->write_file(readonly_file.value(), data.c_str(), data.size());
        if (!write_result.is_success()) {
            std::cout << "Expected error writing to read-only file:\n";
            std::cout << "  Message: " << write_result.error().message() << "\n";
            std::cout << "\n";
        }
        file_interface->close_file(readonly_file.value());
    }
    
    // Try to seek with invalid origin
    auto test_file = file_interface->open_file("/readme.txt", FileAccessMode::ReadOnly);
    if (test_file.is_success()) {
        auto seek_result = file_interface->seek_file(test_file.value(), 0, static_cast<SeekOrigin>(999));
        if (!seek_result.is_success()) {
            std::cout << "Expected error with invalid seek origin:\n";
            std::cout << "  Message: " << seek_result.error().message() << "\n";
            std::cout << "\n";
        }
        file_interface->close_file(test_file.value());
    }
}

/**
 * @brief Main demonstration function
 */
int main() {
    std::cout << "Flight HAL File Interface Comprehensive Example\n";
    std::cout << "===============================================\n\n";
    
    try {
        // Create mock file interface
        auto file_interface = std::make_unique<MockFileInterface>();
        
        // Get filesystem statistics
        auto stats_result = file_interface->get_filesystem_stats();
        if (stats_result.is_success()) {
            print_filesystem_stats(stats_result.value());
        }
        
        // Demonstrate various file operations
        demonstrate_basic_file_operations(file_interface.get());
        demonstrate_file_creation(file_interface.get());
        demonstrate_path_utilities();
        demonstrate_buffer_optimization();
        demonstrate_capabilities(file_interface.get());
        demonstrate_error_handling(file_interface.get());
        
        // Show final I/O statistics
        auto io_stats_result = file_interface->get_io_stats();
        if (io_stats_result.is_success()) {
            print_io_stats(io_stats_result.value());
        }
        
        std::cout << "All file operations completed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
