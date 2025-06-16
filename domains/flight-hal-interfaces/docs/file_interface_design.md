# Flight HAL File Interface Design

## Overview

The Flight HAL File Interface provides a comprehensive, cross-platform file system abstraction designed to handle the diverse storage requirements of game development across 25+ years of hardware evolution. From Dreamcast GD-ROM and PSP UMD to modern SSDs and browser storage, this interface enables graceful degradation and optimal performance across all target platforms.

## Key Features

### Cross-Platform Storage Support

- **Optical Media**: GD-ROM (Dreamcast), UMD (PSP), ISO9660 (CD/DVD)
- **Flash Storage**: Memory cards, SSDs, embedded flash
- **Network Storage**: Remote file systems, cloud storage
- **Browser Storage**: localStorage, IndexedDB, virtual file systems
- **Archive Formats**: ZIP, PAK, WAD, 7-Zip, custom game archives

### Advanced I/O Capabilities

- **Synchronous & Asynchronous I/O**: Complete async support with callbacks
- **Memory Mapping**: High-performance file access through memory mapping
- **Streaming**: Large file streaming with optimized buffering
- **File Watching**: Real-time file system change notifications
- **Bulk Operations**: Optimized operations for reading/writing entire files

### Performance Optimization

- **Access Pattern Hints**: Sequential, random, streaming, memory-mapped
- **Dynamic Buffer Sizing**: Automatic buffer optimization based on file size and access pattern
- **Caching System**: Intelligent caching with configurable limits
- **I/O Statistics**: Comprehensive performance monitoring and profiling

## Architecture

### Core Components

```cpp
namespace flight::hal {

// Main interface
class IFileInterface : public IHALInterface, public ICapabilityProvider;

// Supporting interfaces  
class IArchiveProvider;     // Plugin system for archive formats
class IFileWatcher;         // File system change notifications
class IMemoryMappedFileView; // Memory-mapped file access

// Key data structures
struct FileHandle;          // Lightweight file references
struct FileInfo;           // Complete file metadata
struct FileSystemStats;    // File system information
struct MemoryMappedFile;   // Memory mapping descriptor

}
```

### File Handle System

The interface uses lightweight file handles for efficient resource management:

```cpp
struct FileHandle {
    uint32_t id;                    // Unique file identifier
    FileType type;                  // File type classification
    uint32_t generation;            // Generation counter for validation
    
    bool is_valid() const;
    void invalidate();
};
```

### Capability Detection

Robust capability detection enables graceful degradation:

```cpp
enum class FileSystemCapability : uint32_t {
    SynchronousIO = 1 << 0,        // Basic file operations
    AsynchronousIO = 1 << 1,       // Async I/O support
    MemoryMapping = 1 << 2,        // Memory-mapped files
    DirectoryEnum = 1 << 3,        // Directory listing
    FileWatching = 1 << 4,         // Change notifications
    ArchiveSupport = 1 << 5,       // Archive handling
    StreamingIO = 1 << 6,          // Large file streaming
    // ... additional capabilities
};
```

## API Design

### Basic File Operations

```cpp
// Open file with simple access mode
HALResult<FileHandle> open_file(const std::string& path, 
                               FileAccessMode access_mode = FileAccessMode::ReadOnly);

// Open file with detailed parameters
HALResult<FileHandle> open_file(const std::string& path, 
                               const FileOpenParams& params);

// Read/write operations
HALResult<size_t> read_file(FileHandle file_handle, void* buffer, size_t size);
HALResult<size_t> write_file(FileHandle file_handle, const void* data, size_t size);

// File positioning
HALResult<uint64_t> seek_file(FileHandle file_handle, int64_t offset, SeekOrigin origin);
HALResult<uint64_t> tell_file(FileHandle file_handle);
```

### Asynchronous Operations

