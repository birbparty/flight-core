/**
 * C++17 Cross-Language Error Handling Tests
 * 
 * Comprehensive tests for error handling and cross-language compatibility
 * ensuring zero information loss across TypeScript, Go, Rust, and C++17
 * using modern C++17 features (std::variant, std::optional)
 */

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <optional>
#include <variant>
#include <chrono>
#include <memory>
#include <iostream>

using json = nlohmann::json;

namespace flight::cross_language::test {

// C++17 Error Types for Cross-Language Testing
enum class ErrorSeverity : std::uint8_t {
    Info,
    Warning,
    Error,
    Critical,
    Fatal
};

enum class ErrorCategory : std::uint8_t {
    Memory,
    Platform,
    Network,
    Validation,
    Security,
    Component,
    ServiceIntegration,
    FlightSystem,
    Application,
    Unknown
};

struct MetadataPair {
    std::string key;
    std::string value;
    
    bool operator==(const MetadataPair& other) const noexcept {
        return key == other.key && value == other.value;
    }
};

struct ErrorContext {
    std::string source;
    std::string operation;
    std::optional<std::string> session_id;
    std::optional<std::string> user_id;
    std::optional<std::string> platform;
    std::optional<std::string> service_id;
    std::vector<MetadataPair> metadata;
    
    // C++17 convenience methods
    void add_metadata(std::string_view key, std::string_view value) {
        metadata.emplace_back(MetadataPair{std::string(key), std::string(value)});
    }
    
    std::optional<std::string> get_metadata(std::string_view key) const {
        for (const auto& pair : metadata) {
            if (pair.key == key) {
                return pair.value;
            }
        }
        return std::nullopt;
    }
};

struct FlightError {
    std::string id;
    ErrorSeverity severity;
    ErrorCategory category;
    std::string message;
    std::optional<std::string> details;
    ErrorContext context;
    std::chrono::system_clock::time_point timestamp;
    std::optional<std::string> cause;
    
    // C++17 constructor with default timestamp
    FlightError(std::string id, ErrorSeverity severity, ErrorCategory category,
                std::string message, ErrorContext context)
        : id(std::move(id)), severity(severity), category(category),
          message(std::move(message)), context(std::move(context)),
          timestamp(std::chrono::system_clock::now()) {}
    
    // C++17 utilities
    [[nodiscard]] bool is_recoverable() const noexcept {
        return severity != ErrorSeverity::Fatal && 
               category != ErrorCategory::Security;
    }
    
    [[nodiscard]] std::string to_string() const {
        std::string result = "[" + severity_to_string(severity) + "] " +
                           category_to_string(category) + ": " + message;
        if (details) {
            result += " (" + *details + ")";
        }
        return result;
    }
    
    // JSON serialization support
    [[nodiscard]] json to_json() const {
        json j;
        j["id"] = id;
        j["severity"] = severity_to_string(severity);
        j["category"] = category_to_string(category);
        j["message"] = message;
        if (details) j["details"] = *details;
        
        // Context serialization
        j["context"]["source"] = context.source;
        j["context"]["operation"] = context.operation;
        if (context.session_id) j["context"]["session_id"] = *context.session_id;
        if (context.user_id) j["context"]["user_id"] = *context.user_id;
        if (context.platform) j["context"]["platform"] = *context.platform;
        if (context.service_id) j["context"]["service_id"] = *context.service_id;
        
        j["context"]["metadata"] = json::array();
        for (const auto& pair : context.metadata) {
            j["context"]["metadata"].push_back({{"key", pair.key}, {"value", pair.value}});
        }
        
        j["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
            timestamp.time_since_epoch()).count();
        if (cause) j["cause"] = *cause;
        
        return j;
    }
    
