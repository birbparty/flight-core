#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <chrono>

namespace flight::hal::macos {

/// Development tool type
enum class DevelopmentTool {
    Xcode,
    Instruments,
    ActivityMonitor,
    Console,
    NetworkLinkConditioner,
    Simulator,
    DeviceManager,
    SystemProfiler
};

/// Profiling session configuration
struct ProfilingConfig {
    std::string session_name;
    std::vector<std::string> target_processes;
    bool profile_cpu;
    bool profile_memory;
    bool profile_gpu;
    bool profile_network;
    bool profile_disk_io;
    std::chrono::seconds duration;
    std::string output_path;
};

/// Performance metrics from profiling
struct PerformanceMetrics {
    // CPU Metrics
    float avg_cpu_usage;
    float peak_cpu_usage;
    std::vector<float> per_core_usage;
    
    // Memory Metrics
    uint64_t peak_memory_usage;
    uint64_t avg_memory_usage;
    uint32_t memory_allocations;
    uint32_t memory_deallocations;
    
    // GPU Metrics (Metal)
    float avg_gpu_usage;
    float peak_gpu_usage;
    uint64_t gpu_memory_usage;
    
    // I/O Metrics
    uint64_t disk_reads;
    uint64_t disk_writes;
    uint64_t network_bytes_sent;
    uint64_t network_bytes_received;
    
    // Apple Silicon Specific
    bool used_performance_cores;
    bool used_efficiency_cores;
    float neural_engine_usage;
};

/// Debug session handle
class DebugSession {
public:
    virtual ~DebugSession() = default;
    
    /// Attach debugger to process
    virtual bool attachToProcess(int pid) = 0;
    
    /// Detach debugger
    virtual void detach() = 0;
    
    /// Set breakpoint
    virtual bool setBreakpoint(const std::string& function_name) = 0;
    
    /// Remove breakpoint
    virtual bool removeBreakpoint(const std::string& function_name) = 0;
    
    /// Continue execution
    virtual bool continueExecution() = 0;
    
    /// Step over
    virtual bool stepOver() = 0;
    
    /// Get call stack
    virtual std::vector<std::string> getCallStack() = 0;
    
    /// Evaluate expression
    virtual std::optional<std::string> evaluateExpression(const std::string& expression) = 0;
};

/// Profiling session handle
class ProfilingSession {
public:
    virtual ~ProfilingSession() = default;
    
    /// Start profiling
    virtual bool start() = 0;
    
    /// Stop profiling
    virtual bool stop() = 0;
    
    /// Pause profiling
    virtual bool pause() = 0;
    
    /// Resume profiling
    virtual bool resume() = 0;
    
    /// Check if profiling is active
    virtual bool isActive() const = 0;
    
    /// Get current metrics
    virtual PerformanceMetrics getCurrentMetrics() = 0;
    
    /// Save profiling data
    virtual bool saveData(const std::string& path) = 0;
};

/// macOS development tools integration for Flight ecosystem
class DevelopmentTools {
public:
    /// Create development tools instance
    static std::unique_ptr<DevelopmentTools> create();
    
    /// Destructor
    virtual ~DevelopmentTools() = default;
    
    // Tool Detection and Management
    /// Check if development tool is available
    virtual bool isToolAvailable(DevelopmentTool tool) = 0;
    
    /// Get tool version
    virtual std::optional<std::string> getToolVersion(DevelopmentTool tool) = 0;
    
    /// Launch development tool
    virtual bool launchTool(
        DevelopmentTool tool,
        const std::vector<std::string>& args = {}
    ) = 0;
    
    // Xcode Integration
    /// Open Xcode project
    virtual bool openXcodeProject(const std::string& project_path) = 0;
    
    /// Build Xcode project
    virtual bool buildXcodeProject(
        const std::string& project_path,
        const std::string& scheme,
        const std::string& configuration = "Debug"
    ) = 0;
    
    /// Run Xcode tests
    virtual bool runXcodeTests(
        const std::string& project_path,
        const std::string& scheme
    ) = 0;
    
    // Instruments Integration
    /// Start profiling session with Instruments
    virtual std::unique_ptr<ProfilingSession> startProfiling(
        const ProfilingConfig& config
    ) = 0;
    
