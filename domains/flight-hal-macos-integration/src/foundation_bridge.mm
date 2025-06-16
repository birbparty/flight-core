#include "flight/hal/macos/foundation_bridge.hpp"
#import <Foundation/Foundation.h>
#import <AppKit/NSWorkspace.h>
#include <chrono>
#include <future>
#include <thread>

namespace flight::hal::macos {

// Utility functions for NS <-> std conversions
std::string FoundationBridge::nsStringToStd(void* nsString) {
    if (!nsString) return "";
    NSString* str = (__bridge NSString*)nsString;
    return std::string([str UTF8String] ?: "");
}

void* FoundationBridge::stdToNSString(const std::string& str) {
    return (__bridge_retained void*)[[NSString alloc] initWithUTF8String:str.c_str()];
}

std::vector<std::string> FoundationBridge::nsArrayToStdVector(void* nsArray) {
    std::vector<std::string> result;
    if (!nsArray) return result;
    
    NSArray* array = (__bridge NSArray*)nsArray;
    for (id obj in array) {
        if ([obj isKindOfClass:[NSString class]]) {
            result.push_back(nsStringToStd((__bridge void*)obj));
        }
    }
    return result;
}

void* FoundationBridge::stdVectorToNSArray(const std::vector<std::string>& vec) {
    NSMutableArray* array = [[NSMutableArray alloc] initWithCapacity:vec.size()];
    for (const auto& str : vec) {
        [array addObject:[NSString stringWithUTF8String:str.c_str()]];
    }
    return (__bridge_retained void*)array;
}

std::unordered_map<std::string, std::string> FoundationBridge::nsDictionaryToStdMap(void* nsDictionary) {
    std::unordered_map<std::string, std::string> result;
    if (!nsDictionary) return result;
    
    NSDictionary* dict = (__bridge NSDictionary*)nsDictionary;
    for (NSString* key in dict) {
        NSString* value = dict[key];
        if ([key isKindOfClass:[NSString class]] && [value isKindOfClass:[NSString class]]) {
            result[nsStringToStd((__bridge void*)key)] = nsStringToStd((__bridge void*)value);
        }
    }
    return result;
}

void* FoundationBridge::stdMapToNSDictionary(const std::unordered_map<std::string, std::string>& map) {
    NSMutableDictionary* dict = [[NSMutableDictionary alloc] initWithCapacity:map.size()];
    for (const auto& [key, value] : map) {
        NSString* nsKey = [NSString stringWithUTF8String:key.c_str()];
        NSString* nsValue = [NSString stringWithUTF8String:value.c_str()];
        dict[nsKey] = nsValue;
    }
    return (__bridge_retained void*)dict;
}

// NSTaskWrapper Implementation
class NSTaskWrapper::Impl {
public:
    NSTask* task_;
    NSPipe* stdoutPipe_;
    NSPipe* stderrPipe_;
    std::string executable_;
    std::vector<std::string> arguments_;
    std::filesystem::path workingDirectory_;
    std::unordered_map<std::string, std::string> environment_;
    std::chrono::milliseconds timeout_;
    
    Impl() : task_(nil), stdoutPipe_(nil), stderrPipe_(nil), timeout_(std::chrono::minutes(5)) {
        reset();
    }
    
    ~Impl() {
        if (task_ && [task_ isRunning]) {
            [task_ terminate];
        }
    }
    
    void reset() {
        task_ = [[NSTask alloc] init];
        stdoutPipe_ = [[NSPipe alloc] init];
        stderrPipe_ = [[NSPipe alloc] init];
        [task_ setStandardOutput:stdoutPipe_];
        [task_ setStandardError:stderrPipe_];
    }
    
    ProcessResult executeSync() {
        ProcessResult result;
        auto start_time = std::chrono::steady_clock::now();
        
        // Configure task
        if (!executable_.empty()) {
            [task_ setExecutableURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:executable_.c_str()]]];
        }
        
        if (!arguments_.empty()) {
            NSMutableArray* nsArgs = [[NSMutableArray alloc] init];
            for (const auto& arg : arguments_) {
                [nsArgs addObject:[NSString stringWithUTF8String:arg.c_str()]];
            }
            [task_ setArguments:nsArgs];
        }
        