    // Static factory from JSON
    static std::optional<FlightError> from_json(const json& j) {
        try {
            std::string id = j.at("id");
            ErrorSeverity severity = string_to_severity(j.at("severity"));
            ErrorCategory category = string_to_category(j.at("category"));
            std::string message = j.at("message");
            
            ErrorContext context;
            context.source = j.at("context").at("source");
            context.operation = j.at("context").at("operation");
            
            if (j.at("context").contains("session_id") && !j.at("context").at("session_id").is_null()) {
                context.session_id = j.at("context").at("session_id");
            }
            if (j.at("context").contains("user_id") && !j.at("context").at("user_id").is_null()) {
                context.user_id = j.at("context").at("user_id");
            }
            if (j.at("context").contains("platform") && !j.at("context").at("platform").is_null()) {
                context.platform = j.at("context").at("platform");
            }
            if (j.at("context").contains("service_id") && !j.at("context").at("service_id").is_null()) {
                context.service_id = j.at("context").at("service_id");
            }
            
            if (j.at("context").contains("metadata")) {
                for (const auto& meta : j.at("context").at("metadata")) {
                    context.metadata.emplace_back(MetadataPair{meta.at("key"), meta.at("value")});
                }
            }
            
            FlightError error(std::move(id), severity, category, std::move(message), std::move(context));
            
            if (j.contains("details") && !j.at("details").is_null()) {
                error.details = j.at("details");
            }
            if (j.contains("cause") && !j.at("cause").is_null()) {
                error.cause = j.at("cause");
            }
            if (j.contains("timestamp")) {
                auto timestamp_seconds = j.at("timestamp").get<std::int64_t>();
                error.timestamp = std::chrono::system_clock::from_time_t(timestamp_seconds);
            }
            
            return error;
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }

private:
    static std::string severity_to_string(ErrorSeverity severity) {
        switch (severity) {
            case ErrorSeverity::Info: return "info";
            case ErrorSeverity::Warning: return "warning";
            case ErrorSeverity::Error: return "error";
            case ErrorSeverity::Critical: return "critical";
            case ErrorSeverity::Fatal: return "fatal";
            default: return "unknown";
        }
    }
    
    static std::string category_to_string(ErrorCategory category) {
        switch (category) {
            case ErrorCategory::Memory: return "memory";
            case ErrorCategory::Platform: return "platform";
            case ErrorCategory::Network: return "network";
            case ErrorCategory::Validation: return "validation";
            case ErrorCategory::Security: return "security";
            case ErrorCategory::Component: return "component";
            case ErrorCategory::ServiceIntegration: return "service-integration";
            case ErrorCategory::FlightSystem: return "flight-system";
            case ErrorCategory::Application: return "application";
            case ErrorCategory::Unknown: return "unknown";
            default: return "unknown";
        }
    }
    
    static ErrorSeverity string_to_severity(const std::string& str) {
        if (str == "info") return ErrorSeverity::Info;
        if (str == "warning") return ErrorSeverity::Warning;
        if (str == "error") return ErrorSeverity::Error;
        if (str == "critical") return ErrorSeverity::Critical;
        if (str == "fatal") return ErrorSeverity::Fatal;
        return ErrorSeverity::Unknown;
    }
    
    static ErrorCategory string_to_category(const std::string& str) {
        if (str == "memory") return ErrorCategory::Memory;
        if (str == "platform") return ErrorCategory::Platform;
        if (str == "network") return ErrorCategory::Network;
        if (str == "validation") return ErrorCategory::Validation;
        if (str == "security") return ErrorCategory::Security;
        if (str == "component") return ErrorCategory::Component;
        if (str == "service-integration") return ErrorCategory::ServiceIntegration;
        if (str == "flight-system") return ErrorCategory::FlightSystem;
        if (str == "application") return ErrorCategory::Application;
        return ErrorCategory::Unknown;
    }
};

// C++17 Result type using std::variant
template<typename T>
using FlightResult = std::variant<T, FlightError>;

// Helper functions for std::variant result handling
template<typename T>
[[nodiscard]] constexpr bool is_ok(const FlightResult<T>& result) noexcept {
    return std::holds_alternative<T>(result);
}

template<typename T>
[[nodiscard]] constexpr bool is_err(const FlightResult<T>& result) noexcept {
    return std::holds_alternative<FlightError>(result);
}

template<typename T>
[[nodiscard]] const T& unwrap(const FlightResult<T>& result) {
    return std::get<T>(result);
}

template<typename T>
[[nodiscard]] const FlightError& unwrap_err(const FlightResult<T>& result) {
    return std::get<FlightError>(result);
}

template<typename T>
[[nodiscard]] T unwrap_or(const FlightResult<T>& result, T&& default_value) {
    if (is_ok(result)) {
        return unwrap(result);
    }
    return std::forward<T>(default_value);
}

// Test fixture
class Cpp17CrossLanguageErrorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup common test data
    }
    