    /// Create performance trace
    virtual bool createPerformanceTrace(
        int pid,
        const std::string& output_path,
        std::chrono::seconds duration
    ) = 0;
    
    /// Analyze existing trace file
    virtual std::optional<PerformanceMetrics> analyzeTrace(
        const std::string& trace_path
    ) = 0;
    
    // Debugging Support
    /// Create debug session
    virtual std::unique_ptr<DebugSession> createDebugSession() = 0;
    
    /// Attach LLDB to process
    virtual bool attachLLDB(int pid) = 0;
    
    /// Generate crash report
    virtual std::optional<std::string> generateCrashReport(int pid) = 0;
    
    // System Monitoring
    /// Monitor system performance
    virtual void startSystemMonitoring(
        std::function<void(const PerformanceMetrics&)> callback,
        std::chrono::seconds interval = std::chrono::seconds(1)
    ) = 0;
    
    /// Stop system monitoring
    virtual void stopSystemMonitoring() = 0;
    
    /// Get current system metrics
    virtual PerformanceMetrics getCurrentSystemMetrics() = 0;
    
    // Flight Ecosystem Integration
    /// Profile Flight CLI performance
    virtual std::unique_ptr<ProfilingSession> profileFlightCLI(
        const std::vector<std::string>& cli_args,
        const std::string& output_path
    ) = 0;
    
    /// Profile Flight Runtime performance
    virtual std::unique_ptr<ProfilingSession> profileFlightRuntime(
        const std::vector<std::string>& runtime_args,
        const std::string& output_path
    ) = 0;
    
    /// Profile Component Flattening performance
    virtual std::unique_ptr<ProfilingSession> profileComponentFlattening(
        const std::vector<std::string>& flattening_args,
        const std::string& output_path
    ) = 0;
    
    /// Generate Flight performance report
    virtual bool generateFlightPerformanceReport(
        const std::vector<std::string>& trace_paths,
        const std::string& report_path
    ) = 0;
    
    // Apple Silicon Specific Tools
    /// Enable Metal debugging
    virtual bool enableMetalDebugging() = 0;
    
    /// Capture Metal frame
    virtual bool captureMetalFrame(
        int pid,
        const std::string& output_path
    ) = 0;
    
    /// Profile Neural Engine usage
    virtual std::unique_ptr<ProfilingSession> profileNeuralEngine(
        int pid,
        const std::string& output_path
    ) = 0;
    
    /// Monitor unified memory bandwidth
    virtual void monitorUnifiedMemoryBandwidth(
        std::function<void(float bandwidth_gbps)> callback
    ) = 0;
    
    /// Analyze M4 Max performance characteristics
    virtual PerformanceMetrics analyzeM4MaxPerformance(
        int pid,
        std::chrono::seconds duration
    ) = 0;
    
    // Code Quality Tools
    /// Run static analysis
    virtual bool runStaticAnalysis(
        const std::string& source_path,
        const std::string& output_path
    ) = 0;
    
    /// Generate code coverage report
    virtual bool generateCodeCoverage(
        const std::string& executable_path,
        const std::vector<std::string>& test_args,
        const std::string& output_path
    ) = 0;
    
    /// Run sanitizers (AddressSanitizer, ThreadSanitizer)
    virtual bool runSanitizers(
        const std::string& executable_path,
        const std::vector<std::string>& args
    ) = 0;
    
    // Build System Integration
    /// Monitor build performance
    virtual void monitorBuildPerformance(
        const std::string& build_command,
        std::function<void(const PerformanceMetrics&)> callback
    ) = 0;
    
    /// Optimize build settings for Apple Silicon
    virtual std::unordered_map<std::string, std::string> getOptimalBuildSettings() = 0;
    
    /// Generate build performance report
    virtual bool generateBuildPerformanceReport(
        const std::vector<std::string>& build_logs,
        const std::string& report_path
    ) = 0;
    
protected:
    DevelopmentTools() = default;
    
private:
    // Non-copyable, non-movable
    DevelopmentTools(const DevelopmentTools&) = delete;
    DevelopmentTools& operator=(const DevelopmentTools&) = delete;
    DevelopmentTools(DevelopmentTools&&) = delete;
    DevelopmentTools& operator=(DevelopmentTools&&) = delete;
};

} // namespace flight::hal::macos
