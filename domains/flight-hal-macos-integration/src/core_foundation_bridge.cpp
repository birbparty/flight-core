#include "flight/hal/macos/core_foundation_bridge.hpp"
#include <algorithm>
#include <cassert>

namespace flight::hal::macos {

// String Utilities Implementation
std::optional<std::string> CoreFoundationBridge::cfStringToStd(CFStringRef cf_str) {
    if (!cf_str) {
        return std::nullopt;
    }
    
    CFIndex length = CFStringGetLength(cf_str);
    if (length == 0) {
        return std::string{};
    }
    
    // Try to get direct pointer to UTF-8 bytes first (most efficient)
    const char* fast_ptr = CFStringGetCStringPtr(cf_str, kCFStringEncodingUTF8);
    if (fast_ptr) {
        return std::string(fast_ptr);
    }
    
    // Fallback: get the maximum buffer size needed and copy
    CFIndex max_size = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
    std::string result(max_size, '\0');
    
    Boolean success = CFStringGetCString(cf_str, result.data(), max_size, kCFStringEncodingUTF8);
    if (success) {
        // Resize to actual length (excluding null terminator)
        result.resize(std::strlen(result.c_str()));
        return result;
    }
    
    return std::nullopt;
}

CFStringWrapper CoreFoundationBridge::stdToCFString(const std::string& str) {
    return stdToCFString(str, kCFStringEncodingUTF8);
}

CFStringWrapper CoreFoundationBridge::stdToCFString(const std::string& str, CFStringEncoding encoding) {
    CFStringRef cf_str = CFStringCreateWithCString(kCFAllocatorDefault, str.c_str(), encoding);
    return CFStringWrapper(cf_str);
}

// Dictionary Utilities Implementation
std::unordered_map<std::string, std::string> CoreFoundationBridge::cfDictionaryToStdMap(CFDictionaryRef dict) {
    std::unordered_map<std::string, std::string> result;
    
    if (!dict) {
        return result;
    }
    
    CFIndex count = CFDictionaryGetCount(dict);
    if (count == 0) {
        return result;
    }
    
    std::vector<const void*> keys(count);
    std::vector<const void*> values(count);
    
    CFDictionaryGetKeysAndValues(dict, keys.data(), values.data());
    
    for (CFIndex i = 0; i < count; ++i) {
        // Convert key
        auto key_str = cfStringToStd(static_cast<CFStringRef>(keys[i]));
        if (!key_str) continue;
        
        // Convert value (assume it's a string)
        auto value_str = cfStringToStd(static_cast<CFStringRef>(values[i]));
        if (!value_str) continue;
        
        result[*key_str] = *value_str;
    }
    
    return result;
}

CFDictionaryWrapper CoreFoundationBridge::stdMapToCFDictionary(const std::unordered_map<std::string, std::string>& map) {
    if (map.empty()) {
        return CFDictionaryWrapper(CFDictionaryCreate(kCFAllocatorDefault, nullptr, nullptr, 0, 
            &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
    }
    
    std::vector<CFStringRef> keys;
    std::vector<CFStringRef> values;
    keys.reserve(map.size());
    values.reserve(map.size());
    
    // Create CF strings for keys and values
    for (const auto& [key, value] : map) {
        auto cf_key = stdToCFString(key);
        auto cf_value = stdToCFString(value);
        
        if (cf_key && cf_value) {
            keys.push_back(cf_key.release());
            values.push_back(cf_value.release());
        }
    }
    
    // Create dictionary
    CFDictionaryRef dict = CFDictionaryCreate(kCFAllocatorDefault,
        reinterpret_cast<const void**>(keys.data()),
        reinterpret_cast<const void**>(values.data()),
        keys.size(),
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks);
    
    // Release the individual strings (dictionary retains them)
    for (CFStringRef key : keys) {
        CFRelease(key);
    }
    for (CFStringRef value : values) {
        CFRelease(value);
    }
    
    return CFDictionaryWrapper(dict);
}

std::optional<std::string> CoreFoundationBridge::getStringFromDictionary(CFDictionaryRef dict, const std::string& key) {
    if (!dict) {
        return std::nullopt;
    }
    
    auto cf_key = stdToCFString(key);
    if (!cf_key) {
        return std::nullopt;
    }
    
    CFStringRef value = static_cast<CFStringRef>(CFDictionaryGetValue(dict, cf_key.get()));
    return cfStringToStd(value);
}

std::optional<int64_t> CoreFoundationBridge::getNumberFromDictionary(CFDictionaryRef dict, const std::string& key) {
    if (!dict) {
        return std::nullopt;
    }
    
    auto cf_key = stdToCFString(key);
    if (!cf_key) {
        return std::nullopt;
    }
    
    CFNumberRef number = static_cast<CFNumberRef>(CFDictionaryGetValue(dict, cf_key.get()));
    return cfNumberToInt64(number);
}

// Array Utilities Implementation
std::vector<std::string> CoreFoundationBridge::cfStringArrayToStdVector(CFArrayRef array) {
    std::vector<std::string> result;
    
    if (!array) {
        return result;
    }
    
    CFIndex count = CFArrayGetCount(array);
    result.reserve(count);
    
    for (CFIndex i = 0; i < count; ++i) {
        CFStringRef str = static_cast<CFStringRef>(CFArrayGetValueAtIndex(array, i));
        auto std_str = cfStringToStd(str);
        if (std_str) {
            result.push_back(*std_str);
        }
    }
    
    return result;
}

CFArrayWrapper CoreFoundationBridge::stdVectorToCFStringArray(const std::vector<std::string>& vec) {
    if (vec.empty()) {
        return CFArrayWrapper(CFArrayCreate(kCFAllocatorDefault, nullptr, 0, &kCFTypeArrayCallBacks));
    }
    
    std::vector<CFStringRef> cf_strings;
    cf_strings.reserve(vec.size());
    
    for (const auto& str : vec) {
        auto cf_str = stdToCFString(str);
        if (cf_str) {
            cf_strings.push_back(cf_str.release());
        }
    }
    
    CFArrayRef array = CFArrayCreate(kCFAllocatorDefault,
        reinterpret_cast<const void**>(cf_strings.data()),
        cf_strings.size(),
        &kCFTypeArrayCallBacks);
    
    // Release individual strings (array retains them)
    for (CFStringRef str : cf_strings) {
        CFRelease(str);
    }
    
    return CFArrayWrapper(array);
}

// Number Utilities Implementation
CFNumberWrapper CoreFoundationBridge::createCFNumber(int32_t value) {
    return CFNumberWrapper(CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &value));
}

CFNumberWrapper CoreFoundationBridge::createCFNumber(int64_t value) {
    return CFNumberWrapper(CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt64Type, &value));
}

CFNumberWrapper CoreFoundationBridge::createCFNumber(double value) {
    return CFNumberWrapper(CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType, &value));
}