    std::string generate_error_id() {
        static std::size_t counter = 0;
        return "cpp17-error-" + std::to_string(++counter);
    }
    
    ErrorContext create_test_context(std::string_view source, std::string_view operation) {
        ErrorContext context;
        context.source = source;
        context.operation = operation;
        return context;
    }
};

TEST_F(Cpp17CrossLanguageErrorTest, BasicErrorRoundTripSerialization) {
    // Create comprehensive FlightError
    auto context = create_test_context("cpp17-cross-lang-memory-manager", "allocate_cross_lang_buffer");
    context.session_id = "cpp17-cross-session-123";
    context.user_id = "cpp17-cross-user-456";
    context.platform = "dreamcast";
    context.service_id = "cpp17-cross-service-789";
    
    context.add_metadata("language_source", "c++17");
    context.add_metadata("language_target", "typescript");
    context.add_metadata("requested_bytes", "8388608");
    context.add_metadata("available_bytes", "4194304");
    context.add_metadata("fragmentation_ratio", "0.25");
    context.add_metadata("cross_lang_call_depth", "4");
    context.add_metadata("memory_pressure", "critical");
    context.add_metadata("cpp_standard", "C++17");
    context.add_metadata("std_variant_usage", "true");

    FlightError originalError(
        generate_error_id(),
        ErrorSeverity::Error,
        ErrorCategory::Memory,
        "C++17 cross-language memory allocation failed",
        std::move(context)
    );
    originalError.details = "Insufficient heap space for std::variant allocation across language boundary";
    originalError.cause = "cpp17-cross-language-variant-overhead";

    // Test JSON serialization
    json serialized = originalError.to_json();
    EXPECT_FALSE(serialized.empty());
    
    std::string jsonString = serialized.dump();
    EXPECT_FALSE(jsonString.empty());

    // Test JSON deserialization
    json parsedJson = json::parse(jsonString);
    auto deserializedOpt = FlightError::from_json(parsedJson);
    ASSERT_TRUE(deserializedOpt.has_value());
    
    const auto& deserialized = *deserializedOpt;
    
    // Verify all core fields preserved
    EXPECT_EQ(originalError.id, deserialized.id);
    EXPECT_EQ(originalError.severity, deserialized.severity);
    EXPECT_EQ(originalError.category, deserialized.category);
    EXPECT_EQ(originalError.message, deserialized.message);
    EXPECT_EQ(originalError.details, deserialized.details);
    EXPECT_EQ(originalError.cause, deserialized.cause);
    
    // Verify complete context preservation
    EXPECT_EQ(originalError.context.source, deserialized.context.source);
    EXPECT_EQ(originalError.context.operation, deserialized.context.operation);
    EXPECT_EQ(originalError.context.session_id, deserialized.context.session_id);
    EXPECT_EQ(originalError.context.user_id, deserialized.context.user_id);
    EXPECT_EQ(originalError.context.platform, deserialized.context.platform);
    EXPECT_EQ(originalError.context.service_id, deserialized.context.service_id);
    
    // Verify metadata preservation
    EXPECT_EQ(originalError.context.metadata.size(), deserialized.context.metadata.size());
    for (std::size_t i = 0; i < originalError.context.metadata.size(); ++i) {
        EXPECT_EQ(originalError.context.metadata[i], deserialized.context.metadata[i]);
    }
    
    // Test specific cross-language metadata
    EXPECT_EQ(deserialized.context.get_metadata("language_source"), "c++17");
    EXPECT_EQ(deserialized.context.get_metadata("language_target"), "typescript");
    EXPECT_EQ(deserialized.context.get_metadata("cross_lang_call_depth"), "4");
    EXPECT_EQ(deserialized.context.get_metadata("std_variant_usage"), "true");
}

