/**
 * @file mock_file_driver.hpp
 * @brief Mock File Driver for Testing and Development
 * 
 * Provides a complete in-memory file system implementation for testing,
 * prototyping, and development purposes. Simulates various filesystem
 * behaviors and capabilities.
 */

#pragma once

#include "../../include/flight/hal/interfaces/file.hpp"
#include "../../include/flight/hal/core/driver_registry.hpp"
#include <unordered_map>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <sstream>
#include <chrono>

namespace flight::hal::examples {

/**
 * @brief Mock file entry for in-memory filesystem
 */
struct MockFileEntry {
    std::string name;
    std::string full_path;
    FileType type;
    std::vector<uint8_t> data;
    FilePermission permissions;
    std::chrono::system_clock::time_point created_time;
    std::chrono::system_clock::time_point modified_time;
    std::chrono::system_clock::time_point accessed_time;
    bool is_hidden;
    bool is_system;
    std::unordered_map<std::string, std::unique_ptr<MockFileEntry>> children; // For directories
    
    MockFileEntry(const std::string& name, FileType type = FileType::Regular)
        : name(name), type(type), permissions(FilePermission::ReadWrite),
          is_hidden(false), is_system(false) {
        auto now = std::chrono::system_clock::now();
        created_time = modified_time = accessed_time = now;
    }
};

/**
 * @brief Mock file handle for tracking open files
 */
struct MockFileHandle {
    uint32_t id;
    std::shared_ptr<MockFileEntry> entry;
    FileAccessMode access_mode;
    FileOpenParams params;
    size_t position;
    bool is_open;
    
    MockFileHandle(uint32_t id, std::shared_ptr<MockFileEntry> entry, FileAccessMode mode, const FileOpenParams& params)
        : id(id), entry(entry), access_mode(mode), params(params), position(0), is_open(true) {}
};

/**
 * @brief Async operation structure
 */
struct AsyncOperation {
    uint32_t id;
    std::function<void()> operation;
    bool completed;
    std::chrono::steady_clock::time_point start_time;
    
    AsyncOperation(uint32_t id, std::function<void()> op)
        : id(id), operation(std::move(op)), completed(false), start_time(std::chrono::steady_clock::now()) {}
};

/**
 * @brief Mock file interface implementation (simplified for demonstration)
 */
class MockFileInterface : public IFileInterface {
private:
    mutable std::mutex mutex_;
    std::shared_ptr<MockFileEntry> root_;
    std::unordered_map<uint32_t, std::unique_ptr<MockFileHandle>> open_files_;
    std::atomic<uint32_t> next_file_id_{1};
    std::string current_directory_;
    
    // Statistics
    FileIOStats global_stats_{};
    
public:
    MockFileInterface() : current_directory_("/") {
        // Initialize root directory
        root_ = std::make_shared<MockFileEntry>("", FileType::Directory);
        root_->full_path = "/";
        
        // Create some mock files and directories for testing
        create_mock_filesystem();
    }
    
    ~MockFileInterface() = default;
    
private:
    void create_mock_filesystem() {
        // Create basic directory structure for testing
        auto assets = std::make_unique<MockFileEntry>("assets", FileType::Directory);
        assets->full_path = "/assets";
        root_->children["assets"] = std::move(assets);
        
        // Create a test file
        auto readme = std::make_unique<MockFileEntry>("readme.txt", FileType::Regular);
        readme->full_path = "/readme.txt";
        std::string content = "Hello World!";
        readme->data.assign(content.begin(), content.end());
        root_->children["readme.txt"] = std::move(readme);
    }
    
    std::shared_ptr<MockFileEntry> find_entry(const std::string& path) {
        if (path.empty() || path == "/") {
            return root_;
        }
        
        std::string normalized_path = file::normalize_path(path);
        if (normalized_path.front() == '/') {
            normalized_path = normalized_path.substr(1);
        }
        
        std::shared_ptr<MockFileEntry> current = root_;
        std::stringstream ss(normalized_path);
        std::string component;
        
        while (std::getline(ss, component, '/')) {
            if (component.empty()) continue;
            
            if (current->type != FileType::Directory) {
                return nullptr;
            }
            
            auto it = current->children.find(component);
            if (it == current->children.end()) {
                return nullptr;
            }
            
            current = std::shared_ptr<MockFileEntry>(it->second.get(), [](MockFileEntry*) {});
        }
        
        return current;
    }
    
