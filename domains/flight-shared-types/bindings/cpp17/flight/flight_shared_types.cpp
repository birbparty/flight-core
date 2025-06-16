/**
 * Flight-Core Shared Types - C++17 Implementation
 */

#include "flight_shared_types.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <random>
#include <cctype>

namespace flight::shared_types {

namespace memory {

// MemorySize implementations
MemorySize MemorySize::from_bytes(std::uint64_t bytes) noexcept {
    std::ostringstream oss;
    
    if (bytes >= (1ULL << 30)) {
        double gb = bytes / double(1ULL << 30);
        oss << std::fixed << std::setprecision(1) << gb << "GB";
    } else if (bytes >= (1ULL << 20)) {
        double mb = bytes / double(1ULL << 20);
        oss << std::fixed << std::setprecision(1) << mb << "MB";
    } else if (bytes >= (1ULL << 10)) {
        double kb = bytes / double(1ULL << 10);
        oss << std::fixed << std::setprecision(1) << kb << "KB";
    } else {
        oss << bytes << "B";
    }
    
    return MemorySize{bytes, oss.str()};
}

MemorySize MemorySize::from_kb(std::uint64_t kb) noexcept {
    return from_bytes(kb * 1024);
}

MemorySize MemorySize::from_mb(std::uint64_t mb) noexcept {
    return from_bytes(mb * 1024 * 1024);
}

MemorySize MemorySize::from_gb(std::uint64_t gb) noexcept {
    return from_bytes(gb * 1024 * 1024 * 1024);
}

// MemoryUsageSnapshot implementations
double MemoryUsageSnapshot::usage_percentage() const noexcept {
    if (total.bytes == 0) return 0.0;
    return (used.bytes * 100.0) / total.bytes;
}

bool MemoryUsageSnapshot::is_low_memory() const noexcept {
    return usage_percentage() > 85.0;
}

bool MemoryUsageSnapshot::exceeds_threshold(double threshold) const noexcept {
    return usage_percentage() > threshold;
}

bool MemoryUsageSnapshot::is_fragmented() const noexcept {
    return fragmentation_ratio > 0.3f; // 30% fragmentation threshold
}

// MemoryAllocation implementations
bool MemoryAllocation::is_active() const noexcept {
    return !freed_at.has_value();
}

std::chrono::duration<double> MemoryAllocation::age() const noexcept {
    return std::chrono::system_clock::now() - allocated_at;
}

// MemoryLimits implementations
bool MemoryLimits::validate_allocation(const MemorySize& size) const noexcept {
    return size.bytes <= soft_limit.bytes;
}

// MemoryError implementations
std::string MemoryError::to_string() const {
    std::ostringstream oss;
    
    oss << "MemoryError[";
    switch (code_) {
        case Code::InsufficientMemory: oss << "InsufficientMemory"; break;
        case Code::LimitExceeded: oss << "LimitExceeded"; break;
        case Code::InvalidSize: oss << "InvalidSize"; break;
        case Code::AllocationFailed: oss << "AllocationFailed"; break;
        case Code::AlreadyFreed: oss << "AlreadyFreed"; break;
        case Code::InvalidAllocation: oss << "InvalidAllocation"; break;
        case Code::UnsupportedPlatform: oss << "UnsupportedPlatform"; break;
        case Code::FragmentationError: oss << "FragmentationError"; break;
    }
    oss << "]: " << message_;
    
    if (details_) {
        oss << " - " << *details_;
    }
    
    return oss.str();
}

// V6R utilities implementations
namespace v6r {
    MemorySize V6RMemoryUtils::get_vm_memory_limit(std::string_view vm_size) noexcept {
        if (vm_size == "small") return MemorySize::from_mb(512);
        if (vm_size == "medium") return MemorySize::from_gb(1);
        if (vm_size == "large") return MemorySize::from_gb(2);
        return MemorySize::from_mb(512); // Default to small
    }
    
    MemoryUsageSnapshot V6RMemoryUtils::create_snapshot(
        std::string_view session_id,
        std::string_view platform,
        const MemorySize& used) noexcept {
        
        auto total = get_vm_memory_limit(platform);
        auto available_bytes = total.bytes > used.bytes ? total.bytes - used.bytes : 0;
        auto available = MemorySize::from_bytes(available_bytes);
        
        return MemoryUsageSnapshot{
            .timestamp = std::chrono::system_clock::now(),
            .session_id = std::string(session_id),
            .platform = std::string(platform),
            .total = total,
            .used = used,
            .available = available,
            .fragmentation_ratio = 0.05f
        };
    }
    
    bool V6RMemoryUtils::validate_config(const V6RMemoryConfig& config) noexcept {
        return !config.vm_size.empty() && 
               !config.session_id.empty() && 
               !config.user_id.empty() &&
               (config.vm_size == "small" || config.vm_size == "medium" || config.vm_size == "large");
    }
}

// Helper function to convert from C bindings
namespace {
    std::string convert_c_string(const flight_memory_string_t& c_str) {
        return std::string(reinterpret_cast<const char*>(c_str.ptr), c_str.len);
    }
    