TEST_F(Cpp17CrossLanguageErrorTest, OptionalFieldHandling) {
    // Test std::optional usage with nullptr/nullopt fields
    auto context = create_test_context("cpp17-optional-test", "test_optional_handling");
    // Deliberately leave optional fields as nullopt
    context.add_metadata("optional_test", "true");
    context.add_metadata("cpp_feature", "std::optional");

    FlightError errorWithOptionals(
        generate_error_id(),
        ErrorSeverity::Warning,
        ErrorCategory::Platform,
        "C++17 cross-language optional field test",
        std::move(context)
    );
    // details remains nullopt
    // cause remains nullopt

    json serialized = errorWithOptionals.to_json();
    auto deserializedOpt = FlightError::from_json(serialized);
    ASSERT_TRUE(deserializedOpt.has_value());
    
    const auto& deserialized = *deserializedOpt;

    // Verify optional field handling
    EXPECT_FALSE(deserialized.details.has_value());
    EXPECT_FALSE(deserialized.context.session_id.has_value());
    EXPECT_FALSE(deserialized.context.user_id.has_value());
    EXPECT_FALSE(deserialized.context.platform.has_value());
    EXPECT_FALSE(deserialized.context.service_id.has_value());
    EXPECT_FALSE(deserialized.cause.has_value());
    
    // Verify non-optional fields preserved
    EXPECT_EQ(errorWithOptionals.id, deserialized.id);
    EXPECT_EQ(deserialized.context.get_metadata("optional_test"), "true");
    EXPECT_EQ(deserialized.context.get_metadata("cpp_feature"), "std::optional");
}

TEST_F(Cpp17CrossLanguageErrorTest, VariantResultPatterns) {
    // Test successful result using std::variant
    std::string successValue = "cpp17-cross-lang-success";
    FlightResult<std::string> successResult = successValue;
    
    EXPECT_TRUE(is_ok(successResult));
    EXPECT_FALSE(is_err(successResult));
    
    if (is_ok(successResult)) {
        const auto& value = unwrap(successResult);
        EXPECT_EQ(value, "cpp17-cross-lang-success");
    }

    // Test error result using std::variant
    auto context = create_test_context("cpp17-variant-test", "test_std_variant");
    context.add_metadata("variant_type", "FlightResult");
    context.add_metadata("cpp_feature", "std::variant");
    context.add_metadata("test_case", "error_handling");

    FlightError testError(
        generate_error_id(),
        ErrorSeverity::Critical,
        ErrorCategory::Platform,
        "C++17 std::variant error test",
        std::move(context)
    );
    testError.details = "Testing C++17 std::variant error handling patterns";

    FlightResult<std::string> errorResult = testError;
    
    EXPECT_FALSE(is_ok(errorResult));
    EXPECT_TRUE(is_err(errorResult));
    
    if (is_err(errorResult)) {
        const auto& error = unwrap_err(errorResult);
        EXPECT_EQ(error.id, testError.id);
        EXPECT_EQ(error.severity, ErrorSeverity::Critical);
        EXPECT_TRUE(error.details.has_value());
        EXPECT_EQ(error.details.value(), "Testing C++17 std::variant error handling patterns");
    }
    
    // Test unwrap_or pattern
    std::string fallbackValue = unwrap_or(errorResult, std::string("fallback"));
    EXPECT_EQ(fallbackValue, "fallback");
}

