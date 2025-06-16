#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <functional>
#include <chrono>

namespace flight::hal::macos {

/// Process priority levels optimized for Apple Silicon
enum class ProcessPriority {
    Low,           ///< Background tasks, use efficiency cores
    Normal,        ///< Standard priority 
    High,          ///< Use performance cores preferentially
    RealTime,      ///< Real-time priority for critical tasks
    SystemCritical ///< System-level critical processes
};

/// Process execution context for Flight components
struct ProcessContext {
    std::string name;
    ProcessPriority priority;
    bool prefer_performance_cores;
    bool metal_access_required;
    size_t memory_limit_mb;
    std::chrono::milliseconds timeout;
};

/// Process execution result
struct ProcessResult {
    int exit_code;
    std::string stdout_output;
    std::string stderr_output;
    std::chrono::milliseconds execution_time;
    bool timed_out;
    bool success;
    
    ProcessResult() : exit_code(-1), execution_time(0), timed_out(false), success(false) {}
};

/// Process handle for running processes
class ProcessHandle {
public:
    virtual ~ProcessHandle() = default;
    
    /// Get process ID
    virtual int getPID() const = 0;
    
    /// Check if process is still running
    virtual bool isRunning() const = 0;
    
    /// Wait for process completion
    virtual ProcessResult wait() = 0;
    
    /// Wait for process completion with timeout
    virtual std::optional<ProcessResult> waitFor(std::chrono::milliseconds timeout) = 0;
    
    /// Terminate process gracefully
    virtual bool terminate() = 0;
    
    /// Kill process forcefully
    virtual bool kill() = 0;
    
    /// Get current CPU usage
    virtual float getCPUUsage() const = 0;
    
    /// Get current memory usage
    virtual size_t getMemoryUsage() const = 0;
};

/// Process manager for Flight ecosystem coordination
class ProcessManager {
public:
    /// Create process manager instance
    static std::unique_ptr<ProcessManager> create();
    
    /// Destructor
    virtual ~ProcessManager() = default;
    
    // Process Execution
    /// Execute a process with given context
    virtual std::unique_ptr<ProcessHandle> execute(
        const std::string& command,
        const std::vector<std::string>& args,
        const ProcessContext& context
    ) = 0;
    
    /// Execute and wait for completion
    virtual ProcessResult executeAndWait(
        const std::string& command,
        const std::vector<std::string>& args,
        const ProcessContext& context
    ) = 0;
    
    // Flight Component Process Management
    /// Launch Flight CLI process
    virtual std::unique_ptr<ProcessHandle> launchFlightCLI(
        const std::vector<std::string>& args
    ) = 0;
    
    /// Launch Flight Runtime process
    virtual std::unique_ptr<ProcessHandle> launchFlightRuntime(
        const std::vector<std::string>& args
    ) = 0;
    
    /// Launch Component Flattening process
    virtual std::unique_ptr<ProcessHandle> launchComponentFlattening(
        const std::vector<std::string>& args
    ) = 0;
    
    // Process Coordination
    /// Set CPU affinity for Apple Silicon cores
    virtual bool setCPUAffinity(int pid, bool prefer_performance_cores) = 0;
    
    /// Set process priority
    virtual bool setPriority(int pid, ProcessPriority priority) = 0;
    
    /// Set memory limit for process
    virtual bool setMemoryLimit(int pid, size_t limit_mb) = 0;
    
    /// Enable Metal access for process
    virtual bool enableMetalAccess(int pid) = 0;
    
    // System Integration
    /// Get system process information
    virtual std::vector<int> getRunningFlightProcesses() const = 0;
    
    /// Get process information by PID
    virtual std::optional<ProcessContext> getProcessInfo(int pid) const = 0;
    
    /// Monitor process performance
    virtual void registerProcessMonitor(
        int pid,
        std::function<void(int pid, float cpu_usage, size_t memory_usage)> callback
    ) = 0;
    
    /// Remove process monitor
    virtual void removeProcessMonitor(int pid) = 0;
    
    // Apple Silicon Optimizations
    /// Optimize process for M4 Max performance cores
    virtual bool optimizeForPerformanceCores(int pid) = 0;
    
    /// Optimize process for M4 Max efficiency cores
    virtual bool optimizeForEfficiencyCores(int pid) = 0;
    
    /// Enable Grand Central Dispatch integration
    virtual bool enableGCDIntegration(int pid) = 0;
    
protected:
    ProcessManager() = default;
    
private:
    // Non-copyable, non-movable
    ProcessManager(const ProcessManager&) = delete;
    ProcessManager& operator=(const ProcessManager&) = delete;
    ProcessManager(ProcessManager&&) = delete;
    ProcessManager& operator=(ProcessManager&&) = delete;
};

} // namespace flight::hal::macos
