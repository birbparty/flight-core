#pragma once

#include "flight/hal/macos/core_foundation_bridge.hpp"
#include "flight/hal/macos/process_manager.hpp"
#include <future>
#include <chrono>
#include <functional>
#include <filesystem>
#include <thread>

// Forward declarations for Objective-C classes
#ifdef __OBJC__
@class NSTask;
@class NSPipe;
@class NSFileManager;
@class NSTimer;
@class NSNotificationCenter;
@class NSWorkspace;
@class NSRunningApplication;
#else
class NSTask;
class NSPipe;
class NSFileManager;
class NSTimer;
class NSNotificationCenter;
class NSWorkspace;
class NSRunningApplication;
#endif

namespace flight::hal::macos {

/// System information from NSWorkspace
struct WorkspaceInfo {
    std::string computer_name;
    std::string user_name;
    std::vector<std::string> running_applications;
    std::string active_application;
    bool screen_locked;
    double cpu_usage;
    uint64_t memory_usage;
};

/// File system event from NSFileManager
enum class FSEvent {
    Created,
    Modified,
    Deleted,
    Moved,
    AttributeChanged
};

/// File system watcher callback
using FSWatcherCallback = std::function<void(FSEvent, const std::filesystem::path&)>;

/// RAII wrapper for NSTask with modern C++ interface
class NSTaskWrapper {
public:
    NSTaskWrapper();
    ~NSTaskWrapper();
    
    // Non-copyable, movable
    NSTaskWrapper(const NSTaskWrapper&) = delete;
    NSTaskWrapper& operator=(const NSTaskWrapper&) = delete;
    NSTaskWrapper(NSTaskWrapper&&) noexcept;
    NSTaskWrapper& operator=(NSTaskWrapper&&) noexcept;
    
    /// Set the executable path
    void setExecutable(const std::string& path);
    
    /// Set command line arguments
    void setArguments(const std::vector<std::string>& args);
    
    /// Set working directory
    void setWorkingDirectory(const std::filesystem::path& path);
    
    /// Set environment variables
    void setEnvironment(const std::unordered_map<std::string, std::string>& env);
    
    /// Set timeout for execution
    void setTimeout(std::chrono::milliseconds timeout);
    
    /// Execute synchronously and return result
    ProcessResult execute();
    
    /// Execute asynchronously and return future
    std::future<ProcessResult> executeAsync();
    
    /// Check if task is currently running
    bool isRunning() const;
    
    /// Terminate the running task
    bool terminate();
    
    /// Kill the running task forcefully
    bool kill();
    
    /// Get process ID (if running)
    int getProcessId() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl_;
};

/// RAII wrapper for NSFileManager operations
class NSFileManagerWrapper {
public:
    NSFileManagerWrapper();
    ~NSFileManagerWrapper();
    
    // Non-copyable, non-movable (singleton-like behavior)
    NSFileManagerWrapper(const NSFileManagerWrapper&) = delete;
    NSFileManagerWrapper& operator=(const NSFileManagerWrapper&) = delete;
    NSFileManagerWrapper(NSFileManagerWrapper&&) = delete;
    NSFileManagerWrapper& operator=(NSFileManagerWrapper&&) = delete;
    
    /// Check if file or directory exists
    bool exists(const std::filesystem::path& path);
    
    /// Check if path is directory
    bool isDirectory(const std::filesystem::path& path);
    
    /// Create directory (with intermediate directories if needed)
    Result<bool> createDirectory(const std::filesystem::path& path, bool createIntermediates = true);
    
    /// Remove file or directory
    Result<bool> removeItem(const std::filesystem::path& path);
    
    /// Copy file or directory
    Result<bool> copyItem(const std::filesystem::path& source, const std::filesystem::path& destination);
    
    /// Move file or directory
    Result<bool> moveItem(const std::filesystem::path& source, const std::filesystem::path& destination);
    
    /// Get file attributes
    Result<std::unordered_map<std::string, std::string>> getAttributes(const std::filesystem::path& path);
    
    /// Set file attributes
    Result<bool> setAttributes(const std::filesystem::path& path, const std::unordered_map<std::string, std::string>& attributes);
    
    /// List directory contents
    Result<std::vector<std::string>> listDirectory(const std::filesystem::path& path);
    
    /// Get file size
    Result<uint64_t> getFileSize(const std::filesystem::path& path);
    
    /// Get file modification date
    Result<std::chrono::system_clock::time_point> getModificationDate(const std::filesystem::path& path);
    
    /// Watch directory for changes (returns handle for stopping)
    std::unique_ptr<class FSWatcher> watchDirectory(const std::filesystem::path& path, FSWatcherCallback callback);
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl_;
};

/// File system watcher handle
class FSWatcher {
public:
    virtual ~FSWatcher() = default;
    
    /// Stop watching
    virtual void stop() = 0;
    
    /// Check if still watching
    virtual bool isActive() const = 0;
    
