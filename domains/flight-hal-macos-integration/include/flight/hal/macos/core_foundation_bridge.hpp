#pragma once

#include <CoreFoundation/CoreFoundation.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <variant>

namespace flight::hal::macos {

/// Error type for framework operations
struct FrameworkError {
    std::string message;
    int code;
    std::string category;
    
    FrameworkError(std::string msg, int error_code = 0, std::string cat = "CoreFoundation")
        : message(std::move(msg)), code(error_code), category(std::move(cat)) {}
};

/// Result type for error handling
template<typename T>
using Result = std::variant<T, FrameworkError>;

/// RAII wrapper for Core Foundation types
template<typename CFType>
class CFWrapper {
private:
    CFType ref_;

public:
    /// Constructor takes ownership of CF reference
    explicit CFWrapper(CFType ref = nullptr) : ref_(ref) {}
    
    /// Destructor releases CF reference
    ~CFWrapper() {
        if (ref_) {
            CFRelease(ref_);
        }
    }
    
    /// Move constructor
    CFWrapper(CFWrapper&& other) noexcept : ref_(other.ref_) {
        other.ref_ = nullptr;
    }
    
    /// Move assignment
    CFWrapper& operator=(CFWrapper&& other) noexcept {
        if (this != &other) {
            if (ref_) {
                CFRelease(ref_);
            }
            ref_ = other.ref_;
            other.ref_ = nullptr;
        }
        return *this;
    }
    
    /// Deleted copy operations
    CFWrapper(const CFWrapper&) = delete;
    CFWrapper& operator=(const CFWrapper&) = delete;
    
    /// Get the underlying CF reference
    CFType get() const { return ref_; }
    
    /// Release ownership and return the CF reference
    CFType release() {
        CFType tmp = ref_;
        ref_ = nullptr;
        return tmp;
    }
    
    /// Reset with a new CF reference
    void reset(CFType new_ref = nullptr) {
        if (ref_) {
            CFRelease(ref_);
        }
        ref_ = new_ref;
    }
    
    /// Check if wrapper contains a valid reference
    explicit operator bool() const { return ref_ != nullptr; }
    
    /// Implicit conversion to CF type for API calls
    operator CFType() const { return ref_; }
};

/// Type aliases for common CF wrappers
using CFStringWrapper = CFWrapper<CFStringRef>;
using CFDictionaryWrapper = CFWrapper<CFDictionaryRef>;
using CFMutableDictionaryWrapper = CFWrapper<CFMutableDictionaryRef>;
using CFArrayWrapper = CFWrapper<CFArrayRef>;
using CFMutableArrayWrapper = CFWrapper<CFMutableArrayRef>;
using CFDataWrapper = CFWrapper<CFDataRef>;
using CFNumberWrapper = CFWrapper<CFNumberRef>;
using CFBundleWrapper = CFWrapper<CFBundleRef>;
using CFRunLoopWrapper = CFWrapper<CFRunLoopRef>;

/// Core Foundation utility functions
class CoreFoundationBridge {
public:
    // String Utilities
    /// Convert CFString to std::string
    static std::optional<std::string> cfStringToStd(CFStringRef cf_str);
    
    /// Convert std::string to CFString (caller takes ownership)
    static CFStringWrapper stdToCFString(const std::string& str);
    
    /// Convert std::string to CFString with custom encoding
    static CFStringWrapper stdToCFString(const std::string& str, CFStringEncoding encoding);
    
    // Dictionary Utilities
    /// Convert CFDictionary to std::unordered_map<std::string, std::string>
    static std::unordered_map<std::string, std::string> cfDictionaryToStdMap(CFDictionaryRef dict);
    
    /// Convert std::unordered_map to CFDictionary
    static CFDictionaryWrapper stdMapToCFDictionary(const std::unordered_map<std::string, std::string>& map);
    
    /// Get string value from CFDictionary by key
    static std::optional<std::string> getStringFromDictionary(CFDictionaryRef dict, const std::string& key);
    