    flight_memory_string_t to_c_string(const std::string& str) {
        // Note: This is a simplified conversion. In production, proper memory management
        // would be needed for the string data lifetime
        flight_memory_string_t result;
        result.ptr = reinterpret_cast<uint8_t*>(const_cast<char*>(str.c_str()));
        result.len = str.length();
        return result;
    }
    
    MemorySize convert_memory_size(const exports_flight_memory_memory_types_memory_size_t& c_size) {
        return MemorySize{
            .bytes = c_size.bytes,
            .human_readable = convert_c_string(c_size.human_readable)
        };
    }
    
    exports_flight_memory_memory_types_memory_size_t to_c_memory_size(const MemorySize& size) {
        exports_flight_memory_memory_types_memory_size_t result;
        result.bytes = size.bytes;
        result.human_readable = to_c_string(size.human_readable);
        return result;
    }
    
    std::string generate_allocation_id() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);
        
        std::ostringstream oss;
        oss << "alloc_";
        for (int i = 0; i < 8; ++i) {
            oss << std::hex << dis(gen);
        }
        return oss.str();
    }
}

// MemoryOperations implementations
MemoryResult<MemoryAllocation> MemoryOperations::create_allocation(
    std::string_view session_id,
    const MemorySize& size,
    MemoryPurpose purpose) noexcept {
    
    try {
        // Convert to C types and call underlying C function
        auto c_session_id = to_c_string(std::string(session_id));
        auto c_size = to_c_memory_size(size);
        auto c_purpose = static_cast<exports_flight_memory_memory_operations_memory_purpose_t>(purpose);
        
        exports_flight_memory_memory_operations_memory_allocation_t c_result;
        exports_flight_memory_memory_operations_memory_error_t c_error;
        
        bool success = exports_flight_memory_memory_operations_create_allocation(
            &c_session_id, &c_size, c_purpose, &c_result, &c_error);
        
        if (success) {
            MemoryAllocation allocation{
                .id = convert_c_string(c_result.id),
                .session_id = convert_c_string(c_result.session_id),
                .size = convert_memory_size(c_result.size),
                .purpose = static_cast<MemoryPurpose>(c_result.purpose),
                .allocated_at = std::chrono::system_clock::from_time_t(c_result.allocated_at),
                .freed_at = c_result.freed_at.is_some ? 
                    std::optional<Timestamp>(std::chrono::system_clock::from_time_t(c_result.freed_at.val)) :
                    std::nullopt
            };
            return allocation;
        } else {
            auto error_code = static_cast<MemoryError::Code>(c_error.code);
            std::string message = convert_c_string(c_error.message);
            OptionalString details = c_error.details.is_some ? 
                std::optional<std::string>(convert_c_string(c_error.details.val)) : 
                std::nullopt;
            
            return MemoryError(error_code, std::move(message), std::move(details));
        }
    } catch (const std::exception& e) {
        return MemoryError(MemoryError::Code::AllocationFailed, e.what());
    }
}

MemoryResult<bool> MemoryOperations::free_allocation(std::string_view allocation_id) noexcept {
    try {
        auto c_allocation_id = to_c_string(std::string(allocation_id));
        
        bool c_result;
        exports_flight_memory_memory_operations_memory_error_t c_error;
        
        bool success = exports_flight_memory_memory_operations_free_allocation(
            &c_allocation_id, &c_result, &c_error);
        
        if (success) {
            return c_result;
        } else {
            auto error_code = static_cast<MemoryError::Code>(c_error.code);
            std::string message = convert_c_string(c_error.message);
            OptionalString details = c_error.details.is_some ? 
                std::optional<std::string>(convert_c_string(c_error.details.val)) : 
                std::nullopt;
            
            return MemoryError(error_code, std::move(message), std::move(details));
        }
    } catch (const std::exception& e) {
        return MemoryError(MemoryError::Code::AllocationFailed, e.what());
    }
}

MemoryResult<MemoryUsageSnapshot> MemoryOperations::get_memory_snapshot(
    std::string_view session_id) noexcept {
    
    try {
        auto c_session_id = to_c_string(std::string(session_id));
        
        exports_flight_memory_memory_operations_memory_usage_snapshot_t c_result;
        exports_flight_memory_memory_operations_memory_error_t c_error;
        
        bool success = exports_flight_memory_memory_operations_get_memory_snapshot(
            &c_session_id, &c_result, &c_error);
        
        if (success) {
            MemoryUsageSnapshot snapshot{
                .timestamp = std::chrono::system_clock::from_time_t(c_result.timestamp),
                .session_id = convert_c_string(c_result.session_id),
                .platform = convert_c_string(c_result.platform),
                .total = convert_memory_size(c_result.total),
                .used = convert_memory_size(c_result.used),
                .available = convert_memory_size(c_result.available),
                .fragmentation_ratio = c_result.fragmentation_ratio
            };
            return snapshot;
        } else {
            auto error_code = static_cast<MemoryError::Code>(c_error.code);
            std::string message = convert_c_string(c_error.message);
            OptionalString details = c_error.details.is_some ? 
                std::optional<std::string>(convert_c_string(c_error.details.val)) : 
                std::nullopt;
            
            return MemoryError(error_code, std::move(message), std::move(details));
        }
    } catch (const std::exception& e) {
        return MemoryError(MemoryError::Code::AllocationFailed, e.what());
    }
}