        if (!workingDirectory_.empty()) {
            [task_ setCurrentDirectoryURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:workingDirectory_.c_str()]]];
        }
        
        if (!environment_.empty()) {
            NSMutableDictionary* nsEnv = [[NSMutableDictionary alloc] init];
            for (const auto& [key, value] : environment_) {
                nsEnv[[NSString stringWithUTF8String:key.c_str()]] = [NSString stringWithUTF8String:value.c_str()];
            }
            [task_ setEnvironment:nsEnv];
        }
        
        // Launch task
        NSError* error = nil;
        BOOL launched = [task_ launchAndReturnError:&error];
        if (!launched) {
            result.success = false;
            if (error) {
                result.stderr_output = FoundationBridge::nsStringToStd((__bridge void*)[error localizedDescription]);
            }
            return result;
        }
        
        // Wait with timeout
        dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
        __block BOOL finished = NO;
        
        [task_ setTerminationHandler:^(NSTask* task) {
            finished = YES;
            dispatch_semaphore_signal(semaphore);
        }];
        
        auto timeout_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(timeout_).count();
        long wait_result = dispatch_semaphore_wait(semaphore, dispatch_time(DISPATCH_TIME_NOW, timeout_ns));
        
        if (wait_result != 0) {
            // Timeout occurred
            [task_ terminate];
            result.timed_out = true;
            result.success = false;
        } else {
            result.exit_code = [task_ terminationStatus];
            result.success = (result.exit_code == 0);
        }
        
        // Read output
        NSData* stdoutData = [[stdoutPipe_ fileHandleForReading] readDataToEndOfFile];
        NSData* stderrData = [[stderrPipe_ fileHandleForReading] readDataToEndOfFile];
        
        if (stdoutData) {
            NSString* stdoutStr = [[NSString alloc] initWithData:stdoutData encoding:NSUTF8StringEncoding];
            if (stdoutStr) {
                result.stdout_output = FoundationBridge::nsStringToStd((__bridge void*)stdoutStr);
            }
        }
        
        if (stderrData) {
            NSString* stderrStr = [[NSString alloc] initWithData:stderrData encoding:NSUTF8StringEncoding];
            if (stderrStr) {
                result.stderr_output = FoundationBridge::nsStringToStd((__bridge void*)stderrStr);
            }
        }
        
        auto end_time = std::chrono::steady_clock::now();
        result.execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        return result;
    }
};

NSTaskWrapper::NSTaskWrapper() : pImpl_(std::make_unique<Impl>()) {}
NSTaskWrapper::~NSTaskWrapper() = default;

NSTaskWrapper::NSTaskWrapper(NSTaskWrapper&& other) noexcept : pImpl_(std::move(other.pImpl_)) {}
NSTaskWrapper& NSTaskWrapper::operator=(NSTaskWrapper&& other) noexcept {
    pImpl_ = std::move(other.pImpl_);
    return *this;
}

void NSTaskWrapper::setExecutable(const std::string& path) {
    pImpl_->executable_ = path;
}

void NSTaskWrapper::setArguments(const std::vector<std::string>& args) {
    pImpl_->arguments_ = args;
}

void NSTaskWrapper::setWorkingDirectory(const std::filesystem::path& path) {
    pImpl_->workingDirectory_ = path;
}

void NSTaskWrapper::setEnvironment(const std::unordered_map<std::string, std::string>& env) {
    pImpl_->environment_ = env;
}

void NSTaskWrapper::setTimeout(std::chrono::milliseconds timeout) {
    pImpl_->timeout_ = timeout;
}

ProcessResult NSTaskWrapper::execute() {
    return pImpl_->executeSync();
}

std::future<ProcessResult> NSTaskWrapper::executeAsync() {
    return std::async(std::launch::async, [this]() {
        return pImpl_->executeSync();
    });
}

bool NSTaskWrapper::isRunning() const {
    return pImpl_->task_ && [pImpl_->task_ isRunning];
}

bool NSTaskWrapper::terminate() {
    if (pImpl_->task_ && [pImpl_->task_ isRunning]) {
        [pImpl_->task_ terminate];
        return true;
    }
    return false;
}

bool NSTaskWrapper::kill() {
    if (pImpl_->task_ && [pImpl_->task_ isRunning]) {
        [pImpl_->task_ terminate]; // NSTask doesn't have separate kill method
        return true;
    }
    return false;
}