    std::shared_ptr<MockFileEntry> create_entry(const std::string& path, FileType type) {
        std::string directory = file::get_directory(path);
        std::string filename = file::get_filename(path);
        
        auto parent = find_entry(directory);
        if (!parent || parent->type != FileType::Directory) {
            return nullptr;
        }
        
        auto entry = std::make_unique<MockFileEntry>(filename, type);
        entry->full_path = file::normalize_path(path);
        
        auto shared_entry = std::shared_ptr<MockFileEntry>(entry.get(), [](MockFileEntry*) {});
        parent->children[filename] = std::move(entry);
        
        return shared_entry;
    }
    
public:
    // === File System Information ===
    
    HALResult<FileSystemStats> get_filesystem_stats(const std::string& path) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        FileSystemStats stats = {};
        stats.total_space = 1024 * 1024 * 1024; // 1GB virtual space
        stats.free_space = 512 * 1024 * 1024;   // 512MB free
        stats.available_space = stats.free_space;
        stats.total_files = 1000;
        stats.free_files = 500;
        stats.block_size = 4096;
        stats.max_filename_length = 255;
        stats.max_path_length = 4096;
        stats.type = FileSystemType::Memory;
        stats.capabilities = static_cast<uint32_t>(FileSystemCapability::SynchronousIO) |
                           static_cast<uint32_t>(FileSystemCapability::AsynchronousIO) |
                           static_cast<uint32_t>(FileSystemCapability::MemoryMapping) |
                           static_cast<uint32_t>(FileSystemCapability::DirectoryEnum) |
                           static_cast<uint32_t>(FileSystemCapability::Seeking) |
                           static_cast<uint32_t>(FileSystemCapability::Truncation);
        stats.is_case_sensitive = true;
        stats.is_read_only = false;
        stats.supports_unicode = true;
        
        return HALResult<FileSystemStats>::success(std::move(stats));
    }
    
    HALResult<std::string> get_current_directory() override {
        std::lock_guard<std::mutex> lock(mutex_);
        return HALResult<std::string>::success(current_directory_);
    }
    
