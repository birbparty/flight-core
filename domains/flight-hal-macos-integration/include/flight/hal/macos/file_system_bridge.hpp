#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <filesystem>

namespace flight::hal::macos {

/// File system operation result
enum class FileSystemResult {
    Success,
    PermissionDenied,
    NotFound,
    AlreadyExists,
    InsufficientSpace,
    IOError,
    NotSupported
};

/// File system event types for monitoring
enum class FileSystemEvent {
    Created,
    Modified,
    Deleted,
    Moved,
    AttributeChanged,
    AccessTimeChanged
};

/// File system statistics
struct FileSystemStats {
    uint64_t total_size;
    uint64_t available_size;
    uint64_t used_size;
    uint32_t block_size;
    bool case_sensitive;
    bool supports_extended_attributes;
    std::string filesystem_type;
};

/// File metadata information
struct FileMetadata {
    std::filesystem::path path;
    uint64_t size;
    std::chrono::system_clock::time_point created_time;
    std::chrono::system_clock::time_point modified_time;
    std::chrono::system_clock::time_point accessed_time;
    bool is_directory;
    bool is_regular_file;
    bool is_symbolic_link;
    bool is_hidden;
    uint32_t permissions;
    std::string owner;
    std::string group;
};

/// File system watcher handle
class FileSystemWatcher {
public:
    virtual ~FileSystemWatcher() = default;
    
    /// Stop watching the path
    virtual void stop() = 0;
    
    /// Check if watcher is active
    virtual bool isActive() const = 0;
    
    /// Get watched path
    virtual std::filesystem::path getWatchedPath() const = 0;
};

/// macOS file system integration bridge for Flight ecosystem
class FileSystemBridge {
public:
    /// Create file system bridge instance
    static std::unique_ptr<FileSystemBridge> create();
    
    /// Destructor
    virtual ~FileSystemBridge() = default;
    
    // Basic File Operations
    /// Read file contents
    virtual std::optional<std::vector<uint8_t>> readFile(
        const std::filesystem::path& path
    ) = 0;
    
    /// Write file contents
    virtual FileSystemResult writeFile(
        const std::filesystem::path& path,
        const std::vector<uint8_t>& data,
        bool create_directories = true
    ) = 0;
    
    /// Copy file or directory
    virtual FileSystemResult copy(
        const std::filesystem::path& source,
        const std::filesystem::path& destination,
        bool overwrite = false
    ) = 0;
    
    /// Move file or directory
    virtual FileSystemResult move(
        const std::filesystem::path& source,
        const std::filesystem::path& destination
    ) = 0;
    
    /// Delete file or directory
    virtual FileSystemResult remove(
        const std::filesystem::path& path,
        bool recursive = false
    ) = 0;
    
    // Directory Operations
    /// Create directory
    virtual FileSystemResult createDirectory(
        const std::filesystem::path& path,
        bool create_parents = true
    ) = 0;
    
    /// List directory contents
    virtual std::optional<std::vector<FileMetadata>> listDirectory(
        const std::filesystem::path& path,
        bool recursive = false
    ) = 0;
    
    /// Check if path exists
    virtual bool exists(const std::filesystem::path& path) = 0;
    
    /// Get file metadata
    virtual std::optional<FileMetadata> getMetadata(
        const std::filesystem::path& path
    ) = 0;
    
    // macOS Specific Features
    /// Get or set extended attributes (xattr)
    virtual std::optional<std::vector<uint8_t>> getExtendedAttribute(
        const std::filesystem::path& path,
        const std::string& name
    ) = 0;
    
    /// Set extended attribute
    virtual FileSystemResult setExtendedAttribute(
        const std::filesystem::path& path,
        const std::string& name,
        const std::vector<uint8_t>& value
    ) = 0;
    
    /// List extended attributes
    virtual std::vector<std::string> listExtendedAttributes(
        const std::filesystem::path& path
    ) = 0;
    
    /// Remove extended attribute
    virtual FileSystemResult removeExtendedAttribute(
        const std::filesystem::path& path,
        const std::string& name
    ) = 0;
    
    // File System Monitoring
    /// Watch path for changes
    virtual std::unique_ptr<FileSystemWatcher> watchPath(
        const std::filesystem::path& path,
        std::function<void(FileSystemEvent, const std::filesystem::path&)> callback,
        bool recursive = false
    ) = 0;
    
    // Flight Ecosystem Integration
    /// Get Flight workspace root
    virtual std::optional<std::filesystem::path> getFlightWorkspaceRoot() = 0;
    
    /// Get Flight cache directory
    virtual std::filesystem::path getFlightCacheDirectory() = 0;
    
    /// Get Flight temporary directory
    virtual std::filesystem::path getFlightTemporaryDirectory() = 0;
    
    /// Create Flight component workspace
    virtual FileSystemResult createComponentWorkspace(
        const std::string& component_name
    ) = 0;
    
    /// Clean Flight temporary files
    virtual FileSystemResult cleanTemporaryFiles(
        std::chrono::hours max_age = std::chrono::hours(24)
    ) = 0;
    
    // Performance Optimizations
    /// Prefetch files for faster access
    virtual FileSystemResult prefetchFiles(
        const std::vector<std::filesystem::path>& paths
    ) = 0;
    
    /// Enable unified memory optimization for large files
    virtual FileSystemResult enableUnifiedMemoryOptimization(
        const std::filesystem::path& path
    ) = 0;
    
    /// Get file system statistics
    virtual std::optional<FileSystemStats> getFileSystemStats(
        const std::filesystem::path& path
    ) = 0;
    
    // Apple Silicon Optimizations
    /// Enable SSD optimization for Apple Silicon
    virtual FileSystemResult enableSSDOptimization() = 0;
    
    /// Optimize file access patterns for M4 Max
    virtual FileSystemResult optimizeForM4Max() = 0;
    
protected:
    FileSystemBridge() = default;
    
private:
    // Non-copyable, non-movable
    FileSystemBridge(const FileSystemBridge&) = delete;
    FileSystemBridge& operator=(const FileSystemBridge&) = delete;
    FileSystemBridge(FileSystemBridge&&) = delete;
    FileSystemBridge& operator=(FileSystemBridge&&) = delete;
};

} // namespace flight::hal::macos