MemoryResult<PlatformProfile> MemoryOperations::get_platform_profile(
    std::string_view platform) noexcept {
    
    // For now, return a default implementation based on platform string
    try {
        if (platform == "dreamcast") return PlatformProfile::Dreamcast;
        if (platform == "psp") return PlatformProfile::Psp;
        if (platform == "vita") return PlatformProfile::Vita;
        if (platform == "v6r-small") return PlatformProfile::V6RSmall;
        if (platform == "v6r-medium") return PlatformProfile::V6RMedium;
        if (platform == "v6r-large") return PlatformProfile::V6RLarge;
        
        return PlatformProfile::Custom;
    } catch (const std::exception& e) {
        return MemoryError(MemoryError::Code::UnsupportedPlatform, e.what());
    }
}

MemoryResult<bool> MemoryOperations::set_memory_limits(
    std::string_view session_id,
    const MemoryLimits& limits) noexcept {
    
    // Simplified implementation - would integrate with actual C bindings
    try {
        // For now, just validate the limits make sense
        if (limits.heap_max.bytes == 0 || limits.stack_max.bytes == 0) {
            return MemoryError(MemoryError::Code::InvalidSize, "Memory limits cannot be zero");
        }
        
        return true;
    } catch (const std::exception& e) {
        return MemoryError(MemoryError::Code::AllocationFailed, e.what());
    }
}

MemoryResult<MemoryLimits> MemoryOperations::get_memory_limits(
    std::string_view session_id) noexcept {
    
    // Default implementation - would integrate with actual C bindings
    try {
        MemoryLimits limits{
            .heap_max = MemorySize::from_mb(256),
            .stack_max = MemorySize::from_mb(32),
            .cache_max = MemorySize::from_mb(64),
            .soft_limit = MemorySize::from_mb(300),
            .hard_limit = MemorySize::from_mb(400)
        };
        
        return limits;
    } catch (const std::exception& e) {
        return MemoryError(MemoryError::Code::AllocationFailed, e.what());
    }
}

MemoryResult<MemoryPressure> MemoryOperations::get_memory_pressure(
    std::string_view session_id) noexcept {
    
    try {
        // Get current snapshot and calculate pressure
        auto snapshot_result = get_memory_snapshot(session_id);
        if (is_err(snapshot_result)) {
            // Convert MemoryError to MemoryError (they're the same type, so just forward)
            return unwrap_err(snapshot_result);
        }
        
        const auto& snapshot = unwrap(snapshot_result);
        double usage = snapshot.usage_percentage();
        
        if (usage < 50.0) return MemoryPressure::Low;
        if (usage < 75.0) return MemoryPressure::Medium;
        if (usage < 90.0) return MemoryPressure::High;
        return MemoryPressure::Critical;
        
    } catch (const std::exception& e) {
        return MemoryError(MemoryError::Code::AllocationFailed, e.what());
    }
}

MemoryResult<bool> MemoryOperations::validate_allocation_request(
    std::string_view session_id,
    const MemorySize& size,
    MemoryPurpose purpose) noexcept {
    
    try {
        auto limits_result = get_memory_limits(session_id);
        if (is_err(limits_result)) {
            return unwrap_err(limits_result);
        }
        
        const auto& limits = unwrap(limits_result);
        return limits.validate_allocation(size);
        
    } catch (const std::exception& e) {
        return MemoryError(MemoryError::Code::AllocationFailed, e.what());
    }
}

MemoryResult<std::vector<MemoryAllocation>> MemoryOperations::list_allocations(
    std::string_view session_id) noexcept {
    
    // Simplified implementation - would integrate with actual C bindings
    try {
        std::vector<MemoryAllocation> allocations;
        
        // Return empty list for now - in real implementation would call C bindings
        return allocations;
        
    } catch (const std::exception& e) {
        return MemoryError(MemoryError::Code::AllocationFailed, e.what());
    }
}

} // namespace memory

namespace error {

// FlightError implementations
bool FlightError::is_recoverable() const noexcept {
    return severity_ != ErrorSeverity::Fatal && 
           severity_ != ErrorSeverity::Critical;
}

std::string FlightError::to_string() const {
    std::ostringstream oss;
    oss << "[" << id_ << "] ";
    
    switch (severity_) {
        case ErrorSeverity::Info: oss << "INFO"; break;
        case ErrorSeverity::Warning: oss << "WARN"; break;
        case ErrorSeverity::Error: oss << "ERROR"; break;
        case ErrorSeverity::Critical: oss << "CRITICAL"; break;
        case ErrorSeverity::Fatal: oss << "FATAL"; break;
    }
    
    oss << " [";
    switch (category_) {
        case ErrorCategory::Memory: oss << "Memory"; break;
        case ErrorCategory::Platform: oss << "Platform"; break;
        case ErrorCategory::Network: oss << "Network"; break;
        case ErrorCategory::Validation: oss << "Validation"; break;
        case ErrorCategory::Security: oss << "Security"; break;
        case ErrorCategory::Component: oss << "Component"; break;
        case ErrorCategory::ServiceIntegration: oss << "ServiceIntegration"; break;
        case ErrorCategory::FlightSystem: oss << "FlightSystem"; break;
        case ErrorCategory::Application: oss << "Application"; break;
        case ErrorCategory::Unknown: oss << "Unknown"; break;
    }
    oss << "]: " << message_;
    
    if (details_) {
        oss << " - " << *details_;
    }
    
    return oss.str();
}

// ErrorOperations implementations
FlightError ErrorOperations::create_error(ErrorSeverity severity, ErrorCategory category,
                                        std::string message, ErrorContext context) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1000, 9999);
    
    std::string id = "err_" + std::to_string(dis(gen));
    return FlightError(std::move(id), severity, category, std::move(message), std::move(context));
}