TEST_F(Cpp17CrossLanguageErrorTest, PlatformSpecificErrorHandling) {
    // Test Dreamcast-specific error handling
    auto dreamcastContext = create_test_context("cpp17-dreamcast-allocator", "cpp17_texture_allocation");
    dreamcastContext.session_id = "dreamcast-cpp17-session";
    dreamcastContext.platform = "dreamcast";
    
    dreamcastContext.add_metadata("total_memory_bytes", "16777216"); // 16MB
    dreamcastContext.add_metadata("available_memory_bytes", "2097152"); // 2MB
    dreamcastContext.add_metadata("requested_allocation_bytes", "12582912"); // 12MB
    dreamcastContext.add_metadata("cpp_heap_usage_bytes", "8388608"); // 8MB
    dreamcastContext.add_metadata("std_variant_overhead_bytes", "1048576"); // 1MB
    dreamcastContext.add_metadata("allocation_type", "cpp17_cross_lang_texture_buffer");
    dreamcastContext.add_metadata("source_language", "c++17");
    dreamcastContext.add_metadata("target_language", "rust");
    dreamcastContext.add_metadata("hardware_arch", "sh4");
    dreamcastContext.add_metadata("memory_fragmentation", "0.35");
    dreamcastContext.add_metadata("cpp_standard", "C++17");

    FlightError dreamcastError(
        generate_error_id(),
        ErrorSeverity::Critical,
        ErrorCategory::Platform,
        "Dreamcast memory constraint violation in C++17 cross-language operation",
        std::move(dreamcastContext)
    );
    dreamcastError.details = "C++17 std::variant allocation exceeded 16MB Dreamcast limit";
    dreamcastError.cause = "dreamcast-cpp17-memory-limit";

    // Test Flight-Core specific error handling utilities
    EXPECT_TRUE(dreamcastError.is_recoverable()); // Critical but not Fatal, so recoverable
    
    std::string summary = dreamcastError.to_string();
    EXPECT_TRUE(summary.find("CRITICAL") != std::string::npos);
    EXPECT_TRUE(summary.find("PLATFORM") != std::string::npos);
    EXPECT_TRUE(summary.find("Dreamcast") != std::string::npos);

    // Test JSON serialization of platform-specific error
    json serialized = dreamcastError.to_json();
    auto deserializedOpt = FlightError::from_json(serialized);
    ASSERT_TRUE(deserializedOpt.has_value());
    
    const auto& deserialized = *deserializedOpt;
    EXPECT_EQ(deserialized.context.platform, "dreamcast");
    EXPECT_EQ(deserialized.context.get_metadata("total_memory_bytes"), "16777216");
    EXPECT_EQ(deserialized.context.get_metadata("source_language"), "c++17");
    EXPECT_EQ(deserialized.context.get_metadata("target_language"), "rust");
    EXPECT_EQ(deserialized.context.get_metadata("hardware_arch"), "sh4");
}

