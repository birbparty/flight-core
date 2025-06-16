#include "flight/hal/macos/platform_detector.hpp"

#ifdef __APPLE__
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <IOKit/IOKitLib.h>
#import <sys/sysctl.h>
#endif

#include <string>
#include <vector>
#include <optional>

namespace flight::hal::macos {

#ifdef __APPLE__

namespace {

// Helper function to convert NSString to std::string
std::string NSStringToStdString(NSString* nsString) {
    if (!nsString) return "";
    return std::string([nsString UTF8String]);
}

// Helper to get macOS version
std::string getMacOSVersion() {
    NSOperatingSystemVersion version = [[NSProcessInfo processInfo] operatingSystemVersion];
    return std::to_string(version.majorVersion) + "." + 
           std::to_string(version.minorVersion) + "." + 
           std::to_string(version.patchVersion);
}

// Helper to detect sandboxing
bool isSandboxed() {
    // Check if we're running in an App Sandbox
    SecCodeRef codeRef = NULL;
    OSStatus status = SecCodeCopySelf(kSecCSDefaultFlags, &codeRef);
    if (status != errSecSuccess) {
        return false;
    }
    
    CFDictionaryRef signingInfo = NULL;
    status = SecCodeCopySigningInformation(codeRef, kSecCSSigningInformation, &signingInfo);
    CFRelease(codeRef);
    
    if (status != errSecSuccess) {
        return false;
    }
    
    bool sandboxed = false;
    CFDictionaryRef entitlements = (CFDictionaryRef)CFDictionaryGetValue(signingInfo, kSecCodeInfoEntitlementsDict);
    if (entitlements) {
        CFBooleanRef sandboxValue = (CFBooleanRef)CFDictionaryGetValue(entitlements, CFSTR("com.apple.security.app-sandbox"));
        if (sandboxValue) {
            sandboxed = CFBooleanGetValue(sandboxValue);
        }
    }
    
    CFRelease(signingInfo);
    return sandboxed;
}

// Helper to check SIP status
bool isSIPEnabled() {
    // Use system configuration to check SIP status
    int sip_status = 0;
    size_t size = sizeof(sip_status);
    
    // This is a simplified check - in practice SIP detection can be more complex
    if (sysctlbyname("kern.sip_status", &sip_status, &size, NULL, 0) == 0) {
        return sip_status != 0;
    }
    
    // Default to assuming SIP is enabled for security
    return true;
}

} // anonymous namespace

#endif

// Enhanced GPU detection with Metal integration
PlatformCapabilities::GPU PlatformDetector::detect_gpu_capabilities() {
    PlatformCapabilities::GPU gpu = {};
    
#ifdef __APPLE__
    @autoreleasepool {
        // Get the default Metal device
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        
        if (device) {
            gpu.metal_support = true;
            
            // Get device name and family information
            NSString* deviceName = [device name];
            gpu.family = NSStringToStdString(deviceName);
            
            // Detect Metal feature set and capabilities
            if ([device supportsFamily:MTLGPUFamilyApple8]) {
                gpu.family = "Apple8"; // M2 generation
                gpu.metal_version = "3.0";
            } else if ([device supportsFamily:MTLGPUFamilyApple7]) {
                gpu.family = "Apple7"; // M1 generation  
                gpu.metal_version = "3.0";
            } else if ([device supportsFamily:MTLGPUFamilyApple9]) {
                gpu.family = "Apple9"; // M3 generation
                gpu.metal_version = "3.1";
            }
            
            // Check for ray tracing support (M3 and later)
            gpu.supports_raytracing = [device supportsRaytracing];
            
            // Check for mesh shading support
            if (@available(macOS 13.0, *)) {
                gpu.supports_mesh_shading = [device supportsMeshShaders];
            } else {
                gpu.supports_mesh_shading = false;
            }
            
            // Check for function pointer support
            if (@available(macOS 13.0, *)) {
                gpu.supports_function_pointers = [device supportsFunctionPointers];
            } else {
                gpu.supports_function_pointers = false;
            }
            
            // Get memory information
            if ([device hasUnifiedMemory]) {
                gpu.memory_shared_mb = [device recommendedMaxWorkingSetSize] / (1024 * 1024);
            }
            
            // Estimate GPU core count based on device characteristics
            std::string model_name = NSStringToStdString(deviceName);
            if (model_name.find("M4") != std::string::npos) {
                if (model_name.find("Max") != std::string::npos) {
                    gpu.core_count = 40; // M4 Max
                    gpu.family = "Apple10";
                } else if (model_name.find("Pro") != std::string::npos) {
                    gpu.core_count = 20; // M4 Pro
                    gpu.family = "Apple10";
                } else {
                    gpu.core_count = 10; // Base M4
                    gpu.family = "Apple10";
                }
            } else if (model_name.find("M3") != std::string::npos) {
                if (model_name.find("Max") != std::string::npos) {
                    gpu.core_count = 40; // M3 Max
                } else if (model_name.find("Pro") != std::string::npos) {
                    gpu.core_count = 18; // M3 Pro
                } else {
                    gpu.core_count = 10; // Base M3
                }
                gpu.family = "Apple9";
            } else if (model_name.find("M2") != std::string::npos) {
                if (model_name.find("Max") != std::string::npos) {
                    gpu.core_count = 38; // M2 Max
                } else if (model_name.find("Pro") != std::string::npos) {
                    gpu.core_count = 19; // M2 Pro
                } else {
                    gpu.core_count = 10; // Base M2
                }
                gpu.family = "Apple8";
            } else if (model_name.find("M1") != std::string::npos) {
                if (model_name.find("Max") != std::string::npos) {
                    gpu.core_count = 32; // M1 Max
                } else if (model_name.find("Pro") != std::string::npos) {
                    gpu.core_count = 16; // M1 Pro
                } else {
                    gpu.core_count = 8; // Base M1
                }
                gpu.family = "Apple7";
            }
            
        } else {
            // Fallback if Metal device creation fails
            gpu.metal_support = false;
            gpu.core_count = 0;
            gpu.family = "Unknown";
        }
    }
#else
    // Non-Apple platforms
    gpu.metal_support = false;
    gpu.core_count = 0;
    gpu.family = "Unknown";
#endif
    
    return gpu;
}

// Enhanced system capabilities detection
PlatformCapabilities::System PlatformDetector::detect_system_capabilities() {
    PlatformCapabilities::System system = {};
    
#ifdef __APPLE__
    @autoreleasepool {
        // Get macOS version
        system.macos_version = getMacOSVersion();
        
        // Get model identifier
        size_t size = 0;
        sysctlbyname("hw.model", nullptr, &size, nullptr, 0);
        if (size > 0) {
            std::string model(size, '\0');
            sysctlbyname("hw.model", model.data(), &size, nullptr, 0);
            if (!model.empty() && model.back() == '\0') {
                model.pop_back();
            }
            system.model_identifier = model;
        }
        
        // Get chip identifier
        system.chip_identifier = identify_chip_from_model(system.model_identifier);
        
        // Security and sandboxing detection
        system.sandboxed = isSandboxed();
        system.sip_enabled = isSIPEnabled();
    }
#endif
    
    return system;
}

// Development environment detection
bool PlatformDetector::is_development_environment() {
#ifdef __APPLE__
    @autoreleasepool {
        // Check for Xcode installation
        NSString* xcodePath = [[NSWorkspace sharedWorkspace] fullPathForApplication:@"Xcode"];
        if (xcodePath) {
            return true;
        }
        
        // Check for command line tools
        NSTask* task = [[NSTask alloc] init];
        task.launchPath = @"/usr/bin/xcode-select";
        task.arguments = @[@"-p"];
        
        NSPipe* pipe = [NSPipe pipe];
        task.standardOutput = pipe;
        task.standardError = pipe;
        
        @try {
            [task launch];
            [task waitUntilExit];
            
            if (task.terminationStatus == 0) {
                return true;
            }
        } @catch (NSException* exception) {
            // Command failed, probably no command line tools
        }
        
        return false;
    }
#else
    return false;
#endif
}

std::optional<std::string> PlatformDetector::xcode_path() {
#ifdef __APPLE__
    @autoreleasepool {
        NSString* xcodePath = [[NSWorkspace sharedWorkspace] fullPathForApplication:@"Xcode"];
        if (xcodePath) {
            return NSStringToStdString(xcodePath);
        }
        
        // Check command line tools path
        NSTask* task = [[NSTask alloc] init];
        task.launchPath = @"/usr/bin/xcode-select";
        task.arguments = @[@"-p"];
        
        NSPipe* pipe = [NSPipe pipe];
        task.standardOutput = pipe;
        task.standardError = pipe;
        
        @try {
            [task launch];
            [task waitUntilExit];
            
            if (task.terminationStatus == 0) {
                NSData* data = [[pipe fileHandleForReading] readDataToEndOfFile];
                NSString* output = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
                NSString* trimmed = [output stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
                return NSStringToStdString(trimmed);
            }
        } @catch (NSException* exception) {
            // Command failed
        }
        
        return std::nullopt;
    }
#else
    return std::nullopt;
#endif
}

// Enhanced development environment detection
DevelopmentEnvironment PlatformDetector::get_development_environment() {
    DevelopmentEnvironment env = {};
    
#ifdef __APPLE__
    @autoreleasepool {
        // Check for Xcode
        NSString* xcodePath = [[NSWorkspace sharedWorkspace] fullPathForApplication:@"Xcode"];
        env.has_xcode = (xcodePath != nil);
        if (xcodePath) {
            env.xcode_path = NSStringToStdString(xcodePath);
            
            // Get Xcode version
            NSBundle* xcodeBundle = [NSBundle bundleWithPath:xcodePath];
            if (xcodeBundle) {
                NSString* version = [xcodeBundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
                if (version) {
                    env.xcode_version = NSStringToStdString(version);
                }
            }
        }
        
        // Check for command line tools
        NSTask* task = [[NSTask alloc] init];
        task.launchPath = @"/usr/bin/xcode-select";
        task.arguments = @[@"-p"];
        
        NSPipe* pipe = [NSPipe pipe];
        task.standardOutput = pipe;
        task.standardError = pipe;
        
        @try {
            [task launch];
            [task waitUntilExit];
            env.has_command_line_tools = (task.terminationStatus == 0);
        } @catch (NSException* exception) {
            env.has_command_line_tools = false;
        }
        
        // Check for Instruments
        NSString* instrumentsPath = [[NSWorkspace sharedWorkspace] fullPathForApplication:@"Instruments"];
        env.has_instruments = (instrumentsPath != nil);
        
        // Check for Simulator
        NSString* simulatorPath = [[NSWorkspace sharedWorkspace] fullPathForApplication:@"Simulator"];
        env.has_simulator = (simulatorPath != nil);
        
        // Check for available SDKs
        if (env.has_command_line_tools) {
            NSTask* sdkTask = [[NSTask alloc] init];
            sdkTask.launchPath = @"/usr/bin/xcodebuild";
            sdkTask.arguments = @[@"-showsdks"];
            
            NSPipe* sdkPipe = [NSPipe pipe];
            sdkTask.standardOutput = sdkPipe;
            sdkTask.standardError = sdkPipe;
            
            @try {
                [sdkTask launch];
                [sdkTask waitUntilExit];
                
                if (sdkTask.terminationStatus == 0) {
                    NSData* data = [[sdkPipe fileHandleForReading] readDataToEndOfFile];
                    NSString* output = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
                    NSArray* lines = [output componentsSeparatedByString:@"\n"];
                    
                    for (NSString* line in lines) {
                        if ([line containsString:@"macosx"] || [line containsString:@"iphoneos"] || [line containsString:@"iphonesimulator"]) {
                            env.available_sdks.push_back(NSStringToStdString([line stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]]));
                        }
                    }
                }
            } @catch (NSException* exception) {
                // Failed to get SDK list
            }
        }
        
        // Check code signing capability
        env.code_signing_available = env.has_command_line_tools && !isSandboxed();
    }
#endif
    
    return env;
}

// Metal Performance Shaders support detection
bool PlatformDetector::supports_metal_performance_shaders() {
#ifdef __APPLE__
    @autoreleasepool {
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (!device) {
            return false;
        }
        
        // Check if the device supports compute shaders and Metal Performance Shaders
        return [device supportsFamily:MTLGPUFamilyApple7] || [device supportsFamily:MTLGPUFamilyApple8] || [device supportsFamily:MTLGPUFamilyApple9];
    }
#else
    return false;
#endif
}

} // namespace flight::hal::macos
