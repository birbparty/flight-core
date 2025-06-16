/**
 * @file file.cpp
 * @brief Flight HAL File Interface Implementation
 * 
 * Core implementation and utility functions for the file I/O interface.
 */

#include "../../include/flight/hal/interfaces/file.hpp"
#include "../../include/flight/hal/core/hal_logging.hpp"
#include <algorithm>
#include <sstream>
#include <filesystem>

namespace flight::hal {

namespace file {

const char* to_string(FileSystemCapability capability) {
    switch (capability) {
        case FileSystemCapability::SynchronousIO: return "SynchronousIO";
        case FileSystemCapability::AsynchronousIO: return "AsynchronousIO";
        case FileSystemCapability::MemoryMapping: return "MemoryMapping";
        case FileSystemCapability::DirectoryEnum: return "DirectoryEnum";
        case FileSystemCapability::FileWatching: return "FileWatching";
        case FileSystemCapability::ArchiveSupport: return "ArchiveSupport";
        case FileSystemCapability::StreamingIO: return "StreamingIO";
        case FileSystemCapability::NetworkFS: return "NetworkFS";
        case FileSystemCapability::ReadOnly: return "ReadOnly";
        case FileSystemCapability::Executable: return "Executable";
        case FileSystemCapability::Compression: return "Compression";
        case FileSystemCapability::Seeking: return "Seeking";
        case FileSystemCapability::Truncation: return "Truncation";
        case FileSystemCapability::Locking: return "Locking";
        case FileSystemCapability::Permissions: return "Permissions";
        case FileSystemCapability::Timestamps: return "Timestamps";
        case FileSystemCapability::HardLinks: return "HardLinks";
        case FileSystemCapability::SymbolicLinks: return "SymbolicLinks";
        case FileSystemCapability::CaseSensitive: return "CaseSensitive";
        default: return "Unknown";
    }
}

const char* to_string(FileAccessMode mode) {
    switch (mode) {
        case FileAccessMode::ReadOnly: return "ReadOnly";
        case FileAccessMode::WriteOnly: return "WriteOnly";
        case FileAccessMode::ReadWrite: return "ReadWrite";
        case FileAccessMode::Append: return "Append";
        case FileAccessMode::Create: return "Create";
        case FileAccessMode::CreateOrTruncate: return "CreateOrTruncate";
        case FileAccessMode::CreateOrOpen: return "CreateOrOpen";
        default: return "Unknown";
    }
}

const char* to_string(FileAccessPattern pattern) {
    switch (pattern) {
        case FileAccessPattern::Sequential: return "Sequential";
        case FileAccessPattern::Random: return "Random";
        case FileAccessPattern::Streaming: return "Streaming";
        case FileAccessPattern::MemoryMapped: return "MemoryMapped";
        case FileAccessPattern::WriteOnce: return "WriteOnce";
        case FileAccessPattern::Temporary: return "Temporary";
        default: return "Unknown";
    }
}

const char* to_string(FileType type) {
    switch (type) {
        case FileType::Regular: return "Regular";
        case FileType::Directory: return "Directory";
        case FileType::SymbolicLink: return "SymbolicLink";
        case FileType::HardLink: return "HardLink";
        case FileType::Archive: return "Archive";
        case FileType::Device: return "Device";
        case FileType::Special: return "Special";
        case FileType::Unknown: return "Unknown";
        default: return "Unknown";
    }
}

const char* to_string(ArchiveFormat format) {
    switch (format) {
        case ArchiveFormat::Unknown: return "Unknown";
        case ArchiveFormat::ZIP: return "ZIP";
        case ArchiveFormat::PAK: return "PAK";
        case ArchiveFormat::WAD: return "WAD";
        case ArchiveFormat::TAR: return "TAR";
        case ArchiveFormat::SevenZip: return "7-Zip";
        case ArchiveFormat::Custom: return "Custom";
        default: return "Unknown";
    }
}

const char* to_string(FileSystemType type) {
    switch (type) {
        case FileSystemType::Unknown: return "Unknown";
        case FileSystemType::Native: return "Native";
        case FileSystemType::ISO9660: return "ISO9660";
        case FileSystemType::GDROM: return "GD-ROM";
        case FileSystemType::UMD: return "UMD";
        case FileSystemType::Archive: return "Archive";
        case FileSystemType::Network: return "Network";
        case FileSystemType::Memory: return "Memory";
        case FileSystemType::Browser: return "Browser";
        default: return "Unknown";
    }
}

std::string get_file_extension(const std::string& path) {
    auto dot_pos = path.find_last_of('.');
    auto slash_pos = std::max(path.find_last_of('/'), path.find_last_of('\\'));
    
    // Check if dot is after last slash (to handle hidden files like .bashrc)
    if (dot_pos != std::string::npos && 
        (slash_pos == std::string::npos || dot_pos > slash_pos)) {
        return path.substr(dot_pos + 1);
    }
    
    return "";
}

std::string get_filename(const std::string& path, bool with_extension) {
    auto slash_pos = std::max(path.find_last_of('/'), path.find_last_of('\\'));
    std::string filename;
    
    if (slash_pos != std::string::npos) {
        filename = path.substr(slash_pos + 1);
    } else {
        filename = path;
    }
    
    if (!with_extension) {
        auto dot_pos = filename.find_last_of('.');
        if (dot_pos != std::string::npos) {
            filename = filename.substr(0, dot_pos);
        }
    }
    
    return filename;
}

std::string get_directory(const std::string& path) {
    auto slash_pos = std::max(path.find_last_of('/'), path.find_last_of('\\'));
    
    if (slash_pos != std::string::npos) {
        return path.substr(0, slash_pos);
    }
    
    return ".";
}

std::string join_paths(const std::vector<std::string>& paths) {
    if (paths.empty()) {
        return "";
    }
    
    std::string result = paths[0];
    
    for (size_t i = 1; i < paths.size(); ++i) {
        const std::string& path = paths[i];
        if (path.empty()) continue;
        
        // Add separator if needed
        if (!result.empty() && result.back() != '/' && result.back() != '\\') {
            result += '/';
        }
        
        // Skip leading separator in path component
        size_t start = 0;
        while (start < path.length() && (path[start] == '/' || path[start] == '\\')) {
            start++;
        }
        
        result += path.substr(start);
    }
    
    return result;
}

std::string normalize_path(const std::string& path) {
    if (path.empty()) {
        return ".";
    }
    
    std::vector<std::string> components;
    std::stringstream ss(path);
    std::string component;
    bool is_absolute = path[0] == '/' || path[0] == '\\';
    
    // Split path into components
    while (std::getline(ss, component, '/') || std::getline(ss, component, '\\')) {
        if (component.empty() || component == ".") {
            continue;
        }
        
        if (component == "..") {
            if (!components.empty() && components.back() != "..") {
                components.pop_back();
            } else if (!is_absolute) {
                components.push_back("..");
            }
        } else {
            components.push_back(component);
        }
    }
    
    // Build normalized path
    std::string result;
    if (is_absolute) {
        result = "/";
    }
    
    for (size_t i = 0; i < components.size(); ++i) {
        if (i > 0 || is_absolute) {
            result += "/";
        }
        result += components[i];
    }
    
    return result.empty() ? "." : result;
}

bool is_absolute_path(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    
    // Unix-style absolute path
    if (path[0] == '/') {
        return true;
    }
    
    // Windows-style absolute path (C:\, D:\, etc.)
    if (path.length() >= 3 && std::isalpha(path[0]) && path[1] == ':' && 
        (path[2] == '\\' || path[2] == '/')) {
        return true;
    }
    
    // UNC path (\\server\share)
    if (path.length() >= 2 && path[0] == '\\' && path[1] == '\\') {
        return true;
    }
    
    return false;
}

std::string to_absolute_path(const std::string& path, const std::string& base_path) {
    if (is_absolute_path(path)) {
        return normalize_path(path);
    }
    
    std::string base = base_path;
    if (base.empty()) {
        // Use current working directory if available
        try {
            base = std::filesystem::current_path().string();
        } catch (...) {
            base = ".";
        }
    }
    
    return normalize_path(join_paths({base, path}));
}

FileOpenParams make_file_open_params(FileAccessMode access_mode, FileAccessPattern access_pattern) {
    FileOpenParams params = {};
    params.access_mode = access_mode;
    params.share_mode = FileShareMode::Read;
    params.access_pattern = access_pattern;
    params.buffer_size = 0; // Use default
    params.enable_caching = true;
    params.direct_io = false;
    params.sequential_scan = (access_pattern == FileAccessPattern::Sequential);
    params.random_access = (access_pattern == FileAccessPattern::Random);
    params.delete_on_close = false;
    params.timeout_ms = 5000; // 5 second timeout
    params.debug_name = {};
    
    return params;
}

FileOpenParams make_streaming_params(FileAccessMode access_mode, uint32_t buffer_size) {
    FileOpenParams params = make_file_open_params(access_mode, FileAccessPattern::Streaming);
    params.buffer_size = buffer_size;
    params.enable_caching = true;
    params.sequential_scan = true;
    params.random_access = false;
    
    return params;
}

FileOpenParams make_memory_mapped_params(bool writable) {
    FileAccessMode access_mode = writable ? FileAccessMode::ReadWrite : FileAccessMode::ReadOnly;
    FileOpenParams params = make_file_open_params(access_mode, FileAccessPattern::MemoryMapped);
    params.enable_caching = false; // Memory mapping doesn't need additional caching
    params.direct_io = true;       // Direct access to avoid double caching
    params.sequential_scan = false;
    params.random_access = true;
    
    return params;
}

uint32_t calculate_optimal_buffer_size(uint64_t file_size, FileAccessPattern access_pattern) {
    constexpr uint32_t MIN_BUFFER_SIZE = 4 * 1024;      // 4KB
    constexpr uint32_t MAX_BUFFER_SIZE = 1024 * 1024;   // 1MB
    constexpr uint32_t DEFAULT_BUFFER_SIZE = 64 * 1024; // 64KB
    
    switch (access_pattern) {
        case FileAccessPattern::Sequential:
            // Larger buffers for sequential access
            if (file_size < 64 * 1024) {
                return std::max(MIN_BUFFER_SIZE, static_cast<uint32_t>(file_size / 4));
            } else if (file_size < 1024 * 1024) {
                return DEFAULT_BUFFER_SIZE;
            } else {
                return MAX_BUFFER_SIZE;
            }
            
        case FileAccessPattern::Random:
            // Smaller buffers for random access
            return std::min(DEFAULT_BUFFER_SIZE / 2, std::max(MIN_BUFFER_SIZE, static_cast<uint32_t>(file_size / 16)));
            
        case FileAccessPattern::Streaming:
            // Large buffers for streaming
            return MAX_BUFFER_SIZE;
            
        case FileAccessPattern::MemoryMapped:
            // No buffering needed for memory mapped files
            return 0;
            
        case FileAccessPattern::WriteOnce:
            // Medium buffer for write-once pattern
            return DEFAULT_BUFFER_SIZE;
            
        case FileAccessPattern::Temporary:
            // Small buffer for temporary files
            return MIN_BUFFER_SIZE;
            
        default:
            return DEFAULT_BUFFER_SIZE;
    }
}

bool is_valid_filename(const std::string& filename, FileSystemType filesystem_type) {
    if (filename.empty()) {
        return false;
    }
    
    // Check for reserved characters
    const std::string invalid_chars = "<>:\"|?*";
    for (char c : invalid_chars) {
        if (filename.find(c) != std::string::npos) {
            return false;
        }
    }
    
    // Check for control characters
    for (char c : filename) {
        if (c >= 0 && c <= 31) {
            return false;
        }
    }
    
    // Filesystem-specific checks
    switch (filesystem_type) {
        case FileSystemType::ISO9660:
        case FileSystemType::GDROM:
        case FileSystemType::UMD:
            // ISO 9660 has strict naming rules
            if (filename.length() > 255) return false;
            // Additional ISO 9660 checks could go here
            break;
            
        case FileSystemType::Browser:
            // Browser filesystems are more restrictive
            if (filename.find('/') != std::string::npos || filename.find('\\') != std::string::npos) {
                return false;
            }
            break;
            
        default:
            // Standard filesystem checks
            if (filename.length() > 255) {
                return false;
            }
            break;
    }
    
    // Check for reserved names on Windows
    const std::vector<std::string> reserved_names = {
        "CON", "PRN", "AUX", "NUL",
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
    };
    
    std::string upper_filename = filename;
    std::transform(upper_filename.begin(), upper_filename.end(), upper_filename.begin(), ::toupper);
    
    for (const std::string& reserved : reserved_names) {
        if (upper_filename == reserved || upper_filename.find(reserved + ".") == 0) {
            return false;
        }
    }
    
    return true;
}

std::string sanitize_filename(const std::string& filename, FileSystemType filesystem_type) {
    if (filename.empty()) {
        return "unnamed";
    }
    
    std::string sanitized = filename;
    
    // Replace invalid characters
    const std::string invalid_chars = "<>:\"|?*";
    for (char& c : sanitized) {
        if (invalid_chars.find(c) != std::string::npos || (c >= 0 && c <= 31)) {
            c = '_';
        }
    }
    
    // Replace path separators
    std::replace(sanitized.begin(), sanitized.end(), '/', '_');
    std::replace(sanitized.begin(), sanitized.end(), '\\', '_');
    
    // Trim whitespace and periods from ends
    while (!sanitized.empty() && (sanitized.back() == ' ' || sanitized.back() == '.')) {
        sanitized.pop_back();
    }
    
    while (!sanitized.empty() && sanitized.front() == ' ') {
        sanitized.erase(0, 1);
    }
    
    // Ensure minimum length
    if (sanitized.empty()) {
        sanitized = "unnamed";
    }
    
    // Filesystem-specific sanitization
    switch (filesystem_type) {
        case FileSystemType::ISO9660:
        case FileSystemType::GDROM:
        case FileSystemType::UMD:
            // Convert to uppercase for ISO 9660 compatibility
            std::transform(sanitized.begin(), sanitized.end(), sanitized.begin(), ::toupper);
            if (sanitized.length() > 8) {
                // Truncate to 8.3 format if needed
                auto dot_pos = sanitized.find('.');
                if (dot_pos != std::string::npos) {
                    std::string name = sanitized.substr(0, std::min(8UL, dot_pos));
                    std::string ext = sanitized.substr(dot_pos + 1);
                    if (ext.length() > 3) {
                        ext = ext.substr(0, 3);
                    }
                    sanitized = name + "." + ext;
                } else {
                    sanitized = sanitized.substr(0, 8);
                }
            }
            break;
            
        default:
            // Standard filesystem truncation
            if (sanitized.length() > 255) {
                sanitized = sanitized.substr(0, 255);
            }
            break;
    }
    
    // Handle reserved names
    const std::vector<std::string> reserved_names = {
        "CON", "PRN", "AUX", "NUL",
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
    };
    
    std::string upper_sanitized = sanitized;
    std::transform(upper_sanitized.begin(), upper_sanitized.end(), upper_sanitized.begin(), ::toupper);
    
    for (const std::string& reserved : reserved_names) {
        if (upper_sanitized == reserved || upper_sanitized.find(reserved + ".") == 0) {
            sanitized = "_" + sanitized;
            break;
        }
    }
    
    return sanitized;
}

} // namespace file

} // namespace flight::hal