int NSTaskWrapper::getProcessId() const {
    if (pImpl_->task_ && [pImpl_->task_ isRunning]) {
        return [pImpl_->task_ processIdentifier];
    }
    return -1;
}

// NSFileManagerWrapper Implementation
class NSFileManagerWrapper::Impl {
public:
    NSFileManager* fileManager_;
    
    Impl() {
        fileManager_ = [NSFileManager defaultManager];
    }
};

NSFileManagerWrapper::NSFileManagerWrapper() : pImpl_(std::make_unique<Impl>()) {}
NSFileManagerWrapper::~NSFileManagerWrapper() = default;

bool NSFileManagerWrapper::exists(const std::filesystem::path& path) {
    NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
    return [pImpl_->fileManager_ fileExistsAtPath:nsPath];
}

bool NSFileManagerWrapper::isDirectory(const std::filesystem::path& path) {
    NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
    BOOL isDir = NO;
    BOOL exists = [pImpl_->fileManager_ fileExistsAtPath:nsPath isDirectory:&isDir];
    return exists && isDir;
}

Result<bool> NSFileManagerWrapper::createDirectory(const std::filesystem::path& path, bool createIntermediates) {
    NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
    NSError* error = nil;
    
    BOOL success = [pImpl_->fileManager_ createDirectoryAtPath:nsPath 
                                   withIntermediateDirectories:createIntermediates 
                                                    attributes:nil 
                                                         error:&error];
    
    if (success) {
        return true;
    } else {
        std::string errorMsg = error ? FoundationBridge::nsStringToStd((__bridge void*)[error localizedDescription]) : "Unknown error";
        return FrameworkError("Failed to create directory: " + errorMsg, error ? (int)[error code] : -1, "Foundation");
    }
}

Result<bool> NSFileManagerWrapper::removeItem(const std::filesystem::path& path) {
    NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
    NSError* error = nil;
    
    BOOL success = [pImpl_->fileManager_ removeItemAtPath:nsPath error:&error];
    
    if (success) {
        return true;
    } else {
        std::string errorMsg = error ? FoundationBridge::nsStringToStd((__bridge void*)[error localizedDescription]) : "Unknown error";
        return FrameworkError("Failed to remove item: " + errorMsg, error ? (int)[error code] : -1, "Foundation");
    }
}

Result<bool> NSFileManagerWrapper::copyItem(const std::filesystem::path& source, const std::filesystem::path& destination) {
    NSString* nsSource = [NSString stringWithUTF8String:source.c_str()];
    NSString* nsDest = [NSString stringWithUTF8String:destination.c_str()];
    NSError* error = nil;
    
    BOOL success = [pImpl_->fileManager_ copyItemAtPath:nsSource toPath:nsDest error:&error];
    
    if (success) {
        return true;
    } else {
        std::string errorMsg = error ? FoundationBridge::nsStringToStd((__bridge void*)[error localizedDescription]) : "Unknown error";
        return FrameworkError("Failed to copy item: " + errorMsg, error ? (int)[error code] : -1, "Foundation");
    }
}

Result<bool> NSFileManagerWrapper::moveItem(const std::filesystem::path& source, const std::filesystem::path& destination) {
    NSString* nsSource = [NSString stringWithUTF8String:source.c_str()];
    NSString* nsDest = [NSString stringWithUTF8String:destination.c_str()];
    NSError* error = nil;
    
    BOOL success = [pImpl_->fileManager_ moveItemAtPath:nsSource toPath:nsDest error:&error];
    
    if (success) {
        return true;
    } else {
        std::string errorMsg = error ? FoundationBridge::nsStringToStd((__bridge void*)[error localizedDescription]) : "Unknown error";
        return FrameworkError("Failed to move item: " + errorMsg, error ? (int)[error code] : -1, "Foundation");
    }
}

Result<std::vector<std::string>> NSFileManagerWrapper::listDirectory(const std::filesystem::path& path) {
    NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
    NSError* error = nil;
    
    NSArray* contents = [pImpl_->fileManager_ contentsOfDirectoryAtPath:nsPath error:&error];
    
    if (contents) {
        return FoundationBridge::nsArrayToStdVector((__bridge void*)contents);
    } else {
        std::string errorMsg = error ? FoundationBridge::nsStringToStd((__bridge void*)[error localizedDescription]) : "Unknown error";
        return FrameworkError("Failed to list directory: " + errorMsg, error ? (int)[error code] : -1, "Foundation");
    }
}