TEST_F(Cpp17CrossLanguageErrorTest, V6RCloudScalingError) {
    // Test V6R-specific error handling
    auto v6rContext = create_test_context("cpp17-v6r-memory-monitor", "monitor_cpp17_cross_lang_memory");
    v6rContext.session_id = "v6r-cpp17-dev-session-123";
    v6rContext.user_id = "v6r-cpp17-developer-456";
    v6rContext.platform = "v6r-large";
    v6rContext.service_id = "v6r-cpp17-memory-service";
    
    v6rContext.add_metadata("vm_memory_limit_bytes", "2147483648"); // 2GB
    v6rContext.add_metadata("current_usage_bytes", "1932735283"); // 1.8GB (90%)
    v6rContext.add_metadata("cpp_heap_usage_bytes", "1073741824"); // 1GB
    v6rContext.add_metadata("std_variant_overhead_bytes", "104857600"); // 100MB
    v6rContext.add_metadata("cross_lang_overhead_bytes", "209715200"); // 200MB
    v6rContext.add_metadata("usage_percentage", "90.0");
    v6rContext.add_metadata("warning_threshold", "85.0");
    v6rContext.add_metadata("language_breakdown", "c++17:50%,typescript:20%,go:15%,rust:10%,overhead:5%");
    v6rContext.add_metadata("vm_tier", "large");
    v6rContext.add_metadata("scaling_available", "true");
    v6rContext.add_metadata("auto_scale_threshold", "95.0");
    v6rContext.add_metadata("cpp_standard", "C++17");
    v6rContext.add_metadata("std_optional_usage", "extensive");

    FlightError v6rError(
        generate_error_id(),
        ErrorSeverity::Warning,
        ErrorCategory::ServiceIntegration,
        "V6R VM approaching memory limit during C++17 cross-language operation",
        std::move(v6rContext)
    );
    v6rError.details = "C++17 heap usage at 90% of V6R large VM limit with std::variant overhead";

    json serialized = v6rError.to_json();
    auto deserializedOpt = FlightError::from_json(serialized);
    ASSERT_TRUE(deserializedOpt.has_value());
    
    const auto& deserialized = *deserializedOpt;
    EXPECT_EQ(deserialized.context.platform, "v6r-large");
    EXPECT_EQ(deserialized.context.user_id, "v6r-cpp17-developer-456");
    EXPECT_EQ(deserialized.context.get_metadata("vm_memory_limit_bytes"), "2147483648");
    EXPECT_EQ(deserialized.context.get_metadata("language_breakdown"), "c++17:50%,typescript:20%,go:15%,rust:10%,overhead:5%");
    EXPECT_EQ(deserialized.context.get_metadata("auto_scale_threshold"), "95.0");
    EXPECT_EQ(deserialized.context.get_metadata("cpp_standard"), "C++17");
}

TEST_F(Cpp17CrossLanguageErrorTest, ErrorEnrichmentAndChaining) {
    // Test error context enrichment patterns
    auto baseContext = create_test_context("cpp17-component-loader", "load_cross_lang_component");
    baseContext.add_metadata("language_layer", "c++17");
    baseContext.add_metadata("cpp_call_stack_depth", "7");
    baseContext.add_metadata("cpp_memory_usage", "4194304");
    baseContext.add_metadata("cpp_processing_time_ms", "45");

    FlightError baseError(
        generate_error_id(),
        ErrorSeverity::Error,
        ErrorCategory::Component,
        "C++17 cross-language component initialization failed",
        std::move(baseContext)
    );

    // Simulate enrichment as error propagates through language layers
    baseError.context.add_metadata("language_layer", "typescript");
    baseError.context.add_metadata("ts_call_stack_depth", "3");
    baseError.context.add_metadata("ts_memory_usage", "1048576");
    baseError.context.add_metadata("ts_processing_time_ms", "120");

    baseError.context.add_metadata("language_layer", "go");
    baseError.context.add_metadata("go_goroutine_id", "789");
    baseError.context.add_metadata("go_memory_usage", "2097152");
    baseError.context.add_metadata("go_processing_time_ms", "75");

    baseError.context.add_metadata("language_layer", "rust");
    baseError.context.add_metadata("rust_thread_id", "1011");
    baseError.context.add_metadata("rust_memory_usage", "524288");
    baseError.context.add_metadata("rust_processing_time_ms", "20");
    baseError.context.add_metadata("total_chain_time_ms", "260");

    // Test serialization preserves full enrichment chain
    json serialized = baseError.to_json();
    auto deserializedOpt = FlightError::from_json(serialized);
    ASSERT_TRUE(deserializedOpt.has_value());
    
    const auto& deserialized = *deserializedOpt;
    EXPECT_EQ(deserialized.id, baseError.id); // Same error ID throughout chain
    
    // Verify all language layers preserved
    EXPECT_TRUE(deserialized.context.get_metadata("cpp_call_stack_depth").has_value());
    EXPECT_TRUE(deserialized.context.get_metadata("ts_call_stack_depth").has_value());
    EXPECT_TRUE(deserialized.context.get_metadata("go_goroutine_id").has_value());
    EXPECT_TRUE(deserialized.context.get_metadata("rust_thread_id").has_value());
    
    // Verify cumulative data
    EXPECT_EQ(deserialized.context.get_metadata("total_chain_time_ms"), "260");
    
    // Verify metadata count preservation (important for debugging)
    EXPECT_GT(deserialized.context.metadata.size(), 12);
}