std::optional<int32_t> CoreFoundationBridge::cfNumberToInt32(CFNumberRef number) {
    if (!number) {
        return std::nullopt;
    }
    
    int32_t value;
    if (CFNumberGetValue(number, kCFNumberSInt32Type, &value)) {
        return value;
    }
    
    return std::nullopt;
}

std::optional<int64_t> CoreFoundationBridge::cfNumberToInt64(CFNumberRef number) {
    if (!number) {
        return std::nullopt;
    }
    
    int64_t value;
    if (CFNumberGetValue(number, kCFNumberSInt64Type, &value)) {
        return value;
    }
    
    return std::nullopt;
}

std::optional<double> CoreFoundationBridge::cfNumberToDouble(CFNumberRef number) {
    if (!number) {
        return std::nullopt;
    }
    
    double value;
    if (CFNumberGetValue(number, kCFNumberDoubleType, &value)) {
        return value;
    }
    
    return std::nullopt;
}

// Data Utilities Implementation
CFDataWrapper CoreFoundationBridge::createCFData(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return CFDataWrapper(CFDataCreate(kCFAllocatorDefault, nullptr, 0));
    }
    
    return CFDataWrapper(CFDataCreate(kCFAllocatorDefault, data.data(), data.size()));
}

std::vector<uint8_t> CoreFoundationBridge::cfDataToStdVector(CFDataRef data) {
    if (!data) {
        return {};
    }
    
    CFIndex length = CFDataGetLength(data);
    if (length == 0) {
        return {};
    }
    
    const UInt8* bytes = CFDataGetBytePtr(data);
    return std::vector<uint8_t>(bytes, bytes + length);
}

// Bundle Utilities Implementation
CFBundleWrapper CoreFoundationBridge::getMainBundle() {
    return CFBundleWrapper(retainCF(CFBundleGetMainBundle()));
}

CFBundleWrapper CoreFoundationBridge::getBundleByIdentifier(const std::string& identifier) {
    auto cf_identifier = stdToCFString(identifier);
    if (!cf_identifier) {
        return CFBundleWrapper(nullptr);
    }
    
    return CFBundleWrapper(retainCF(CFBundleGetBundleWithIdentifier(cf_identifier.get())));
}