    HALResult<void> set_current_directory(const std::string& path) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto entry = find_entry(path);
        if (!entry || entry->type != FileType::Directory) {
            return HALResult<void>::error(errors::invalid_parameter(1, "Path is not a directory"));
        }
        current_directory_ = file::normalize_path(path);
        return HALResult<void>::success();
    }
    
    bool exists(const std::string& path) override {
        std::lock_guard<std::mutex> lock(mutex_);
        return find_entry(path) != nullptr;
    }
    
    HALResult<FileInfo> get_file_info(const std::string& path) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto entry = find_entry(path);
        if (!entry) {
            return HALResult<FileInfo>::error(errors::device_not_found(1, "File not found"));
        }
        
        FileInfo info = {};
        info.name = entry->name;
        info.full_path = entry->full_path;
        info.type = entry->type;
        info.size = entry->data.size();
        info.permissions = entry->permissions;
        info.created_time = entry->created_time;
        info.modified_time = entry->modified_time;
        info.accessed_time = entry->accessed_time;
        info.is_hidden = entry->is_hidden;
        info.is_system = entry->is_system;
        info.is_archive = false;
        info.is_compressed = false;
        info.attributes = 0;
        
        return HALResult<FileInfo>::success(std::move(info));
    }
    
    // === Synchronous File Operations ===
    
    HALResult<FileHandle> open_file(const std::string& path, const FileOpenParams& params) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto entry = find_entry(path);
        
        // Handle creation modes
        if (!entry) {
            if (params.access_mode == FileAccessMode::Create ||
                params.access_mode == FileAccessMode::CreateOrTruncate ||
                params.access_mode == FileAccessMode::CreateOrOpen) {
                entry = create_entry(path, FileType::Regular);
                if (!entry) {
                    return HALResult<FileHandle>::error(errors::invalid_parameter(2, "Invalid path"));
                }
            } else {
                return HALResult<FileHandle>::error(errors::device_not_found(2, "File not found"));
            }
        } else if (params.access_mode == FileAccessMode::Create) {
            return HALResult<FileHandle>::error(errors::internal_error(1, "File already exists"));
        }
        
        // Create file handle
        uint32_t file_id = next_file_id_++;
        auto handle = std::make_unique<MockFileHandle>(file_id, entry, params.access_mode, params);
        
        FileHandle result;
        result.id = file_id;
        result.type = entry->type;
        result.generation = 1;
        
        open_files_[file_id] = std::move(handle);
        
        // Update access time
        entry->accessed_time = std::chrono::system_clock::now();
        
        return HALResult<FileHandle>::success(std::move(result));
    }
    
    HALResult<FileHandle> open_file(const std::string& path, FileAccessMode access_mode) override {
        FileOpenParams params = file::make_file_open_params(access_mode);
        return open_file(path, params);
    }
    
    HALResult<void> close_file(FileHandle file_handle) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = open_files_.find(file_handle.id);
        if (it == open_files_.end()) {
            return HALResult<void>::error(errors::invalid_parameter(3, "Invalid file handle"));
        }
        
        it->second->is_open = false;
        open_files_.erase(it);
        
        return HALResult<void>::success();
    }
    
    HALResult<size_t> read_file(FileHandle file_handle, void* buffer, size_t size) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = open_files_.find(file_handle.id);
        if (it == open_files_.end() || !it->second->is_open) {
            return HALResult<size_t>::error(errors::invalid_parameter(4, "Invalid file handle"));
        }
        
        auto& handle = it->second;
        auto& entry = handle->entry;
        
        if (handle->access_mode == FileAccessMode::WriteOnly) {
            return HALResult<size_t>::error(errors::invalid_state(1, "File not open for reading"));
        }
        
        size_t bytes_to_read = std::min(size, entry->data.size() - handle->position);
        if (bytes_to_read > 0) {
            std::memcpy(buffer, entry->data.data() + handle->position, bytes_to_read);
            handle->position += bytes_to_read;
        }
        
        // Update statistics
        global_stats_.bytes_read += bytes_to_read;
        global_stats_.read_operations++;
        
        // Update access time
        entry->accessed_time = std::chrono::system_clock::now();
        
        return HALResult<size_t>::success(bytes_to_read);
    }
    
    HALResult<size_t> write_file(FileHandle file_handle, const void* data, size_t size) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = open_files_.find(file_handle.id);
        if (it == open_files_.end() || !it->second->is_open) {
            return HALResult<size_t>::error(errors::invalid_parameter(5, "Invalid file handle"));
        }
        
        auto& handle = it->second;
        auto& entry = handle->entry;
        
        if (handle->access_mode == FileAccessMode::ReadOnly) {
            return HALResult<size_t>::error(errors::invalid_state(2, "File not open for writing"));
        }
        
        // Resize buffer if needed
        if (handle->position + size > entry->data.size()) {
            entry->data.resize(handle->position + size);
        }
        
        std::memcpy(entry->data.data() + handle->position, data, size);
        handle->position += size;
        
        // Update statistics
        global_stats_.bytes_written += size;
        global_stats_.write_operations++;
        
        // Update modification time
        entry->modified_time = std::chrono::system_clock::now();
        
        return HALResult<size_t>::success(size);
    }
    
    HALResult<uint64_t> seek_file(FileHandle file_handle, int64_t offset, SeekOrigin origin) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = open_files_.find(file_handle.id);
        if (it == open_files_.end() || !it->second->is_open) {
            return HALResult<uint64_t>::error(errors::invalid_parameter(6, "Invalid file handle"));
        }
        
        auto& handle = it->second;
        auto& entry = handle->entry;
        
        size_t new_position;
        switch (origin) {
            case SeekOrigin::Begin:
                new_position = static_cast<size_t>(std::max(int64_t(0), offset));
                break;
            case SeekOrigin::Current:
                new_position = static_cast<size_t>(std::max(int64_t(0), static_cast<int64_t>(handle->position) + offset));
                break;
            case SeekOrigin::End:
                new_position = static_cast<size_t>(std::max(int64_t(0), static_cast<int64_t>(entry->data.size()) + offset));
                break;
            default:
                return HALResult<uint64_t>::error(errors::invalid_parameter(7, "Invalid seek origin"));
        }
        
        handle->position = new_position;
        global_stats_.seek_operations++;
        
        return HALResult<uint64_t>::success(new_position);
    }
    
    HALResult<uint64_t> tell_file(FileHandle file_handle) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = open_files_.find(file_handle.id);
        if (it == open_files_.end() || !it->second->is_open) {
            return HALResult<uint64_t>::error(errors::invalid_parameter(8, "Invalid file handle"));
        }
        
        return HALResult<uint64_t>::success(it->second->position);
    }
    
    HALResult<uint64_t> get_file_size(FileHandle file_handle) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = open_files_.find(file_handle.id);
        if (it == open_files_.end() || !it->second->is_open) {
            return HALResult<uint64_t>::error(errors::invalid_parameter(9, "Invalid file handle"));
        }
        
        return HALResult<uint64_t>::success(it->second->entry->data.size());
    }
    
    HALResult<void> flush_file(FileHandle file_handle) override {
        // Mock implementation - always succeeds
        return HALResult<void>::success();
    }
    
    HALResult<void> truncate_file(FileHandle file_handle, uint64_t size) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = open_files_.find(file_handle.id);
        if (it == open_files_.end() || !it->second->is_open) {
            return HALResult<void>::error(errors::invalid_parameter(10, "Invalid file handle"));
        }
        
        auto& handle = it->second;
        if (handle->access_mode == FileAccessMode::ReadOnly) {
            return HALResult<void>::error(errors::invalid_state(3, "File not open for writing"));
        }
        
        handle->entry->data.resize(size);
        if (handle->position > size) {
            handle->position = size;
        }
        
        handle->entry->modified_time = std::chrono::system_clock::now();
        
        return HALResult<void>::success();
    }
    
    // === Remaining interface methods (simplified implementations) ===
    
    // For brevity, most async and advanced methods return not_implemented
    HALResult<uint32_t> open_file_async(const std::string& path, const FileOpenParams& params, FileOpenCallback callback) override {
        return HALResult<uint32_t>::error(errors::not_implemented(1, "Async operations not implemented in mock"));
    }
    
    HALResult<uint32_t> read_file_async(FileHandle file_handle, void* buffer, size_t size, FileIOCallback callback) override {
        return HALResult<uint32_t>::error(errors::not_implemented(2, "Async operations not implemented in mock"));
    }
    
    HALResult<uint32_t> write_file_async(FileHandle file_handle, const void* data, size_t size, FileIOCallback callback) override {
        return HALResult<uint32_t>::error(errors::not_implemented(3, "Async operations not implemented in mock"));
    }
    
    HALResult<void> cancel_async_operation(uint32_t operation_id) override {
        return HALResult<void>::error(errors::not_implemented(4, "Async operations not implemented in mock"));
    }
    
    HALResult<void> wait_for_async_operation(uint32_t operation_id, uint32_t timeout_ms) override {
        return HALResult<void>::error(errors::not_implemented(5, "Async operations not implemented in mock"));
    }
    
    HALResult<void> create_directory(const std::string& path, bool recursive) override {
        return HALResult<void>::error(errors::not_implemented(6, "Directory operations not implemented in mock"));
    }
    
    HALResult<void> remove_directory(const std::string& path, bool recursive) override {
        return HALResult<void>::error(errors::not_implemented(7, "Directory operations not implemented in mock"));
    }
    
    HALResult<std::vector<FileInfo>> enumerate_directory(const std::string& path) override {
        return HALResult<std::vector<FileInfo>>::error(errors::not_implemented(8, "Directory enumeration not implemented in mock"));
    }
    
    HALResult<uint32_t> enumerate_directory_async(const std::string& path, DirectoryEnumCallback callback) override {
        return HALResult<uint32_t>::error(errors::not_implemented(9, "Async directory enumeration not implemented in mock"));
    }
    
    HALResult<void> copy_file(const std::string& source_path, const std::string& destination_path, bool overwrite_existing) override {
        return HALResult<void>::error(errors::not_implemented(10, "File copy not implemented in mock"));
    }
    
    HALResult<void> move_file(const std::string& source_path, const std::string& destination_path, bool overwrite_existing) override {
        return HALResult<void>::error(errors::not_implemented(11, "File move not implemented in mock"));
    }
    
    HALResult<void> delete_file(const std::string& path) override {
        return HALResult<void>::error(errors::not_implemented(12, "File delete not implemented in mock"));
    }
    
    HALResult<void> set_file_permissions(const std::string& path, FilePermission permissions) override {
        return HALResult<void>::error(errors::not_implemented(13, "Set permissions not implemented in mock"));
    }
    
    HALResult<void> set_file_times(const std::string& path, const std::chrono::system_clock::time_point& access_time, const std::chrono::system_clock::time_point& modification_time) override {
        return HALResult<void>::error(errors::not_implemented(14, "Set file times not implemented in mock"));
    }
    
    // Memory mapping methods
    HALResult<std::unique_ptr<IMemoryMappedFileView>> create_memory_mapped_view(FileHandle file_handle, size_t offset, size_t size, bool writable) override {
        return HALResult<std::unique_ptr<IMemoryMappedFileView>>::error(errors::not_implemented(15, "Memory mapping not implemented in mock"));
    }
    
    HALResult<MemoryMappedFile> create_memory_mapped_file(const std::string& path, FileAccessMode access_mode, size_t offset, size_t size) override {
        return HALResult<MemoryMappedFile>::error(errors::not_implemented(16, "Memory mapping not implemented in mock"));
    }
    
    HALResult<void> close_memory_mapped_file(const MemoryMappedFile& mapped_file) override {
        return HALResult<void>::error(errors::not_implemented(17, "Memory mapping not implemented in mock"));
    }
    
    // Archive operations
    HALResult<void> register_archive_provider(std::unique_ptr<IArchiveProvider> provider) override {
        return HALResult<void>::error(errors::not_implemented(18, "Archive operations not implemented in mock"));
    }
    
    HALResult<void> unregister_archive_provider(ArchiveFormat format) override {
        return HALResult<void>::error(errors::not_implemented(19, "Archive operations not implemented in mock"));
    }
    
    IArchiveProvider* get_archive_provider(ArchiveFormat format) override {
        return nullptr;
    }
    
    HALResult<FileHandle> open_archive(const std::string& path) override {
        return HALResult<FileHandle>::error(errors::not_implemented(20, "Archive operations not implemented in mock"));
    }
    
    HALResult<void> extract_archive_file(FileHandle archive_handle, const std::string& entry_path, const std::string& output_path) override {
        return HALResult<void>::error(errors::not_implemented(21, "Archive operations not implemented in mock"));
    }
    
    HALResult<MemoryAllocation> extract_archive_file_to_memory(FileHandle archive_handle, const std::string& entry_path, IMemoryAllocator* allocator) override {
        return HALResult<MemoryAllocation>::error(errors::not_implemented(22, "Archive operations not implemented in mock"));
    }
    
    HALResult<void> mount_archive(FileHandle archive_handle, const std::string& mount_point) override {
        return HALResult<void>::error(errors::not_implemented(23, "Archive mounting not implemented in mock"));
    }
    
    HALResult<void> unmount_archive(const std::string& mount_point) override {
        return HALResult<void>::error(errors::not_implemented(24, "Archive mounting not implemented in mock"));
    }
    
    // File watching
    IFileWatcher* get_file_watcher() override {
        return nullptr;
    }
    
    HALResult<uint32_t> watch_path(const std::string& path, uint32_t event_mask, bool recursive, FileWatchCallback callback) override {
        return HALResult<uint32_t>::error(errors::not_implemented(25, "File watching not implemented in mock"));
    }
    
    HALResult<void> unwatch_path(uint32_t watch_id) override {
        return HALResult<void>::error(errors::not_implemented(26, "File watching not implemented in mock"));
    }
    
    // Streaming operations
    HALResult<FileHandle> create_file_stream(FileHandle file_handle, size_t buffer_size) override {
        return HALResult<FileHandle>::error(errors::not_implemented(27, "Streaming not implemented in mock"));
    }
    
    HALResult<size_t> read_stream(FileHandle stream_handle, void* buffer, size_t size) override {
        return HALResult<size_t>::error(errors::not_implemented(28, "Streaming not implemented in mock"));
    }
    
    HALResult<void> close_stream(FileHandle stream_handle) override {
        return HALResult<void>::error(errors::not_implemented(29, "Streaming not implemented in mock"));
    }
    
    // Bulk operations
    HALResult<MemoryAllocation> read_entire_file(const std::string& path, IMemoryAllocator* allocator) override {
        return HALResult<MemoryAllocation>::error(errors::not_implemented(30, "Bulk operations not implemented in mock"));
    }
    
    HALResult<void> write_entire_file(const std::string& path, const void* data, size_t size, bool overwrite_existing) override {
        return HALResult<void>::error(errors::not_implemented(31, "Bulk operations not implemented in mock"));
    }
    
    HALResult<void> copy_file_with_progress(const std::string& source_path, const std::string& destination_path, std::function<void(uint64_t, uint64_t)> progress_callback, bool overwrite_existing) override {
        return HALResult<void>::error(errors::not_implemented(32, "Progress copy not implemented in mock"));
    }
    
    // Capability queries
    bool supports_filesystem_capability(FileSystemCapability capability) const override {
        return capability == FileSystemCapability::SynchronousIO ||
               capability == FileSystemCapability::Seeking ||
               capability == FileSystemCapability::Truncation;
    }
    
    bool supports_access_mode(FileAccessMode access_mode) const override {
        return access_mode == FileAccessMode::ReadOnly ||
               access_mode == FileAccessMode::WriteOnly ||
               access_mode == FileAccessMode::ReadWrite;
    }
    
    bool supports_archive_format(ArchiveFormat format) const override {
        return false;
    }
    
    uint64_t get_max_file_size() const override {
        return 1024 * 1024 * 1024; // 1GB
    }
    
    uint32_t get_max_path_length() const override {
        return 4096;
    }
    
    std::vector<ArchiveFormat> get_supported_archive_formats() const override {
        return {};
    }
    
    // Statistics and performance
    HALResult<FileIOStats> get_io_stats(FileHandle file_handle) override {
        std::lock_guard<std::mutex> lock(mutex_);
        return HALResult<FileIOStats>::success(global_stats_);
    }
    
    HALResult<void> reset_io_stats(FileHandle file_handle) override {
        std::lock_guard<std::mutex> lock(mutex_);
        global_stats_ = {};
        return HALResult<void>::success();
    }
    
    HALResult<std::pair<uint64_t, uint64_t>> get_cache_stats() override {
        return HALResult<std::pair<uint64_t, uint64_t>>::success({0, 0});
    }
    
    HALResult<void> flush_all_caches() override {
        return HALResult<void>::success();
    }
    
    HALResult<void> set_cache_size_limit(size_t size_bytes) override {
        return HALResult<void>::success();
    }
    
    // Platform-specific extensions
    void* get_extension_interface(std::string_view extension_name) override {
        return nullptr;
    }
    
    uint32_t get_sector_size() const override {
        return 2048; // Mock sector size
    }
    
    void* get_umd_info() const override {
        return nullptr;
    }
    
    void* get_browser_storage_interface() override {
        return nullptr;
    }
    
    // ICapabilityProvider implementation
    bool supports_capability(HALCapability capability) const override {
        return capability == HALCapability::PersistentStorage ||
               capability == HALCapability::AsyncIO;
    }
    
    uint32_t get_capability_mask() const override {
        return static_cast<uint32_t>(HALCapability::PersistentStorage) |
               static_cast<uint32_t>(HALCapability::AsyncIO);
    }
    
    std::vector<HALCapability> get_capabilities() const override {
        return {HALCapability::PersistentStorage, HALCapability::AsyncIO};
    }
    
    PerformanceTier get_performance_tier() const override {
        return PerformanceTier::Limited;
    }
    
    const PlatformInfo& get_platform_info() const override {
        static PlatformInfo info = {
            "Mock File System",
            "Virtual",
            PerformanceTier::Limited,
            1024 * 1024 * 1024, // 1GB
            1,
            true,
            false
        };
        return info;
    }
    
    bool has_fallback(HALCapability capability) const override {
        return capability == HALCapability::AsyncIO; // Can fallback to sync for async
    }
};

} // namespace flight::hal::examples