FlightError ErrorOperations::create_simple_error(ErrorSeverity severity, ErrorCategory category,
                                               std::string message, std::string source,
                                               std::string operation) {
    ErrorContext context{
        .source = std::move(source),
        .operation = std::move(operation)
    };
    
    return create_error(severity, category, std::move(message), std::move(context));
}

FlightError ErrorOperations::create_service_error(ServiceErrorCode code, std::string message,
                                                std::string service_id, OptionalString session_id) {
    ErrorContext context{
        .source = service_id,
        .operation = "service_operation",
        .session_id = session_id,
        .service_id = service_id
    };
    
    return create_error(ErrorSeverity::Error, ErrorCategory::ServiceIntegration, 
                       std::move(message), std::move(context));
}

FlightError ErrorOperations::create_platform_error(PlatformErrorCode code, std::string message,
                                                 std::string platform, std::string operation) {
    ErrorContext context{
        .source = "platform_detector",
        .operation = std::move(operation),
        .platform = platform
    };
    
    return create_error(ErrorSeverity::Error, ErrorCategory::Platform,
                       std::move(message), std::move(context));
}

FlightError ErrorOperations::create_network_error(NetworkErrorCode code, std::string message,
                                                OptionalString endpoint) {
    ErrorContext context{
        .source = "network_layer",
        .operation = "network_operation"
    };
    
    if (endpoint) {
        context.metadata["endpoint"] = *endpoint;
    }
    
    return create_error(ErrorSeverity::Error, ErrorCategory::Network,
                       std::move(message), std::move(context));
}

FlightError ErrorOperations::create_validation_error(std::string message,
                                                   std::vector<ValidationErrorDetails> details) {
    ErrorContext context{
        .source = "validator",
        .operation = "validation"
    };
    
    // Add validation details to metadata
    for (size_t i = 0; i < details.size(); ++i) {
        const auto& detail = details[i];
        std::string prefix = "validation_" + std::to_string(i) + "_";
        context.metadata[prefix + "field"] = detail.field;
        context.metadata[prefix + "rule"] = detail.rule;
        context.metadata[prefix + "expected"] = detail.expected;
        context.metadata[prefix + "actual"] = detail.actual;
    }
    
    return create_error(ErrorSeverity::Error, ErrorCategory::Validation,
                       std::move(message), std::move(context));
}

FlightError ErrorOperations::enrich_error(FlightError error, Metadata additional_context) {
    for (const auto& [key, value] : additional_context) {
        error.context_.metadata[key] = value;
    }
    return error;
}

FlightError ErrorOperations::chain_error(FlightError error, FlightError cause) {
    error.cause_ = cause.id();
    return error;
}

bool ErrorOperations::is_recoverable(const FlightError& error) {
    return error.is_recoverable();
}

std::vector<ErrorRecoverySuggestion> ErrorOperations::get_recovery_suggestions(const FlightError& error) {
    std::vector<ErrorRecoverySuggestion> suggestions;
    
    switch (error.category()) {
        case ErrorCategory::Memory:
            suggestions.push_back({
                RecoveryActionType::ReduceResources,
                "Reduce memory usage by freeing unused allocations",
                true, 3
            });
            suggestions.push_back({
                RecoveryActionType::Retry,
                "Retry operation after memory cleanup",
                true, 2
            });
            break;
            
        case ErrorCategory::Network:
            suggestions.push_back({
                RecoveryActionType::Retry,
                "Retry network operation with exponential backoff",
                true, 3
            });
            suggestions.push_back({
                RecoveryActionType::CheckStatus,
                "Check network connectivity and service status",
                false, 2
            });
            break;
            
        case ErrorCategory::Platform:
            suggestions.push_back({
                RecoveryActionType::TryAlternative,
                "Try alternative platform implementation",
                true, 2
            });
            suggestions.push_back({
                RecoveryActionType::UpdateConfig,
                "Update platform configuration",
                false, 1
            });
            break;
            
        default:
            suggestions.push_back({
                RecoveryActionType::ContactSupport,
                "Contact technical support for assistance",
                false, 1
            });
            break;
    }
    
    return suggestions;
}

std::string ErrorOperations::get_error_summary(const FlightError& error) {
    return error.to_string();
}

} // namespace error