```cpp
// Async file operations with callbacks
HALResult<uint32_t> read_file_async(FileHandle file_handle, void* buffer, size_t size,
                                   FileIOCallback callback);

HALResult<uint32_t> write_file_async(FileHandle file_handle, const void* data, size_t size,
                                    FileIOCallback callback);

// Operation management
HALResult<void> cancel_async_operation(uint32_t operation_id);
HALResult<void> wait_for_async_operation(uint32_t operation_id, uint32_t timeout_ms = 0);
```

### Memory Mapping

```cpp
// Create memory-mapped view
HALResult<std::unique_ptr<IMemoryMappedFileView>> create_memory_mapped_view(
    FileHandle file_handle, size_t offset = 0, size_t size = 0, bool writable = false);

// Direct memory-mapped file creation
HALResult<MemoryMappedFile> create_memory_mapped_file(const std::string& path,
                                                     FileAccessMode access_mode = FileAccessMode::ReadOnly);
```

### Archive Support

The interface provides a plugin system for different archive formats:

```cpp
class IArchiveProvider {
public:
    virtual ArchiveFormat get_format() const = 0;
    virtual bool is_supported_archive(const std::string& file_path) const = 0;
    virtual HALResult<FileHandle> open_archive(const std::string& file_path) = 0;
    virtual HALResult<size_t> extract_file(FileHandle archive_handle,
                                          const std::string& entry_path,
                                          void* output_buffer, size_t buffer_size) = 0;
};

// Register archive providers
HALResult<void> register_archive_provider(std::unique_ptr<IArchiveProvider> provider);
```

## Platform-Specific Adaptations

### Dreamcast GD-ROM

- **Sector-based Access**: 2048-byte sectors optimized for sequential reading
- **Low-Level Extensions**: Direct sector access for performance-critical operations
- **Read-Only**: Immutable storage with robust error handling

```cpp
// Dreamcast-specific extensions
virtual uint32_t get_sector_size() const = 0;  // Returns 2048 for GD-ROM
```

### PSP UMD

- **UMD-Specific Metadata**: Access to UMD disc information
- **Power Management**: Optimized for battery life
- **Cache Management**: Intelligent caching for optical media access patterns

```cpp
// PSP-specific extensions  
virtual void* get_umd_info() const = 0;  // UMD disc information
```

### Web Browser

- **Storage Abstraction**: Unified interface over localStorage, IndexedDB, and virtual file systems
- **Sandbox Compliance**: Respects browser security restrictions
- **Async-First**: Optimized for browser's async nature

```cpp
// Browser-specific extensions
virtual void* get_browser_storage_interface() = 0;  // Browser storage APIs
```

## Utility Functions

### Path Manipulation

```cpp
namespace file {
    std::string get_file_extension(const std::string& path);
    std::string get_filename(const std::string& path, bool with_extension = true);
    std::string get_directory(const std::string& path);
    std::string join_paths(const std::vector<std::string>& paths);
    std::string normalize_path(const std::string& path);
    bool is_absolute_path(const std::string& path);
}
```

### Parameter Helpers

```cpp
// Create optimized file open parameters
FileOpenParams make_file_open_params(FileAccessMode access_mode, FileAccessPattern access_pattern);
FileOpenParams make_streaming_params(FileAccessMode access_mode, uint32_t buffer_size);
FileOpenParams make_memory_mapped_params(bool writable = false);

// Calculate optimal buffer sizes
uint32_t calculate_optimal_buffer_size(uint64_t file_size, FileAccessPattern access_pattern);
```

### Filename Validation

```cpp
// Cross-platform filename validation and sanitization
bool is_valid_filename(const std::string& filename, FileSystemType filesystem_type);
std::string sanitize_filename(const std::string& filename, FileSystemType filesystem_type);
```

## Error Handling

The interface uses the HAL's structured error system:

```cpp
// File operations return HALResult<T> for comprehensive error handling
auto result = file_interface->open_file("/path/to/file.txt");
if (!result.is_success()) {
    const auto& error = result.error();
    std::cout << "Error: " << error.message() << "\n";
    std::cout << "Category: " << category_to_string(error.category()) << "\n";
    if (error.context()) {
        std::cout << "Context: " << error.context() << "\n";
    }
}
```

## Performance Considerations