    /// Get watched path
    virtual std::filesystem::path getPath() const = 0;
};

/// NSTimer wrapper for scheduling
class NSTimerWrapper {
public:
    NSTimerWrapper();
    ~NSTimerWrapper();
    
    // Non-copyable, movable
    NSTimerWrapper(const NSTimerWrapper&) = delete;
    NSTimerWrapper& operator=(const NSTimerWrapper&) = delete;
    NSTimerWrapper(NSTimerWrapper&&) noexcept;
    NSTimerWrapper& operator=(NSTimerWrapper&&) noexcept;
    
    /// Schedule timer to fire once after delay
    void scheduleOnce(std::chrono::milliseconds delay, std::function<void()> callback);
    
    /// Schedule repeating timer
    void scheduleRepeating(std::chrono::milliseconds interval, std::function<void()> callback);
    
    /// Stop the timer
    void stop();
    
    /// Check if timer is valid and scheduled
    bool isValid() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl_;
};

/// NSWorkspace wrapper for system information
class NSWorkspaceWrapper {
public:
    NSWorkspaceWrapper();
    ~NSWorkspaceWrapper();
    
    // Non-copyable, non-movable (singleton-like behavior)
    NSWorkspaceWrapper(const NSWorkspaceWrapper&) = delete;
    NSWorkspaceWrapper& operator=(const NSWorkspaceWrapper&) = delete;
    NSWorkspaceWrapper(NSWorkspaceWrapper&&) = delete;
    NSWorkspaceWrapper& operator=(NSWorkspaceWrapper&&) = delete;
    
    /// Get system workspace information
    WorkspaceInfo getWorkspaceInfo();
    
    /// Get running applications
    std::vector<std::string> getRunningApplications();
    
    /// Get active application name
    std::string getActiveApplication();
    
    /// Launch application by bundle identifier
    Result<bool> launchApplication(const std::string& bundleIdentifier);
    
    /// Launch application at path
    Result<bool> launchApplicationAtPath(const std::filesystem::path& path);
    
    /// Terminate application by bundle identifier
    Result<bool> terminateApplication(const std::string& bundleIdentifier);
    
    /// Check if application is running
    bool isApplicationRunning(const std::string& bundleIdentifier);
    
    /// Open URL with default application
    Result<bool> openURL(const std::string& url);
    
    /// Open file with default application
    Result<bool> openFile(const std::filesystem::path& path);
    
    /// Register for workspace notifications
    void registerForApplicationLaunchNotifications(std::function<void(const std::string&)> callback);
    void registerForApplicationTerminateNotifications(std::function<void(const std::string&)> callback);
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl_;
};

/// Foundation Framework Bridge - Main interface
class FoundationBridge {
public:
    FoundationBridge();
    ~FoundationBridge();
    
    // Non-copyable, non-movable
    FoundationBridge(const FoundationBridge&) = delete;
    FoundationBridge& operator=(const FoundationBridge&) = delete;
    FoundationBridge(FoundationBridge&&) = delete;
    FoundationBridge& operator=(FoundationBridge&&) = delete;
    
    /// Get NSTask wrapper
    std::unique_ptr<NSTaskWrapper> createTask();
    
    /// Get NSFileManager wrapper (shared instance)
    NSFileManagerWrapper& getFileManager();
    
    /// Get NSWorkspace wrapper (shared instance)
    NSWorkspaceWrapper& getWorkspace();
    
    /// Create NSTimer wrapper
    std::unique_ptr<NSTimerWrapper> createTimer();
    
    /// Execute command synchronously using NSTask
    ProcessResult executeCommand(const std::string& command, const std::vector<std::string>& args);
    
    /// Execute command asynchronously using NSTask
    std::future<ProcessResult> executeCommandAsync(const std::string& command, const std::vector<std::string>& args);
    
    /// Execute command with custom environment
    ProcessResult executeCommandWithEnvironment(
        const std::string& command, 
        const std::vector<std::string>& args,
        const std::unordered_map<std::string, std::string>& env
    );
    
    /// Execute command with timeout
    ProcessResult executeCommandWithTimeout(
        const std::string& command, 
        const std::vector<std::string>& args,
        std::chrono::milliseconds timeout
    );
    
    /// Utility: Convert NSString to std::string
    static std::string nsStringToStd(void* nsString);
    
    /// Utility: Convert std::string to NSString (autoreleased)
    static void* stdToNSString(const std::string& str);
    
    /// Utility: Convert NSArray to std::vector<std::string>
    static std::vector<std::string> nsArrayToStdVector(void* nsArray);
    
    /// Utility: Convert std::vector<std::string> to NSArray
    static void* stdVectorToNSArray(const std::vector<std::string>& vec);
    
    /// Utility: Convert NSDictionary to std::unordered_map
    static std::unordered_map<std::string, std::string> nsDictionaryToStdMap(void* nsDictionary);
    
    /// Utility: Convert std::unordered_map to NSDictionary
    static void* stdMapToNSDictionary(const std::unordered_map<std::string, std::string>& map);
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl_;
};

} // namespace flight::hal::macos