namespace platform {

// PlatformInfo implementations
bool PlatformInfo::is_constrained() const noexcept {
    return capability == PlatformCapability::Minimal || 
           capability == PlatformCapability::Basic;
}

bool PlatformInfo::supports_threading() const noexcept {
    auto it = metadata.find("threading");
    return it != metadata.end() && it->second == "true";
}

bool PlatformInfo::supports_networking() const noexcept {
    auto it = metadata.find("networking");
    return it != metadata.end() && it->second != "false";
}

bool PlatformInfo::is_cloud_platform() const noexcept {
    return category == PlatformCategory::Cloud ||
           type == PlatformType::V6RVmSmall ||
           type == PlatformType::V6RVmMedium ||
           type == PlatformType::V6RVmLarge ||
           type == PlatformType::V6RContainer;
}

bool PlatformInfo::is_retro_platform() const noexcept {
    return category == PlatformCategory::RetroGaming ||
           type == PlatformType::Dreamcast ||
           type == PlatformType::Psp ||
           type == PlatformType::Vita;
}

// PlatformDetector implementations
error::FlightResult<PlatformInfo> PlatformDetector::detect_current_platform() noexcept {
    try {
        // Simple platform detection based on compile-time and runtime checks
        PlatformInfo info;
        
        #if defined(__APPLE__)
            info.id = "macos-native";
            info.name = "macOS Native";
            info.type = PlatformType::MacosNative;
            info.category = PlatformCategory::Desktop;
        #elif defined(_WIN32)
            info.id = "windows-native";
            info.name = "Windows Native";
            info.type = PlatformType::WindowsNative;
            info.category = PlatformCategory::Desktop;
        #elif defined(__linux__)
            info.id = "linux-native";
            info.name = "Linux Native";
            info.type = PlatformType::LinuxNative;
            info.category = PlatformCategory::Desktop;
        #else
            info.id = "unknown";
            info.name = "Unknown Platform";
            info.type = PlatformType::LinuxNative; // Default fallback
            info.category = PlatformCategory::Desktop;
        #endif
        
        info.capability = PlatformCapability::Full;
        
        // Set default memory info (would be detected at runtime in real implementation)
        info.memory = {
            .total_memory = memory::MemorySize::from_gb(8),
            .available_memory = memory::MemorySize::from_gb(6),
            .system_reserved = memory::MemorySize::from_gb(2),
            .stack_limit = memory::MemorySize::from_mb(32),
            .heap_limit = memory::MemorySize::from_gb(4),
            .has_virtual_memory = true,
            .has_mmu = true,
            .has_dma = true
        };
        
        // Set default CPU info
        info.cpu = {
            .architecture = CpuArchitecture::X86_64,
            .core_count = 4,
            .clock_speed_mhz = 2400,
            .has_fpu = true,
            .has_simd = true
        };
        
        // Set default graphics info
        info.graphics = {
            .apis = {GraphicsApi::Opengl, GraphicsApi::Vulkan},
            .max_texture_size = 4096,
            .video_memory = memory::MemorySize::from_mb(256),
            .has_hardware_accel = true,
            .color_depth = 32
        };
        
        // Set metadata
        info.metadata = {
            {"threading", "true"},
            {"networking", "true"},
            {"cpp_standard", "C++17"}
        };
        
        return info;
        
    } catch (const std::exception& e) {
        return error::ErrorOperations::create_platform_error(
            error::PlatformErrorCode::PlatformInitFailed,
            std::string("Platform detection failed: ") + e.what(),
            "unknown",
            "detect_current_platform"
        );
    }
}

PlatformInfo PlatformDetector::get_dreamcast_info() noexcept {
    return PlatformInfo{
        .id = "dreamcast",
        .name = "Sega Dreamcast",
        .type = PlatformType::Dreamcast,
        .category = PlatformCategory::RetroGaming,
        .capability = PlatformCapability::Minimal,
        .memory = {
            .total_memory = memory::MemorySize::from_mb(16),
            .available_memory = memory::MemorySize::from_mb(12),
            .system_reserved = memory::MemorySize::from_mb(4),
            .stack_limit = memory::MemorySize::from_mb(1),
            .heap_limit = memory::MemorySize::from_mb(8),
            .has_virtual_memory = false,
            .has_mmu = false,
            .has_dma = true
        },
        .cpu = {
            .architecture = CpuArchitecture::Sh4,
            .core_count = 1,
            .clock_speed_mhz = 200,
            .has_fpu = true,
            .has_simd = false
        },
        .graphics = {
            .apis = {GraphicsApi::Custom},
            .max_texture_size = 1024,
            .video_memory = memory::MemorySize::from_mb(8),
            .has_hardware_accel = true,
            .color_depth = 16
        },
        .metadata = {
            {"threading", "false"},
            {"networking", "basic"},
            {"cpp_standard", "C++17"},
            {"architecture", "sh4"}
        }
    };
}

PlatformInfo PlatformDetector::get_psp_info() noexcept {
    return PlatformInfo{
        .id = "psp",
        .name = "PlayStation Portable",
        .type = PlatformType::Psp,
        .category = PlatformCategory::RetroGaming,
        .capability = PlatformCapability::Basic,
        .memory = {
            .total_memory = memory::MemorySize::from_mb(32),
            .available_memory = memory::MemorySize::from_mb(24),
            .system_reserved = memory::MemorySize::from_mb(8),
            .stack_limit = memory::MemorySize::from_mb(2),
            .heap_limit = memory::MemorySize::from_mb(20),
            .has_virtual_memory = false,
            .has_mmu = false,
            .has_dma = true
        },
        .cpu = {
            .architecture = CpuArchitecture::Mips,
            .core_count = 1,
            .clock_speed_mhz = 333,
            .has_fpu = true,
            .has_simd = false
        },
        .graphics = {
            .apis = {GraphicsApi::Custom},
            .max_texture_size = 512,
            .video_memory = memory::MemorySize::from_mb(4),
            .has_hardware_accel = true,
            .color_depth = 16
        },
        .metadata = {
            {"threading", "limited"},
            {"networking", "wifi"},
            {"cpp_standard", "C++17"},
            {"architecture", "mips"}
        }
    };
}

PlatformInfo PlatformDetector::get_vita_info() noexcept {
    return PlatformInfo{
        .id = "vita",
        .name = "PlayStation Vita",
        .type = PlatformType::Vita,
        .category = PlatformCategory::RetroGaming,
        .capability = PlatformCapability::Standard,
        .memory = {
            .total_memory = memory::MemorySize::from_mb(512),
            .available_memory = memory::MemorySize::from_mb(400),
            .system_reserved = memory::MemorySize::from_mb(112),
            .stack_limit = memory::MemorySize::from_mb(16),
            .heap_limit = memory::MemorySize::from_mb(300),
            .has_virtual_memory = true,
            .has_mmu = true,
            .has_dma = true
        },
        .cpu = {
            .architecture = CpuArchitecture::Arm,
            .core_count = 4,
            .clock_speed_mhz = 444,
            .has_fpu = true,
            .has_simd = true
        },
        .graphics = {
            .apis = {GraphicsApi::OpenglEs},
            .max_texture_size = 2048,
            .video_memory = memory::MemorySize::from_mb(128),
            .has_hardware_accel = true,
            .color_depth = 32
        },
        .metadata = {
            {"threading", "true"},
            {"networking", "wifi"},
            {"cpp_standard", "C++17"},
            {"architecture", "arm"}
        }
    };
}

PlatformInfo PlatformDetector::get_v6r_info(std::string_view vm_size) noexcept {
    auto memory_size = memory::v6r::V6RMemoryUtils::get_vm_memory_limit(vm_size);
    
    PlatformType type;
    PlatformCapability capability;
    
    if (vm_size == "small") {
        type = PlatformType::V6RVmSmall;
        capability = PlatformCapability::Enhanced;
    } else if (vm_size == "medium") {
        type = PlatformType::V6RVmMedium;
        capability = PlatformCapability::Full;
    } else {
        type = PlatformType::V6RVmLarge;
        capability = PlatformCapability::Unlimited;
    }
    
    return PlatformInfo{
        .id = std::string("v6r-") + std::string(vm_size),
        .name = std::string("V6R ") + std::string(vm_size) + " VM",
        .type = type,
        .category = PlatformCategory::Cloud,
        .capability = capability,
        .memory = {
            .total_memory = memory_size,
            .available_memory = memory::MemorySize::from_bytes(memory_size.bytes * 8 / 10),
            .system_reserved = memory::MemorySize::from_bytes(memory_size.bytes * 2 / 10),
            .stack_limit = memory::MemorySize::from_mb(32),
            .heap_limit = memory::MemorySize::from_bytes(memory_size.bytes * 6 / 10),
            .has_virtual_memory = true,
            .has_mmu = true,
            .has_dma = true
        },
        .cpu = {
            .architecture = CpuArchitecture::X86_64,
            .core_count = static_cast<std::uint32_t>(vm_size == "small" ? 2 : vm_size == "medium" ? 4 : 8),
            .clock_speed_mhz = 2400,
            .has_fpu = true,
            .has_simd = true
        },
        .graphics = {
            .apis = {GraphicsApi::Vulkan, GraphicsApi::Opengl},
            .max_texture_size = 4096,
            .video_memory = memory::MemorySize::from_mb(256),
            .has_hardware_accel = true,
            .color_depth = 32
        },
        .metadata = {
            {"threading", "true"},
            {"networking", "full"},
            {"cpp_standard", "C++17"},
            {"container_runtime", "containerd"},
            {"orchestration", "kubernetes"},
            {"vm_size", std::string(vm_size)}
        }
    };
}

bool PlatformDetector::has_feature(const PlatformInfo& platform, std::string_view feature) noexcept {
    auto it = platform.metadata.find(std::string(feature));
    return it != platform.metadata.end() && it->second != "false";
}

std::vector<PlatformInfo> PlatformDetector::get_supported_platforms() noexcept {
    return {
        get_dreamcast_info(),
        get_psp_info(),
        get_vita_info(),
        get_v6r_info("small"),
        get_v6r_info("medium"),
        get_v6r_info("large")
    };
}

std::vector<PlatformInfo> PlatformDetector::filter_by_capability(PlatformCapability min_capability) noexcept {
    auto platforms = get_supported_platforms();
    std::vector<PlatformInfo> filtered;
    
    for (const auto& platform : platforms) {
        if (platform.capability >= min_capability) {
            filtered.push_back(platform);
        }
    }
    
    return filtered;
}

} // namespace platform