Result<uint64_t> NSFileManagerWrapper::getFileSize(const std::filesystem::path& path) {
    NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
    NSError* error = nil;
    
    NSDictionary* attributes = [pImpl_->fileManager_ attributesOfItemAtPath:nsPath error:&error];
    
    if (attributes) {
        NSNumber* fileSize = attributes[NSFileSize];
        return [fileSize unsignedLongLongValue];
    } else {
        std::string errorMsg = error ? FoundationBridge::nsStringToStd((__bridge void*)[error localizedDescription]) : "Unknown error";
        return FrameworkError("Failed to get file size: " + errorMsg, error ? (int)[error code] : -1, "Foundation");
    }
}

Result<std::chrono::system_clock::time_point> NSFileManagerWrapper::getModificationDate(const std::filesystem::path& path) {
    NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
    NSError* error = nil;
    
    NSDictionary* attributes = [pImpl_->fileManager_ attributesOfItemAtPath:nsPath error:&error];
    
    if (attributes) {
        NSDate* modDate = attributes[NSFileModificationDate];
        NSTimeInterval timestamp = [modDate timeIntervalSince1970];
        auto duration = std::chrono::duration<double>(timestamp);
        return std::chrono::system_clock::time_point(std::chrono::duration_cast<std::chrono::system_clock::duration>(duration));
    } else {
        std::string errorMsg = error ? FoundationBridge::nsStringToStd((__bridge void*)[error localizedDescription]) : "Unknown error";
        return FrameworkError("Failed to get modification date: " + errorMsg, error ? (int)[error code] : -1, "Foundation");
    }
}

// Placeholder implementations for remaining classes
Result<std::unordered_map<std::string, std::string>> NSFileManagerWrapper::getAttributes(const std::filesystem::path& path) {
    // TODO: Implement full attribute mapping
    return std::unordered_map<std::string, std::string>{};
}

Result<bool> NSFileManagerWrapper::setAttributes(const std::filesystem::path& path, const std::unordered_map<std::string, std::string>& attributes) {
    // TODO: Implement attribute setting
    return true;
}

std::unique_ptr<FSWatcher> NSFileManagerWrapper::watchDirectory(const std::filesystem::path& path, FSWatcherCallback callback) {
    // TODO: Implement file system watching using dispatch sources or FSEvents
    return nullptr;
}

// FoundationBridge Implementation
class FoundationBridge::Impl {
public:
    std::unique_ptr<NSFileManagerWrapper> fileManager_;
    std::unique_ptr<NSWorkspaceWrapper> workspace_;
    
    Impl() {
        fileManager_ = std::make_unique<NSFileManagerWrapper>();
        workspace_ = std::make_unique<NSWorkspaceWrapper>();
    }
};

FoundationBridge::FoundationBridge() : pImpl_(std::make_unique<Impl>()) {}
FoundationBridge::~FoundationBridge() = default;

std::unique_ptr<NSTaskWrapper> FoundationBridge::createTask() {
    return std::make_unique<NSTaskWrapper>();
}

NSFileManagerWrapper& FoundationBridge::getFileManager() {
    return *pImpl_->fileManager_;
}

NSWorkspaceWrapper& FoundationBridge::getWorkspace() {
    return *pImpl_->workspace_;
}

std::unique_ptr<NSTimerWrapper> FoundationBridge::createTimer() {
    return std::make_unique<NSTimerWrapper>();
}

ProcessResult FoundationBridge::executeCommand(const std::string& command, const std::vector<std::string>& args) {
    auto task = createTask();
    task->setExecutable(command);
    task->setArguments(args);
    return task->execute();
}

std::future<ProcessResult> FoundationBridge::executeCommandAsync(const std::string& command, const std::vector<std::string>& args) {
    auto task = createTask();
    task->setExecutable(command);
    task->setArguments(args);
    return task->executeAsync();
}

ProcessResult FoundationBridge::executeCommandWithEnvironment(
    const std::string& command, 
    const std::vector<std::string>& args,
    const std::unordered_map<std::string, std::string>& env) {
    
    auto task = createTask();
    task->setExecutable(command);
    task->setArguments(args);
    task->setEnvironment(env);
    return task->execute();
}