std::optional<std::string> CoreFoundationBridge::getBundleResourcePath(
    CFBundleRef bundle, 
    const std::string& name, 
    const std::string& type,
    const std::string& subdir) {
    
    if (!bundle) {
        return std::nullopt;
    }
    
    auto cf_name = stdToCFString(name);
    if (!cf_name) {
        return std::nullopt;
    }
    
    CFStringWrapper cf_type = type.empty() ? CFStringWrapper(nullptr) : stdToCFString(type);
    CFStringWrapper cf_subdir = subdir.empty() ? CFStringWrapper(nullptr) : stdToCFString(subdir);
    
    CFURLRef url = CFBundleCopyResourceURL(bundle, cf_name.get(), cf_type.get(), cf_subdir.get());
    if (!url) {
        return std::nullopt;
    }
    
    CFStringRef path = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
    CFRelease(url);
    
    if (!path) {
        return std::nullopt;
    }
    
    auto result = cfStringToStd(path);
    CFRelease(path);
    
    return result;
}

// Run Loop Utilities Implementation
CFRunLoopWrapper CoreFoundationBridge::getCurrentRunLoop() {
    return CFRunLoopWrapper(retainCF(CFRunLoopGetCurrent()));
}

CFRunLoopWrapper CoreFoundationBridge::getMainRunLoop() {
    return CFRunLoopWrapper(retainCF(CFRunLoopGetMain()));
}

// Error Handling Implementation
FrameworkError CoreFoundationBridge::createErrorFromCFError(CFErrorRef error) {
    if (!error) {
        return FrameworkError("Unknown Core Foundation error");
    }
    
    CFIndex code = CFErrorGetCode(error);
    
    auto description = cfStringToStd(CFErrorCopyDescription(error));
    std::string message = description ? *description : "Unknown error";
    
    auto domain = cfStringToStd(CFErrorGetDomain(error));
    std::string category = domain ? *domain : "CoreFoundation";
    
    return FrameworkError(std::move(message), static_cast<int>(code), std::move(category));
}

// CFRunLoopManager Implementation
CFRunLoopManager::CFRunLoopManager() : run_loop_(CoreFoundationBridge::getCurrentRunLoop()) {
}

CFRunLoopManager::~CFRunLoopManager() {
    // Remove all timers and sources
    for (auto timer : timers_) {
        if (CFRunLoopTimerIsValid(timer)) {
            CFRunLoopRemoveTimer(run_loop_.get(), timer, kCFRunLoopDefaultMode);
        }
    }
    
    for (auto source : sources_) {
        if (CFRunLoopSourceIsValid(source)) {
            CFRunLoopRemoveSource(run_loop_.get(), source, kCFRunLoopDefaultMode);
        }
    }
}

bool CFRunLoopManager::addTimer(CFRunLoopTimerRef timer, CFRunLoopMode mode) {
    if (!timer || !run_loop_) {
        return false;
    }
    
    CFRunLoopAddTimer(run_loop_.get(), timer, mode);
    timers_.push_back(timer);
    return true;
}

bool CFRunLoopManager::removeTimer(CFRunLoopTimerRef timer, CFRunLoopMode mode) {
    if (!timer || !run_loop_) {
        return false;
    }
    
    CFRunLoopRemoveTimer(run_loop_.get(), timer, mode);
    
    auto it = std::find(timers_.begin(), timers_.end(), timer);
    if (it != timers_.end()) {
        timers_.erase(it);
    }
    
    return true;
}

bool CFRunLoopManager::addSource(CFRunLoopSourceRef source, CFRunLoopMode mode) {
    if (!source || !run_loop_) {
        return false;
    }
    
    CFRunLoopAddSource(run_loop_.get(), source, mode);
    sources_.push_back(source);
    return true;
}

bool CFRunLoopManager::removeSource(CFRunLoopSourceRef source, CFRunLoopMode mode) {
    if (!source || !run_loop_) {
        return false;
    }
    
    CFRunLoopRemoveSource(run_loop_.get(), source, mode);
    
    auto it = std::find(sources_.begin(), sources_.end(), source);
    if (it != sources_.end()) {
        sources_.erase(it);
    }
    
    return true;
}

void CFRunLoopManager::runFor(CFTimeInterval seconds) {
    if (run_loop_) {
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, seconds, false);
    }
}

void CFRunLoopManager::stop() {
    if (run_loop_) {
        CFRunLoopStop(run_loop_.get());
    }
}

} // namespace flight::hal::macos