namespace session {

// SessionInfo implementations
bool SessionInfo::is_active() const noexcept {
    return state == SessionState::Active;
}

bool SessionInfo::is_expired() const noexcept {
    if (!expires_at) return false;
    return std::chrono::system_clock::now() > *expires_at;
}

std::chrono::duration<double> SessionInfo::age() const noexcept {
    return std::chrono::system_clock::now() - created_at;
}

bool SessionInfo::is_healthy() const noexcept {
    return is_active() && !is_expired();
}

// SessionOperations implementations (simplified for demonstration)
error::FlightResult<SessionInfo> SessionOperations::create_session(
    SessionType type, std::string platform, OptionalString user_id) noexcept {
    
    try {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(10000, 99999);
        
        SessionInfo info{
            .id = "session_" + std::to_string(dis(gen)),
            .type = type,
            .state = SessionState::Active,
            .platform = std::move(platform),
            .user_id = user_id,
            .parent_session_id = std::nullopt,
            .created_at = std::chrono::system_clock::now(),
            .last_activity = std::chrono::system_clock::now(),
            .expires_at = std::nullopt,
            .metadata = {
                {"created_by", "flight_integration"},
                {"cpp_standard", "C++17"}
            }
        };
        
        return info;
        
    } catch (const std::exception& e) {
        return error::ErrorOperations::create_simple_error(
            error::ErrorSeverity::Error,
            error::ErrorCategory::Component,
            std::string("Failed to create session: ") + e.what(),
            "session_operations",
            "create_session"
        );
    }
}

error::FlightResult<SessionInfo> SessionOperations::get_session(std::string_view session_id) noexcept {
    // Simplified implementation - would integrate with actual session storage
    try {
        // For demonstration, create a mock session
        SessionInfo info{
            .id = std::string(session_id),
            .type = SessionType::System,
            .state = SessionState::Active,
            .platform = "unknown",
            .user_id = std::nullopt,
            .parent_session_id = std::nullopt,
            .created_at = std::chrono::system_clock::now() - std::chrono::hours(1),
            .last_activity = std::chrono::system_clock::now(),
            .expires_at = std::nullopt,
            .metadata = {}
        };
        
        return info;
        
    } catch (const std::exception& e) {
        return error::ErrorOperations::create_simple_error(
            error::ErrorSeverity::Error,
            error::ErrorCategory::Component,
            std::string("Failed to get session: ") + e.what(),
            "session_operations",
            "get_session"
        );
    }
}

// Additional simplified implementations for other session operations
error::FlightResult<bool> SessionOperations::update_session_state(
    std::string_view session_id, SessionState new_state) noexcept {
    
    // Simplified implementation
    return true;
}

error::FlightResult<bool> SessionOperations::terminate_session(std::string_view session_id) noexcept {
    // Simplified implementation
    return true;
}

error::FlightResult<SessionResources> SessionOperations::get_session_resources(
    std::string_view session_id) noexcept {
    
    try {
        SessionResources resources{
            .memory = memory::v6r::V6RMemoryUtils::create_snapshot(
                session_id, "system", memory::MemorySize::from_mb(64)),
            .cpu_usage = 25.5f,
            .network_usage = 1024,
            .storage_usage = memory::MemorySize::from_mb(128),
            .connection_count = 2,
            .custom_metrics = {}
        };
        
        return resources;
        
    } catch (const std::exception& e) {
        return error::ErrorOperations::create_simple_error(
            error::ErrorSeverity::Error,
            error::ErrorCategory::Component,
            std::string("Failed to get session resources: ") + e.what(),
            "session_operations",
            "get_session_resources"
        );
    }
}

error::FlightResult<std::vector<SessionInfo>> SessionOperations::list_sessions(
    OptionalString user_id, std::optional<SessionType> type, OptionalString platform) noexcept {
    
    // Simplified implementation - return empty list
    return std::vector<SessionInfo>{};
}

error::FlightResult<bool> SessionOperations::extend_session(
    std::string_view session_id, std::uint64_t additional_seconds) noexcept {
    
    return true;
}

error::FlightResult<SessionHealth> SessionOperations::get_session_health(
    std::string_view session_id) noexcept {
    
    return SessionHealth::Healthy;
}

} // namespace session