### Buffer Size Optimization

The interface automatically calculates optimal buffer sizes based on:

- File size
- Access pattern (sequential, random, streaming)
- Platform capabilities
- Available memory

### Access Pattern Hints

```cpp
enum class FileAccessPattern : uint8_t {
    Sequential = 0,     // Optimal for optical media
    Random,            // Good for SSDs
    Streaming,         // Large file streaming
    MemoryMapped,      // Memory-mapped access
    WriteOnce,         // Write once, read many
    Temporary          // Temporary file, optimize for speed
};
```

### Caching Strategy

- **Intelligent Caching**: Adapts to access patterns and available memory
- **Configurable Limits**: Allows fine-tuning for specific platforms
- **Statistics Tracking**: Monitors cache hit/miss ratios for optimization

## Usage Examples

### Basic File Operations

```cpp
// Open and read a file
auto file_interface = get_file_interface();
auto file_result = file_interface->open_file("/game/config.txt", FileAccessMode::ReadOnly);

if (file_result.is_success()) {
    FileHandle handle = file_result.value();
    
    std::vector<char> buffer(1024);
    auto read_result = file_interface->read_file(handle, buffer.data(), buffer.size());
    
    if (read_result.is_success()) {
        // Process file data
        process_config_data(buffer.data(), read_result.value());
    }
    
    file_interface->close_file(handle);
}
```

### Archive Processing

```cpp
// Extract files from game archive
auto archive_result = file_interface->open_archive("/game/assets.pak");
if (archive_result.is_success()) {
    FileHandle archive = archive_result.value();
    
    // Extract texture to memory
    auto texture_result = file_interface->extract_archive_file_to_memory(
        archive, "textures/player.png", custom_allocator);
    
    if (texture_result.is_success()) {
        auto allocation = texture_result.value();
        load_texture_from_memory(allocation.data, allocation.size);
        custom_allocator->deallocate(allocation);
    }
}
```

### Memory-Mapped File Access

```cpp
// Memory-map a large data file
auto mapped_result = file_interface->create_memory_mapped_file("/game/world.dat");
if (mapped_result.is_success()) {
    MemoryMappedFile mapped_file = mapped_result.value();
    
    // Direct memory access to file data
    WorldData* world_data = reinterpret_cast<WorldData*>(mapped_file.data);
    
    // Use world data directly from mapped memory
    render_world(world_data);
    
    file_interface->close_memory_mapped_file(mapped_file);
}
```

## Integration with HAL

### Driver Registration

```cpp
// Register file interface driver
auto driver_registry = get_driver_registry();
auto file_driver = std::make_unique<PlatformFileDriver>();
driver_registry->register_interface_driver<IFileInterface>(std::move(file_driver));
```

### Capability Queries

```cpp
// Query platform capabilities
auto file_interface = get_file_interface();

if (file_interface->supports_filesystem_capability(FileSystemCapability::MemoryMapping)) {
    // Use memory mapping for large files
    use_memory_mapped_access();
} else {
    // Fall back to streaming
    use_streaming_access();
}
```

## Future Extensions

### Planned Enhancements

1. **Compression Support**: Built-in compression for storage-constrained platforms
2. **Encryption**: File-level encryption for sensitive data
3. **Network File Systems**: Remote file access with caching
4. **Virtual File Systems**: Layered file systems with overlay support
5. **Hot Reloading**: Real-time asset reloading for development

### Platform-Specific Features

- **Console-Specific**: Integration with platform-specific storage APIs
- **Mobile Optimization**: Battery-aware I/O scheduling
- **Cloud Integration**: Seamless cloud save synchronization

## Conclusion

The Flight HAL File Interface provides a robust, performant, and flexible foundation for file I/O across diverse gaming platforms. Its design balances cross-platform compatibility with platform-specific optimizations, enabling developers to write code once while achieving optimal performance on each target platform.

The interface's comprehensive feature set, from basic file operations to advanced memory mapping and archive support, makes it suitable for everything from simple configuration file access to complex asset streaming systems in modern game engines.