    /// Get number value from CFDictionary by key
    static std::optional<int64_t> getNumberFromDictionary(CFDictionaryRef dict, const std::string& key);
    
    // Array Utilities
    /// Convert CFArray of CFStrings to std::vector<std::string>
    static std::vector<std::string> cfStringArrayToStdVector(CFArrayRef array);
    
    /// Convert std::vector<std::string> to CFArray of CFStrings
    static CFArrayWrapper stdVectorToCFStringArray(const std::vector<std::string>& vec);
    
    // Number Utilities
    /// Create CFNumber from various numeric types
    static CFNumberWrapper createCFNumber(int32_t value);
    static CFNumberWrapper createCFNumber(int64_t value);
    static CFNumberWrapper createCFNumber(double value);
    
    /// Extract numeric value from CFNumber
    static std::optional<int32_t> cfNumberToInt32(CFNumberRef number);
    static std::optional<int64_t> cfNumberToInt64(CFNumberRef number);
    static std::optional<double> cfNumberToDouble(CFNumberRef number);
    
    // Data Utilities
    /// Create CFData from std::vector<uint8_t>
    static CFDataWrapper createCFData(const std::vector<uint8_t>& data);
    
    /// Convert CFData to std::vector<uint8_t>
    static std::vector<uint8_t> cfDataToStdVector(CFDataRef data);
    
    // Bundle Utilities
    /// Get main bundle
    static CFBundleWrapper getMainBundle();
    
    /// Get bundle by identifier
    static CFBundleWrapper getBundleByIdentifier(const std::string& identifier);
    
    /// Get resource path from bundle
    static std::optional<std::string> getBundleResourcePath(
        CFBundleRef bundle, 
        const std::string& name, 
        const std::string& type = "",
        const std::string& subdir = ""
    );
    
    // Run Loop Utilities
    /// Get current run loop
    static CFRunLoopWrapper getCurrentRunLoop();
    
    /// Get main run loop
    static CFRunLoopWrapper getMainRunLoop();
    
    // Error Handling
    /// Create FrameworkError from CF error
    static FrameworkError createErrorFromCFError(CFErrorRef error);
    
    /// Check if CFType is valid and create error if not
    template<typename T>
    static Result<T> validateCFType(T cf_ref, const std::string& operation) {
        if (cf_ref) {
            return cf_ref;
        }
        return FrameworkError("Failed to create CF object during: " + operation);
    }
    
    // Memory Management Helpers
    /// Retain CF object (for cases where we need to extend lifetime)
    template<typename CFType>
    static CFType retainCF(CFType ref) {
        if (ref) {
            CFRetain(ref);
        }
        return ref;
    }
    
    /// Safe release of CF object
    template<typename CFType>
    static void safeCFRelease(CFType& ref) {
        if (ref) {
            CFRelease(ref);
            ref = nullptr;
        }
    }
    
private:
    CoreFoundationBridge() = delete; // Static utility class
};

/// RAII wrapper for CFRunLoop sources and timers
class CFRunLoopManager {
public:
    CFRunLoopManager();
    ~CFRunLoopManager();
    
    /// Add timer to run loop
    bool addTimer(CFRunLoopTimerRef timer, CFRunLoopMode mode = kCFRunLoopDefaultMode);
    
    /// Remove timer from run loop
    bool removeTimer(CFRunLoopTimerRef timer, CFRunLoopMode mode = kCFRunLoopDefaultMode);
    
    /// Add source to run loop
    bool addSource(CFRunLoopSourceRef source, CFRunLoopMode mode = kCFRunLoopDefaultMode);
    
    /// Remove source from run loop
    bool removeSource(CFRunLoopSourceRef source, CFRunLoopMode mode = kCFRunLoopDefaultMode);
    
    /// Run the run loop for specified time
    void runFor(CFTimeInterval seconds);
    
    /// Stop the run loop
    void stop();
    
private:
    CFRunLoopWrapper run_loop_;
    std::vector<CFRunLoopTimerRef> timers_;
    std::vector<CFRunLoopSourceRef> sources_;
};

} // namespace flight::hal::macos