namespace component {

// ComponentInfo implementations
bool ComponentInfo::is_running() const noexcept {
    return state == ComponentState::Running;
}

bool ComponentInfo::is_healthy() const noexcept {
    return state == ComponentState::Running || 
           state == ComponentState::Instantiated;
}

std::chrono::duration<double> ComponentInfo::uptime() const noexcept {
    return std::chrono::system_clock::now() - created_at;
}

// ComponentOperations implementations (simplified)
error::FlightResult<ComponentInfo> ComponentOperations::create_component(
    std::string_view name, std::string_view world, std::string_view platform) noexcept {
    
    try {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(10000, 99999);
        
        ComponentInfo info{
            .id = "comp_" + std::to_string(dis(gen)),
            .name = std::string(name),
            .version = "1.0.0",
            .state = ComponentState::Instantiated,
            .world = std::string(world),
            .platform = std::string(platform),
            .session_id = std::nullopt,
            .created_at = std::chrono::system_clock::now(),
            .last_activity = std::chrono::system_clock::now(),
            .memory_usage = memory::v6r::V6RMemoryUtils::create_snapshot(
                "component", platform, memory::MemorySize::from_mb(4)),
            .metadata = {
                {"created_by", "flight_integration"},
                {"cpp_standard", "C++17"}
            }
        };
        
        return info;
        
    } catch (const std::exception& e) {
        return error::ErrorOperations::create_simple_error(
            error::ErrorSeverity::Error,
            error::ErrorCategory::Component,
            std::string("Failed to create component: ") + e.what(),
            "component_operations",
            "create_component"
        );
    }
}

// Additional simplified implementations for other component operations
error::FlightResult<ComponentInfo> ComponentOperations::get_component(
    const ComponentId& component_id) noexcept {
    
    // Simplified implementation - would integrate with actual component storage
    try {
        ComponentInfo info{
            .id = component_id,
            .name = "Mock Component",
            .version = "1.0.0",
            .state = ComponentState::Running,
            .world = "flight:core-world",
            .platform = "system",
            .session_id = std::nullopt,
            .created_at = std::chrono::system_clock::now() - std::chrono::hours(1),
            .last_activity = std::chrono::system_clock::now(),
            .memory_usage = memory::v6r::V6RMemoryUtils::create_snapshot(
                "component", "system", memory::MemorySize::from_mb(8)),
            .metadata = {}
        };
        
        return info;
        
    } catch (const std::exception& e) {
        return error::ErrorOperations::create_simple_error(
            error::ErrorSeverity::Error,
            error::ErrorCategory::Component,
            std::string("Failed to get component: ") + e.what(),
            "component_operations",
            "get_component"
        );
    }
}

error::FlightResult<bool> ComponentOperations::update_component_state(
    const ComponentId& component_id, ComponentState new_state) noexcept {
    
    return true;
}

error::FlightResult<std::vector<ComponentInfo>> ComponentOperations::list_components(
    OptionalString platform, std::optional<ComponentState> state) noexcept {
    
    return std::vector<ComponentInfo>{};
}

error::FlightResult<ExecutionContext> ComponentOperations::get_execution_context(
    const ComponentId& component_id) noexcept {
    
    try {
        ExecutionContext context{
            .component = component_id,
            .stack_depth = 2,
            .available_memory = memory::MemorySize::from_mb(128),
            .cpu_time_ms = 1500,
            .priority = ExecutionPriority::Normal,
            .execution_mode = ExecutionMode::SingleThreaded
        };
        
        return context;
        
    } catch (const std::exception& e) {
        return error::ErrorOperations::create_simple_error(
            error::ErrorSeverity::Error,
            error::ErrorCategory::Component,
            std::string("Failed to get execution context: ") + e.what(),
            "component_operations",
            "get_execution_context"
        );
    }
}

} // namespace component