ProcessResult FoundationBridge::executeCommandWithTimeout(
    const std::string& command, 
    const std::vector<std::string>& args,
    std::chrono::milliseconds timeout) {
    
    auto task = createTask();
    task->setExecutable(command);
    task->setArguments(args);
    task->setTimeout(timeout);
    return task->execute();
}

// Placeholder implementations for NSTimerWrapper and NSWorkspaceWrapper
class NSTimerWrapper::Impl {
public:
    NSTimer* timer_;
    
    Impl() : timer_(nil) {}
    
    ~Impl() {
        if (timer_ && [timer_ isValid]) {
            [timer_ invalidate];
        }
    }
};

NSTimerWrapper::NSTimerWrapper() : pImpl_(std::make_unique<Impl>()) {}
NSTimerWrapper::~NSTimerWrapper() = default;

NSTimerWrapper::NSTimerWrapper(NSTimerWrapper&& other) noexcept : pImpl_(std::move(other.pImpl_)) {}
NSTimerWrapper& NSTimerWrapper::operator=(NSTimerWrapper&& other) noexcept {
    pImpl_ = std::move(other.pImpl_);
    return *this;
}

void NSTimerWrapper::scheduleOnce(std::chrono::milliseconds delay, std::function<void()> callback) {
    // TODO: Implement timer scheduling
}

void NSTimerWrapper::scheduleRepeating(std::chrono::milliseconds interval, std::function<void()> callback) {
    // TODO: Implement repeating timer
}

void NSTimerWrapper::stop() {
    if (pImpl_->timer_ && [pImpl_->timer_ isValid]) {
        [pImpl_->timer_ invalidate];
        pImpl_->timer_ = nil;
    }
}

bool NSTimerWrapper::isValid() const {
    return pImpl_->timer_ && [pImpl_->timer_ isValid];
}

class NSWorkspaceWrapper::Impl {
public:
    NSWorkspace* workspace_;
    
    Impl() {
        workspace_ = [NSWorkspace sharedWorkspace];
    }
};

NSWorkspaceWrapper::NSWorkspaceWrapper() : pImpl_(std::make_unique<Impl>()) {}
NSWorkspaceWrapper::~NSWorkspaceWrapper() = default;

WorkspaceInfo NSWorkspaceWrapper::getWorkspaceInfo() {
    WorkspaceInfo info;
    // TODO: Implement workspace info gathering
    return info;
}

std::vector<std::string> NSWorkspaceWrapper::getRunningApplications() {
    // TODO: Implement running applications enumeration
    return {};
}

std::string NSWorkspaceWrapper::getActiveApplication() {
    // TODO: Implement active application detection
    return "";
}

Result<bool> NSWorkspaceWrapper::launchApplication(const std::string& bundleIdentifier) {
    NSString* identifier = [NSString stringWithUTF8String:bundleIdentifier.c_str()];
    BOOL success = [pImpl_->workspace_ launchApplication:identifier];
    return success;
}

Result<bool> NSWorkspaceWrapper::launchApplicationAtPath(const std::filesystem::path& path) {
    NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
    BOOL success = [pImpl_->workspace_ launchApplication:nsPath];
    return success;
}

Result<bool> NSWorkspaceWrapper::terminateApplication(const std::string& bundleIdentifier) {
    // TODO: Implement application termination
    return false;
}

bool NSWorkspaceWrapper::isApplicationRunning(const std::string& bundleIdentifier) {
    // TODO: Implement application running check
    return false;
}

Result<bool> NSWorkspaceWrapper::openURL(const std::string& url) {
    NSURL* nsUrl = [NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]];
    BOOL success = [pImpl_->workspace_ openURL:nsUrl];
    return success;
}

Result<bool> NSWorkspaceWrapper::openFile(const std::filesystem::path& path) {
    NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
    BOOL success = [pImpl_->workspace_ openFile:nsPath];
    return success;
}

void NSWorkspaceWrapper::registerForApplicationLaunchNotifications(std::function<void(const std::string&)> callback) {
    // TODO: Implement notification registration
}

void NSWorkspaceWrapper::registerForApplicationTerminateNotifications(std::function<void(const std::string&)> callback) {
    // TODO: Implement notification registration  
}

} // namespace flight::hal::macos