TEST_F(Cpp17CrossLanguageErrorTest, PerformanceAndLargeCollections) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Create large collection of errors
    std::vector<FlightError> errors;
    errors.reserve(1000);
    
    for (int i = 0; i < 1000; ++i) {
        ErrorSeverity severity = (i % 2 == 0) ? ErrorSeverity::Warning : ErrorSeverity::Error;
        
        auto context = create_test_context("cpp17-performance-test", "large_collection_test");
        context.add_metadata("test_index", std::to_string(i));
        context.add_metadata("language", "c++17");
        context.add_metadata("std_variant", "true");
        
        errors.emplace_back(
            "cpp17-perf-" + std::to_string(i),
            severity,
            ErrorCategory::Memory,
            "C++17 large collection test error " + std::to_string(i),
            std::move(context)
        );
    }
    
    // Test serialization performance
    auto serialize_start = std::chrono::high_resolution_clock::now();
    json serialized = json::array();
    for (const auto& error : errors) {
        serialized.push_back(error.to_json());
    }
    auto serialize_time = std::chrono::high_resolution_clock::now() - serialize_start;
    
    EXPECT_FALSE(serialized.empty());
    EXPECT_EQ(serialized.size(), 1000);
    
    std::string jsonString = serialized.dump();
    EXPECT_FALSE(jsonString.empty());
    
    // Test deserialization performance
    auto deserialize_start = std::chrono::high_resolution_clock::now();
    std::vector<FlightError> deserialized;
    deserialized.reserve(1000);
    
    for (const auto& errorJson : serialized) {
        auto errorOpt = FlightError::from_json(errorJson);
        ASSERT_TRUE(errorOpt.has_value());
        deserialized.push_back(std::move(*errorOpt));
    }
    auto deserialize_time = std::chrono::high_resolution_clock::now() - deserialize_start;
    
    EXPECT_EQ(deserialized.size(), 1000);
    
    auto total_time = std::chrono::high_resolution_clock::now() - start_time;
    
    std::cout << "C++17 large collection test completed in " 
              << std::chrono::duration_cast<std::chrono::milliseconds>(total_time).count() << "ms"
              << " (serialize: " << std::chrono::duration_cast<std::chrono::milliseconds>(serialize_time).count() << "ms"
              << ", deserialize: " << std::chrono::duration_cast<std::chrono::milliseconds>(deserialize_time).count() << "ms)"
              << std::endl;
    
    // Basic performance assertions
    EXPECT_LT(serialize_time, std::chrono::seconds(1)); // Should serialize in under 1 second
    EXPECT_LT(deserialize_time, std::chrono::milliseconds(500)); // Should deserialize in under 0.5 seconds
}

TEST_F(Cpp17CrossLanguageErrorTest, UnicodeAndSpecialCharacters) {
    // Test Unicode and special character handling
    auto unicodeContext = create_test_context("cpp17-unicode-test-service", "test_unicode_handling");
    unicodeContext.session_id = "cpp17-unicode-session-🎯";
    unicodeContext.user_id = "cpp17-user-émile-测试";
    unicodeContext.platform = "test-platform";
    
    unicodeContext.add_metadata("unicode_message", "🌍 Global C++17 test");
    unicodeContext.add_metadata("special_chars", "\\n\\t\\r\\\"\\\\");
    unicodeContext.add_metadata("languages", "日本語,中文,English,Français,C++17");
    unicodeContext.add_metadata("emoji_test", "🚀🔥💻🎮🎯🌍");
    unicodeContext.add_metadata("cpp_standard", "C++17");

    FlightError unicodeError(
        generate_error_id(),
        ErrorSeverity::Error,
        ErrorCategory::Application,
        "C++17 Unicode test: 🚀🔥💻 Cross-language 日本語 测试",
        std::move(unicodeContext)
    );
    unicodeError.details = "Testing special characters: \\n\\t\\r\\\"\\\\and émojis 🎮";

    json serialized = unicodeError.to_json();
    auto deserializedOpt = FlightError::from_json(serialized);
    ASSERT_TRUE(deserializedOpt.has_value());
    
    const auto& deserialized = *deserializedOpt;

    EXPECT_EQ(deserialized.message, "C++17 Unicode test: 🚀🔥💻 Cross-language 日本語 测试");
    EXPECT_EQ(deserialized.details, "Testing special characters: \\n\\t\\r\\\"\\\\and émojis 🎮");
    EXPECT_EQ(deserialized.context.session_id, "cpp17-unicode-session-🎯");
    EXPECT_EQ(deserialized.context.user_id, "cpp17-user-émile-测试");

    EXPECT_EQ(deserialized.context.get_metadata("unicode_message"), "🌍 Global C++17 test");
    EXPECT_EQ(deserialized.context.get_metadata("emoji_test"), "🚀🔥💻🎮🎯🌍");
}