namespace integration {

// Helper function to convert MemoryResult to FlightResult
template<typename T>
error::FlightResult<T> convert_memory_result(const memory::MemoryResult<T>& memory_result) {
    if (memory::is_ok(memory_result)) {
        return memory::unwrap(memory_result);
    } else {
        const auto& mem_error = memory::unwrap_err(memory_result);
        return error::ErrorOperations::create_simple_error(
            error::ErrorSeverity::Error,
            error::ErrorCategory::Memory,
            mem_error.message(),
            "memory_subsystem",
            "memory_operation"
        );
    }
}

// FlightCoreIntegration implementations
error::FlightResult<platform::PlatformInfo> FlightCoreIntegration::initialize_platform() noexcept {
    return platform::PlatformDetector::detect_current_platform();
}

error::FlightResult<memory::MemoryUsageSnapshot> FlightCoreIntegration::get_system_memory() noexcept {
    // This would integrate with Flight-Core memory subsystem
    try {
        auto used = memory::MemorySize::from_mb(128);
        auto snapshot = memory::v6r::V6RMemoryUtils::create_snapshot(
            "system", "flight", used);
        return snapshot;
    } catch (const std::exception& e) {
        return error::ErrorOperations::create_simple_error(
            error::ErrorSeverity::Error,
            error::ErrorCategory::Memory,
            std::string("Failed to get system memory: ") + e.what(),
            "memory_subsystem",
            "get_system_memory"
        );
    }
}

error::FlightResult<component::ComponentInfo> FlightCoreIntegration::create_hal_component(
    std::string_view platform_id) noexcept {
    
    return component::ComponentOperations::create_component(
        "Flight-Core HAL", "flight:hal-world", platform_id);
}

error::FlightResult<component::ComponentInfo> FlightCoreIntegration::create_runtime_component(
    std::string_view platform_id) noexcept {
    
    return component::ComponentOperations::create_component(
        "Flight-Core Runtime", "flight:runtime-world", platform_id);
}

error::FlightResult<session::SessionInfo> FlightCoreIntegration::create_system_session(
    std::string_view platform_id) noexcept {
    
    return session::SessionOperations::create_session(
        session::SessionType::System, std::string(platform_id), std::nullopt);
}

error::FlightResult<session::SessionInfo> FlightCoreIntegration::create_v6r_session(
    std::string_view vm_size, std::string_view user_id) noexcept {
    
    auto platform = std::string("v6r-") + std::string(vm_size);
    return session::SessionOperations::create_session(
        session::SessionType::Development, platform, std::string(user_id));
}

error::FlightResult<memory::MemoryUsageSnapshot> FlightCoreIntegration::get_v6r_memory_usage(
    std::string_view session_id) noexcept {
    
    auto memory_result = memory::MemoryOperations::get_memory_snapshot(session_id);
    return convert_memory_result(memory_result);
}

} // namespace integration

} // namespace flight::shared_types