TEST_F(Cpp17CrossLanguageErrorTest, MalformedJsonHandling) {
    // Test handling of malformed JSON
    std::vector<std::string> malformedJsonCases = {
        R"({"id":"test")", // Incomplete JSON
        R"({"id":})", // Invalid JSON syntax
        R"({"severity":"invalid_severity"})", // Invalid enum value
        "", // Empty string
        "null", // Null value
        "[]", // Wrong type (array instead of object)
        R"({"message":123})", // Wrong type for message field
    };

    for (const auto& malformedJson : malformedJsonCases) {
        try {
            json parsedJson = json::parse(malformedJson);
            auto errorOpt = FlightError::from_json(parsedJson);
            EXPECT_FALSE(errorOpt.has_value()) << "Should have failed to parse: " << malformedJson;
        } catch (const json::parse_error&) {
            // Expected behavior for truly malformed JSON
            SUCCEED();
        }
    }
}

TEST_F(Cpp17CrossLanguageErrorTest, ErrorChaining) {
    // Test error chaining patterns
    auto rootCauseContext = create_test_context("cpp17-network-client", "cpp17_api_call");
    rootCauseContext.add_metadata("network_error_code", "connection_timeout");
    rootCauseContext.add_metadata("endpoint", "https://api.service.com/cpp17-cross-lang");

    FlightError rootCause(
        generate_error_id(),
        ErrorSeverity::Warning,
        ErrorCategory::Network,
        "C++17 cross-language API call timeout",
        std::move(rootCauseContext)
    );

    auto serviceContext = create_test_context("cpp17-service-bridge", "cpp17_service_call");
    serviceContext.add_metadata("service_error_code", "service_unavailable");
    serviceContext.add_metadata("service_id", "cpp17-cross-lang-service");

    FlightError serviceError(
        generate_error_id(),
        ErrorSeverity::Error,
        ErrorCategory::ServiceIntegration,
        "Service unavailable due to network issues",
        std::move(serviceContext)
    );
    serviceError.cause = rootCause.id;

    auto appContext = create_test_context("cpp17-app-service", "cpp17_cross_lang_operation");
    FlightError applicationError(
        generate_error_id(),
        ErrorSeverity::Error,
        ErrorCategory::Application,
        "C++17 application operation failed",
        std::move(appContext)
    );
    applicationError.cause = serviceError.id;

    // Test serialization preserves error chain
    json serialized = applicationError.to_json();
    auto deserializedOpt = FlightError::from_json(serialized);
    ASSERT_TRUE(deserializedOpt.has_value());
    
    const auto& deserialized = *deserializedOpt;
    EXPECT_TRUE(deserialized.cause.has_value());
    EXPECT_EQ(*deserialized.cause, serviceError.id);

    // Verify error hierarchy preserved
    EXPECT_EQ(deserialized.id, applicationError.id);
    EXPECT_EQ(deserialized.message, "C++17 application operation failed");
}

} // namespace flight::cross_language::test

// Test runner main function (if building as standalone executable)
#ifdef STANDALONE_TEST_RUNNER
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
#endif
