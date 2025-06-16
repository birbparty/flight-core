/**
 * @file file.hpp
 * @brief Flight HAL Elite File I/O and Storage Interface
 * 
 * Unified file system interface handling platform-specific storage from GD-ROM/UMD
 * to modern filesystems with async I/O, memory mapping, and archive format support.
 * Designed for cross-platform game development spanning 25+ years of hardware evolution.
 */

#pragma once

#include "../core/driver_registry.hpp"
#include "../core/hal_result.hpp"
#include "../core/hal_capabilities.hpp"
#include "memory.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>
#include <string>
#include <string_view>
#include <memory>
#include <chrono>

namespace flight::hal {

/**
 * @brief File system specific capabilities
 */
enum class FileSystemCapability : uint32_t {
    SynchronousIO = 1 << 0,        ///< Basic synchronous file operations
    AsynchronousIO = 1 << 1,       ///< Asynchronous I/O with callbacks
    MemoryMapping = 1 << 2,        ///< Memory-mapped file support
    DirectoryEnum = 1 << 3,        ///< Directory listing capabilities
    FileWatching = 1 << 4,         ///< File change notifications
    ArchiveSupport = 1 << 5,       ///< Archive format handling (PAK, ZIP, etc.)
    StreamingIO = 1 << 6,          ///< Streaming large files
    NetworkFS = 1 << 7,            ///< Network filesystem access
    ReadOnly = 1 << 8,             ///< Read-only storage (GD-ROM, UMD)
    Executable = 1 << 9,           ///< Can execute files
    Compression = 1 << 10,         ///< Built-in compression support
    Seeking = 1 << 11,             ///< Random access seeking
    Truncation = 1 << 12,          ///< File truncation support
    Locking = 1 << 13,             ///< File locking mechanisms
    Permissions = 1 << 14,         ///< File permission management
    Timestamps = 1 << 15,          ///< File timestamp support
    HardLinks = 1 << 16,           ///< Hard link support
    SymbolicLinks = 1 << 17,       ///< Symbolic link support
    CaseSensitive = 1 << 18        ///< Case-sensitive filenames
};

/**
 * @brief File access modes
 */
enum class FileAccessMode : uint8_t {
    ReadOnly = 0,       ///< Read-only access
    WriteOnly,          ///< Write-only access
    ReadWrite,          ///< Read and write access
    Append,             ///< Append mode (write at end)
    Create,             ///< Create new file (fail if exists)
    CreateOrTruncate,   ///< Create or truncate existing file
    CreateOrOpen        ///< Create new or open existing file
};

/**
 * @brief File access patterns for optimization hints
 */
enum class FileAccessPattern : uint8_t {
    Sequential = 0,     ///< Sequential reading (optimal for optical media)
    Random,            ///< Random access (good for SSDs)
    Streaming,         ///< Large file streaming
    MemoryMapped,      ///< Memory-mapped access
    WriteOnce,         ///< Write once, read many
    Temporary          ///< Temporary file, optimize for speed
};

/**
 * @brief File types for classification
 */
enum class FileType : uint8_t {
    Regular = 0,        ///< Regular file
    Directory,          ///< Directory/folder
    SymbolicLink,       ///< Symbolic link
    HardLink,           ///< Hard link
    Archive,            ///< Archive file (ZIP, PAK, etc.)
    Device,             ///< Device file
    Special,            ///< Special file (pipe, socket, etc.)
    Unknown             ///< Unknown file type
};

/**
 * @brief File sharing modes
 */
enum class FileShareMode : uint8_t {
    None = 0,           ///< Exclusive access
    Read,               ///< Allow others to read
    Write,              ///< Allow others to write
    ReadWrite,          ///< Allow others to read and write
    Delete              ///< Allow others to delete
};

/**
 * @brief Archive format types
 */
enum class ArchiveFormat : uint8_t {
    Unknown = 0,        ///< Unknown or unsupported format
    ZIP,                ///< ZIP archive
    PAK,                ///< PAK archive (Quake-style)
    WAD,                ///< WAD archive (Doom-style)
    TAR,                ///< TAR archive
    SevenZip,           ///< 7-Zip archive
    Custom              ///< Custom game-specific format
};

/**
 * @brief File system types
 */
enum class FileSystemType : uint8_t {
    Unknown = 0,        ///< Unknown filesystem
    Native,             ///< Native OS filesystem
    ISO9660,            ///< ISO 9660 (CD-ROM)
    GDROM,              ///< Dreamcast GD-ROM
    UMD,                ///< PSP Universal Media Disc
    Archive,            ///< Archive-based virtual filesystem
    Network,            ///< Network filesystem
    Memory,             ///< In-memory filesystem
    Browser             ///< Browser-based filesystem
};

/**
 * @brief Seek origin for file positioning
 */
enum class SeekOrigin : uint8_t {
    Begin = 0,          ///< Seek from beginning of file
    Current,            ///< Seek from current position
    End                 ///< Seek from end of file
};

/**
 * @brief File permission flags
 */
enum class FilePermission : uint16_t {
    None = 0,
    OwnerRead = 1 << 0,     ///< Owner read permission
    OwnerWrite = 1 << 1,    ///< Owner write permission
    OwnerExecute = 1 << 2,  ///< Owner execute permission
    GroupRead = 1 << 3,     ///< Group read permission
    GroupWrite = 1 << 4,    ///< Group write permission
    GroupExecute = 1 << 5,  ///< Group execute permission
    OtherRead = 1 << 6,     ///< Other read permission
    OtherWrite = 1 << 7,    ///< Other write permission
    OtherExecute = 1 << 8,  ///< Other execute permission
    
    // Common combinations
    ReadOnly = OwnerRead | GroupRead | OtherRead,
    ReadWrite = OwnerRead | OwnerWrite | GroupRead | GroupWrite | OtherRead | OtherWrite,
    AllAccess = 0x1FF       ///< All permissions
};

/**
 * @brief File handle for lightweight file references
 */
struct FileHandle {
    uint32_t id;                    ///< Unique file identifier
    FileType type;                  ///< File type classification
    uint32_t generation;            ///< Generation counter for validation
    
    bool is_valid() const { return id != 0; }
    void invalidate() { id = 0; generation = 0; }
    
    bool operator==(const FileHandle& other) const {
        return id == other.id && generation == other.generation;
    }
    
    bool operator!=(const FileHandle& other) const {
        return !(*this == other);
    }
};

/**
 * @brief File information structure
 */
struct FileInfo {
    std::string name;               ///< File name
    std::string full_path;          ///< Full file path
    FileType type;                  ///< File type
    uint64_t size;                  ///< File size in bytes
    FilePermission permissions;     ///< File permissions
    std::chrono::system_clock::time_point created_time;    ///< Creation time
    std::chrono::system_clock::time_point modified_time;   ///< Last modification time
    std::chrono::system_clock::time_point accessed_time;   ///< Last access time
    bool is_hidden;                 ///< Hidden file flag
    bool is_system;                 ///< System file flag
    bool is_archive;                ///< Archive flag
    bool is_compressed;             ///< Compressed flag
    uint32_t attributes;            ///< Platform-specific attributes
};

/**
 * @brief File open parameters
 */
struct FileOpenParams {
    FileAccessMode access_mode;     ///< File access mode
    FileShareMode share_mode;       ///< Sharing mode
    FileAccessPattern access_pattern; ///< Access pattern hint
    uint32_t buffer_size;           ///< Preferred buffer size (0 for default)
    bool enable_caching;            ///< Enable file caching
    bool direct_io;                 ///< Direct I/O bypass cache
    bool sequential_scan;           ///< Sequential scan hint
    bool random_access;             ///< Random access hint
    bool delete_on_close;           ///< Delete file when closed
    uint32_t timeout_ms;            ///< Open timeout in milliseconds
    std::string_view debug_name;    ///< Debug name for profiling
};

/**
 * @brief Memory-mapped file descriptor
 */
struct MemoryMappedFile {
    void* data;                     ///< Mapped memory pointer
    size_t size;                    ///< Size of mapped region
    FileHandle file_handle;         ///< Associated file handle
    bool is_writable;               ///< Whether mapping is writable
    bool is_executable;             ///< Whether mapping is executable
    size_t offset;                  ///< Offset within file
    size_t page_size;               ///< System page size
    uint32_t protection_flags;      ///< Platform-specific protection flags
};

/**
 * @brief Archive entry information
 */
struct ArchiveEntry {
    std::string name;               ///< Entry name within archive
    std::string full_path;          ///< Full path within archive
    uint64_t compressed_size;       ///< Compressed size in bytes
    uint64_t uncompressed_size;     ///< Uncompressed size in bytes
    uint32_t crc32;                 ///< CRC32 checksum
    std::chrono::system_clock::time_point modified_time; ///< Modification time
    bool is_directory;              ///< Directory entry flag
    bool is_encrypted;              ///< Encrypted entry flag
    uint32_t compression_method;    ///< Compression method used
};

/**
 * @brief File system statistics
 */
struct FileSystemStats {
    uint64_t total_space;           ///< Total filesystem space in bytes
    uint64_t free_space;            ///< Free space in bytes
    uint64_t available_space;       ///< Available space for unprivileged users
    uint64_t total_files;           ///< Total number of files (if available)
    uint64_t free_files;            ///< Free file nodes (if available)
    uint32_t block_size;            ///< Filesystem block size
    uint32_t max_filename_length;   ///< Maximum filename length
    uint32_t max_path_length;       ///< Maximum path length
    FileSystemType type;            ///< Filesystem type
    uint32_t capabilities;          ///< Supported FileSystemCapability bitmask
    bool is_case_sensitive;         ///< Case-sensitive filenames
    bool is_read_only;              ///< Read-only filesystem
    bool supports_unicode;          ///< Unicode filename support
};

/**
 * @brief I/O operation statistics
 */
struct FileIOStats {
    uint64_t bytes_read;            ///< Total bytes read
    uint64_t bytes_written;         ///< Total bytes written
    uint64_t read_operations;       ///< Number of read operations
    uint64_t write_operations;      ///< Number of write operations
    uint64_t seek_operations;       ///< Number of seek operations
    uint64_t cache_hits;            ///< Cache hit count
    uint64_t cache_misses;          ///< Cache miss count
    double average_read_time_ms;    ///< Average read time in milliseconds
    double average_write_time_ms;   ///< Average write time in milliseconds
    double average_seek_time_ms;    ///< Average seek time in milliseconds
    uint64_t last_access_time;      ///< Last access timestamp
};

// Forward declarations
class IArchiveProvider;
class IFileWatcher;
class IMemoryMappedFileView;

/**
 * @brief File I/O callback function types
 */
using FileIOCallback = std::function<void(HALResult<size_t> bytes_processed)>;
using FileOpenCallback = std::function<void(HALResult<FileHandle> file_handle)>;
using DirectoryEnumCallback = std::function<void(HALResult<std::vector<FileInfo>> entries)>;
using FileWatchCallback = std::function<void(const std::string& path, uint32_t event_flags)>;
using ArchiveEnumCallback = std::function<void(HALResult<std::vector<ArchiveEntry>> entries)>;

/**
 * @brief File watch events
 */
enum class FileWatchEvent : uint32_t {
    Created = 1 << 0,           ///< File or directory created
    Deleted = 1 << 1,           ///< File or directory deleted
    Modified = 1 << 2,          ///< File modified
    Renamed = 1 << 3,           ///< File or directory renamed
    AttributeChanged = 1 << 4,  ///< File attributes changed
    SizeChanged = 1 << 5,       ///< File size changed
    SecurityChanged = 1 << 6    ///< File security changed
};

/**
 * @brief Archive provider interface
 * 
 * Abstract interface for different archive format handlers.
 */
class IArchiveProvider {
public:
    virtual ~IArchiveProvider() = default;
    
    /**
     * @brief Get supported archive format
     * @return Archive format this provider handles
     */
    virtual ArchiveFormat get_format() const = 0;
    
    /**
     * @brief Check if file is supported archive
     * @param file_path Path to potential archive file
     * @return true if file is supported archive format
     */
    virtual bool is_supported_archive(const std::string& file_path) const = 0;
    
    /**
     * @brief Open archive for reading
     * @param file_path Path to archive file
     * @return HALResult containing archive handle on success
     */
    virtual HALResult<FileHandle> open_archive(const std::string& file_path) = 0;
    
    /**
     * @brief Close archive
     * @param archive_handle Archive handle to close
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> close_archive(FileHandle archive_handle) = 0;
    
    /**
     * @brief Enumerate archive contents
     * @param archive_handle Archive handle
     * @param callback Callback for receiving entries
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> enumerate_archive(FileHandle archive_handle,
                                             ArchiveEnumCallback callback) = 0;
    
    /**
     * @brief Extract file from archive
     * @param archive_handle Archive handle
     * @param entry_path Path within archive
     * @param output_buffer Buffer to receive extracted data
     * @param buffer_size Size of output buffer
     * @return HALResult containing bytes extracted on success
     */
    virtual HALResult<size_t> extract_file(FileHandle archive_handle,
                                          const std::string& entry_path,
                                          void* output_buffer,
                                          size_t buffer_size) = 0;
    
    /**
     * @brief Get entry information
     * @param archive_handle Archive handle
     * @param entry_path Path within archive
     * @return HALResult containing entry information on success
     */
    virtual HALResult<ArchiveEntry> get_entry_info(FileHandle archive_handle,
                                                   const std::string& entry_path) = 0;
};

/**
 * @brief File watcher interface
 * 
 * Provides file system change notification capabilities.
 */
class IFileWatcher {
public:
    virtual ~IFileWatcher() = default;
    
    /**
     * @brief Start watching directory
     * @param directory_path Directory to watch
     * @param event_mask Events to watch for (FileWatchEvent bitmask)
     * @param recursive Watch subdirectories recursively
     * @param callback Callback for file events
     * @return HALResult containing watch ID on success
     */
    virtual HALResult<uint32_t> watch_directory(const std::string& directory_path,
                                               uint32_t event_mask,
                                               bool recursive,
                                               FileWatchCallback callback) = 0;
    
    /**
     * @brief Start watching single file
     * @param file_path File to watch
     * @param event_mask Events to watch for
     * @param callback Callback for file events
     * @return HALResult containing watch ID on success
     */
    virtual HALResult<uint32_t> watch_file(const std::string& file_path,
                                          uint32_t event_mask,
                                          FileWatchCallback callback) = 0;
    
    /**
     * @brief Stop watching
     * @param watch_id Watch ID returned from watch_directory or watch_file
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> stop_watching(uint32_t watch_id) = 0;
    
    /**
     * @brief Stop all watches
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> stop_all_watches() = 0;
    
    /**
     * @brief Check if file watching is supported
     * @return true if file watching is supported on this platform
     */
    virtual bool is_supported() const = 0;
};

/**
 * @brief Memory-mapped file view interface
 * 
 * Provides access to memory-mapped file regions.
 */
class IMemoryMappedFileView {
public:
    virtual ~IMemoryMappedFileView() = default;
    
    /**
     * @brief Get mapped memory pointer
     * @return Pointer to mapped memory
     */
    virtual void* data() const = 0;
    
    /**
     * @brief Get size of mapped region
     * @return Size in bytes
     */
    virtual size_t size() const = 0;
    
    /**
     * @brief Get file offset of mapped region
     * @return Offset within file
     */
    virtual size_t offset() const = 0;
    
    /**
     * @brief Check if mapping is writable
     * @return true if writable
     */
    virtual bool is_writable() const = 0;
    
    /**
     * @brief Flush changes to disk
     * @param offset Offset within mapped region (0 for entire region)
     * @param size Size to flush (0 for entire region)
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> flush(size_t offset = 0, size_t size = 0) = 0;
    
    /**
     * @brief Advise kernel about access pattern
     * @param access_pattern Access pattern hint
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> advise_access_pattern(FileAccessPattern access_pattern) = 0;
    
    /**
     * @brief Lock pages in memory
     * @param offset Offset within mapped region
     * @param size Size to lock
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> lock_pages(size_t offset = 0, size_t size = 0) = 0;
    
    /**
     * @brief Unlock pages from memory
     * @param offset Offset within mapped region
     * @param size Size to unlock
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> unlock_pages(size_t offset = 0, size_t size = 0) = 0;
};

/**
 * @brief Enhanced file I/O interface
 * 
 * Comprehensive file system interface supporting both synchronous and asynchronous
 * operations, memory mapping, archive handling, and cross-platform optimizations
 * for gaming applications.
 */
class IFileInterface : public IHALInterface, public ICapabilityProvider {
public:
    virtual ~IFileInterface() = default;
    
    // === File System Information ===
    
    /**
     * @brief Get file system statistics
     * @param path Path to query (empty for root filesystem)
     * @return HALResult containing filesystem statistics
     */
    virtual HALResult<FileSystemStats> get_filesystem_stats(const std::string& path = "") = 0;
    
    /**
     * @brief Get current working directory
     * @return HALResult containing current directory path
     */
    virtual HALResult<std::string> get_current_directory() = 0;
    
    /**
     * @brief Set current working directory
     * @param path New working directory path
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_current_directory(const std::string& path) = 0;
    
    /**
     * @brief Check if path exists
     * @param path Path to check
     * @return true if path exists
     */
    virtual bool exists(const std::string& path) = 0;
    
    /**
     * @brief Get file information
     * @param path File path
     * @return HALResult containing file information
     */
    virtual HALResult<FileInfo> get_file_info(const std::string& path) = 0;
    
    // === Synchronous File Operations ===
    
    /**
     * @brief Open file with specified parameters
     * @param path File path to open
     * @param params File open parameters
     * @return HALResult containing file handle on success
     */
    virtual HALResult<FileHandle> open_file(const std::string& path,
                                          const FileOpenParams& params) = 0;
    
    /**
     * @brief Open file with simple access mode
     * @param path File path to open
     * @param access_mode File access mode
     * @return HALResult containing file handle on success
     */
    virtual HALResult<FileHandle> open_file(const std::string& path,
                                          FileAccessMode access_mode = FileAccessMode::ReadOnly) = 0;
    
    /**
     * @brief Close file
     * @param file_handle File handle to close
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> close_file(FileHandle file_handle) = 0;
    
    /**
     * @brief Read data from file
     * @param file_handle File handle
     * @param buffer Buffer to receive data
     * @param size Number of bytes to read
     * @return HALResult containing bytes read on success
     */
    virtual HALResult<size_t> read_file(FileHandle file_handle, void* buffer, size_t size) = 0;
    
    /**
     * @brief Write data to file
     * @param file_handle File handle
     * @param data Data to write
     * @param size Number of bytes to write
     * @return HALResult containing bytes written on success
     */
    virtual HALResult<size_t> write_file(FileHandle file_handle, const void* data, size_t size) = 0;
    
    /**
     * @brief Seek to position in file
     * @param file_handle File handle
     * @param offset Offset to seek to
     * @param origin Seek origin
     * @return HALResult containing new file position on success
     */
    virtual HALResult<uint64_t> seek_file(FileHandle file_handle, int64_t offset, SeekOrigin origin) = 0;
    
    /**
     * @brief Get current file position
     * @param file_handle File handle
     * @return HALResult containing current position on success
     */
    virtual HALResult<uint64_t> tell_file(FileHandle file_handle) = 0;
    
    /**
     * @brief Get file size
     * @param file_handle File handle
     * @return HALResult containing file size on success
     */
    virtual HALResult<uint64_t> get_file_size(FileHandle file_handle) = 0;
    
    /**
     * @brief Flush file buffers to disk
     * @param file_handle File handle
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> flush_file(FileHandle file_handle) = 0;
    
    /**
     * @brief Truncate file to specified size
     * @param file_handle File handle
     * @param size New file size
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> truncate_file(FileHandle file_handle, uint64_t size) = 0;
    
    // === Asynchronous File Operations ===
    
    /**
     * @brief Asynchronously open file
     * @param path File path to open
     * @param params File open parameters
     * @param callback Callback for completion
     * @return HALResult containing operation ID on success
     */
    virtual HALResult<uint32_t> open_file_async(const std::string& path,
                                               const FileOpenParams& params,
                                               FileOpenCallback callback) = 0;
    
    /**
     * @brief Asynchronously read from file
     * @param file_handle File handle
     * @param buffer Buffer to receive data
     * @param size Number of bytes to read
     * @param callback Callback for completion
     * @return HALResult containing operation ID on success
     */
    virtual HALResult<uint32_t> read_file_async(FileHandle file_handle,
                                               void* buffer,
                                               size_t size,
                                               FileIOCallback callback) = 0;
    
    /**
     * @brief Asynchronously write to file
     * @param file_handle File handle
     * @param data Data to write
     * @param size Number of bytes to write
     * @param callback Callback for completion
     * @return HALResult containing operation ID on success
     */
    virtual HALResult<uint32_t> write_file_async(FileHandle file_handle,
                                                const void* data,
                                                size_t size,
                                                FileIOCallback callback) = 0;
    
    /**
     * @brief Cancel asynchronous operation
     * @param operation_id Operation ID returned from async function
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> cancel_async_operation(uint32_t operation_id) = 0;
    
    /**
     * @brief Wait for asynchronous operation completion
     * @param operation_id Operation ID
     * @param timeout_ms Timeout in milliseconds (0 for infinite)
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> wait_for_async_operation(uint32_t operation_id, uint32_t timeout_ms = 0) = 0;
    
    // === Directory Operations ===
    
    /**
     * @brief Create directory
     * @param path Directory path to create
     * @param recursive Create parent directories if needed
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> create_directory(const std::string& path, bool recursive = false) = 0;
    
    /**
     * @brief Remove directory
     * @param path Directory path to remove
     * @param recursive Remove subdirectories and files
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> remove_directory(const std::string& path, bool recursive = false) = 0;
    
    /**
     * @brief Enumerate directory contents
     * @param path Directory path to enumerate
     * @return HALResult containing vector of file information
     */
    virtual HALResult<std::vector<FileInfo>> enumerate_directory(const std::string& path) = 0;
    
    /**
     * @brief Asynchronously enumerate directory contents
     * @param path Directory path to enumerate
     * @param callback Callback for receiving entries
     * @return HALResult containing operation ID on success
     */
    virtual HALResult<uint32_t> enumerate_directory_async(const std::string& path,
                                                         DirectoryEnumCallback callback) = 0;
    
    // === File Management Operations ===
    
    /**
     * @brief Copy file
     * @param source_path Source file path
     * @param destination_path Destination file path
     * @param overwrite_existing Overwrite if destination exists
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> copy_file(const std::string& source_path,
                                     const std::string& destination_path,
                                     bool overwrite_existing = false) = 0;
    
    /**
     * @brief Move/rename file
     * @param source_path Source file path
     * @param destination_path Destination file path
     * @param overwrite_existing Overwrite if destination exists
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> move_file(const std::string& source_path,
                                     const std::string& destination_path,
                                     bool overwrite_existing = false) = 0;
    
    /**
     * @brief Delete file
     * @param path File path to delete
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> delete_file(const std::string& path) = 0;
    
    /**
     * @brief Set file permissions
     * @param path File path
     * @param permissions New permissions
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_file_permissions(const std::string& path, FilePermission permissions) = 0;
    
    /**
     * @brief Set file timestamps
     * @param path File path
     * @param access_time New access time
     * @param modification_time New modification time
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_file_times(const std::string& path,
                                          const std::chrono::system_clock::time_point& access_time,
                                          const std::chrono::system_clock::time_point& modification_time) = 0;
    
    // === Memory-Mapped File Operations ===
    
    /**
     * @brief Create memory-mapped file view
     * @param file_handle File handle to map
     * @param offset Offset within file
     * @param size Size to map (0 for entire file)
     * @param writable Whether mapping should be writable
     * @return HALResult containing memory-mapped file view on success
     */
    virtual HALResult<std::unique_ptr<IMemoryMappedFileView>> create_memory_mapped_view(
        FileHandle file_handle,
        size_t offset = 0,
        size_t size = 0,
        bool writable = false) = 0;
    
    /**
     * @brief Create memory-mapped file directly from path
     * @param path File path to map
     * @param access_mode File access mode
     * @param offset Offset within file
     * @param size Size to map (0 for entire file)
     * @return HALResult containing memory-mapped file descriptor on success
     */
    virtual HALResult<MemoryMappedFile> create_memory_mapped_file(const std::string& path,
                                                                 FileAccessMode access_mode = FileAccessMode::ReadOnly,
                                                                 size_t offset = 0,
                                                                 size_t size = 0) = 0;
    
    /**
     * @brief Close memory-mapped file
     * @param mapped_file Memory-mapped file descriptor
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> close_memory_mapped_file(const MemoryMappedFile& mapped_file) = 0;
    
    // === Archive Operations ===
    
    /**
     * @brief Register archive provider
     * @param provider Archive provider to register
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> register_archive_provider(std::unique_ptr<IArchiveProvider> provider) = 0;
    
    /**
     * @brief Unregister archive provider
     * @param format Archive format to unregister
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> unregister_archive_provider(ArchiveFormat format) = 0;
    
    /**
     * @brief Get archive provider for format
     * @param format Archive format
     * @return Pointer to archive provider (nullptr if not found)
     */
    virtual IArchiveProvider* get_archive_provider(ArchiveFormat format) = 0;
    
    /**
     * @brief Open archive file
     * @param path Path to archive file
     * @return HALResult containing archive handle on success
     */
    virtual HALResult<FileHandle> open_archive(const std::string& path) = 0;
    
    /**
     * @brief Extract file from archive
     * @param archive_handle Archive handle
     * @param entry_path Path within archive
     * @param output_path Output file path
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> extract_archive_file(FileHandle archive_handle,
                                                const std::string& entry_path,
                                                const std::string& output_path) = 0;
    
    /**
     * @brief Extract archive file to memory
     * @param archive_handle Archive handle
     * @param entry_path Path within archive
     * @param allocator Memory allocator to use (nullptr for default)
     * @return HALResult containing allocated buffer with extracted data
     */
    virtual HALResult<MemoryAllocation> extract_archive_file_to_memory(FileHandle archive_handle,
                                                                      const std::string& entry_path,
                                                                      IMemoryAllocator* allocator = nullptr) = 0;
    
    /**
     * @brief Mount archive as virtual filesystem
     * @param archive_handle Archive handle
     * @param mount_point Virtual mount point
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> mount_archive(FileHandle archive_handle, const std::string& mount_point) = 0;
    
    /**
     * @brief Unmount archive filesystem
     * @param mount_point Virtual mount point
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> unmount_archive(const std::string& mount_point) = 0;
    
    // === File Watching ===
    
    /**
     * @brief Get file watcher interface
     * @return Pointer to file watcher interface (nullptr if not supported)
     */
    virtual IFileWatcher* get_file_watcher() = 0;
    
    /**
     * @brief Enable file change notifications
     * @param path Path to watch (file or directory)
     * @param event_mask Events to watch for
     * @param recursive Watch subdirectories recursively
     * @param callback Callback for file events
     * @return HALResult containing watch ID on success
     */
    virtual HALResult<uint32_t> watch_path(const std::string& path,
                                          uint32_t event_mask,
                                          bool recursive,
                                          FileWatchCallback callback) = 0;
    
    /**
     * @brief Disable file change notifications
     * @param watch_id Watch ID returned from watch_path
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> unwatch_path(uint32_t watch_id) = 0;
    
    // === Streaming I/O ===
    
    /**
     * @brief Create streaming file reader
     * @param file_handle File handle
     * @param buffer_size Stream buffer size
     * @return HALResult containing stream handle on success
     */
    virtual HALResult<FileHandle> create_file_stream(FileHandle file_handle, size_t buffer_size = 0) = 0;
    
    /**
     * @brief Read from file stream
     * @param stream_handle Stream handle
     * @param buffer Buffer to receive data
     * @param size Number of bytes to read
     * @return HALResult containing bytes read on success
     */
    virtual HALResult<size_t> read_stream(FileHandle stream_handle, void* buffer, size_t size) = 0;
    
    /**
     * @brief Close file stream
     * @param stream_handle Stream handle
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> close_stream(FileHandle stream_handle) = 0;
    
    // === Bulk Operations ===
    
    /**
     * @brief Read entire file into memory
     * @param path File path
     * @param allocator Memory allocator to use (nullptr for default)
     * @return HALResult containing allocated buffer with file contents
     */
    virtual HALResult<MemoryAllocation> read_entire_file(const std::string& path,
                                                        IMemoryAllocator* allocator = nullptr) = 0;
    
    /**
     * @brief Write entire buffer to file
     * @param path File path
     * @param data Data to write
     * @param size Size of data
     * @param overwrite_existing Overwrite if file exists
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> write_entire_file(const std::string& path,
                                             const void* data,
                                             size_t size,
                                             bool overwrite_existing = true) = 0;
    
    /**
     * @brief Copy file with progress callback
     * @param source_path Source file path
     * @param destination_path Destination file path
     * @param progress_callback Progress callback (optional)
     * @param overwrite_existing Overwrite if destination exists
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> copy_file_with_progress(const std::string& source_path,
                                                   const std::string& destination_path,
                                                   std::function<void(uint64_t bytes_copied, uint64_t total_bytes)> progress_callback = nullptr,
                                                   bool overwrite_existing = false) = 0;
    
    // === Capability Queries ===
    
    /**
     * @brief Check if filesystem capability is supported
     * @param capability Filesystem capability to check
     * @return true if capability is supported
     */
    virtual bool supports_filesystem_capability(FileSystemCapability capability) const = 0;
    
    /**
     * @brief Check if file access mode is supported
     * @param access_mode File access mode to check
     * @return true if access mode is supported
     */
    virtual bool supports_access_mode(FileAccessMode access_mode) const = 0;
    
    /**
     * @brief Check if archive format is supported
     * @param format Archive format to check
     * @return true if archive format is supported
     */
    virtual bool supports_archive_format(ArchiveFormat format) const = 0;
    
    /**
     * @brief Get maximum file size supported
     * @return Maximum file size in bytes
     */
    virtual uint64_t get_max_file_size() const = 0;
    
    /**
     * @brief Get maximum path length supported
     * @return Maximum path length in characters
     */
    virtual uint32_t get_max_path_length() const = 0;
    
    /**
     * @brief Get supported archive formats
     * @return Vector of supported archive formats
     */
    virtual std::vector<ArchiveFormat> get_supported_archive_formats() const = 0;
    
    // === Statistics and Performance ===
    
    /**
     * @brief Get file I/O statistics
     * @param file_handle File handle (invalid handle for global stats)
     * @return HALResult containing I/O statistics
     */
    virtual HALResult<FileIOStats> get_io_stats(FileHandle file_handle = {}) = 0;
    
    /**
     * @brief Reset I/O statistics
     * @param file_handle File handle (invalid handle for global stats)
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> reset_io_stats(FileHandle file_handle = {}) = 0;
    
    /**
     * @brief Get cache statistics
     * @return HALResult containing cache statistics
     */
    virtual HALResult<std::pair<uint64_t, uint64_t>> get_cache_stats() = 0; // hits, misses
    
    /**
     * @brief Flush all file system caches
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> flush_all_caches() = 0;
    
    /**
     * @brief Set cache size limit
     * @param size_bytes Cache size limit in bytes
     * @return HALResult indicating success or failure
     */
    virtual HALResult<void> set_cache_size_limit(size_t size_bytes) = 0;
    
    // === Platform-Specific Extensions ===
    
    /**
     * @brief Get platform-specific extension interface
     * @param extension_name Name of extension to query
     * @return Pointer to extension interface (nullptr if not supported)
     */
    virtual void* get_extension_interface(std::string_view extension_name) = 0;
    
    /**
     * @brief Dreamcast GD-ROM specific: Get sector size
     * @return Sector size in bytes (GD-ROM specific)
     */
    virtual uint32_t get_sector_size() const = 0;
    
    /**
     * @brief PSP UMD specific: Get UMD info
     * @return UMD-specific information structure
     */
    virtual void* get_umd_info() const = 0;
    
    /**
     * @brief Web platform specific: Get browser storage interface
     * @return Browser storage interface
     */
    virtual void* get_browser_storage_interface() = 0;
    
    // === IHALInterface implementation ===
    std::string_view get_interface_name() const override {
        return "file";
    }
};

// === Utility Functions ===

/**
 * @brief File utility functions
 */
namespace file {

/**
 * @brief Convert file system capability to string
 * @param capability File system capability to convert
 * @return String representation
 */
const char* to_string(FileSystemCapability capability);

/**
 * @brief Convert file access mode to string
 * @param mode File access mode to convert
 * @return String representation
 */
const char* to_string(FileAccessMode mode);

/**
 * @brief Convert file access pattern to string
 * @param pattern File access pattern to convert
 * @return String representation
 */
const char* to_string(FileAccessPattern pattern);

/**
 * @brief Convert file type to string
 * @param type File type to convert
 * @return String representation
 */
const char* to_string(FileType type);

/**
 * @brief Convert archive format to string
 * @param format Archive format to convert
 * @return String representation
 */
const char* to_string(ArchiveFormat format);

/**
 * @brief Convert file system type to string
 * @param type File system type to convert
 * @return String representation
 */
const char* to_string(FileSystemType type);

/**
 * @brief Get file extension from path
 * @param path File path
 * @return File extension (without dot)
 */
std::string get_file_extension(const std::string& path);

/**
 * @brief Get filename from path
 * @param path File path
 * @param with_extension Include file extension
 * @return Filename
 */
std::string get_filename(const std::string& path, bool with_extension = true);

/**
 * @brief Get directory from path
 * @param path File path
 * @return Directory path
 */
std::string get_directory(const std::string& path);

/**
 * @brief Join path components
 * @param paths Path components to join
 * @return Joined path
 */
std::string join_paths(const std::vector<std::string>& paths);

/**
 * @brief Normalize path (resolve .. and . components)
 * @param path Path to normalize
 * @return Normalized path
 */
std::string normalize_path(const std::string& path);

/**
 * @brief Check if path is absolute
 * @param path Path to check
 * @return true if path is absolute
 */
bool is_absolute_path(const std::string& path);

/**
 * @brief Convert relative path to absolute
 * @param path Relative path
 * @param base_path Base path (empty for current directory)
 * @return Absolute path
 */
std::string to_absolute_path(const std::string& path, const std::string& base_path = "");

/**
 * @brief Create default file open parameters
 * @param access_mode File access mode
 * @param access_pattern Access pattern hint
 * @return Configured FileOpenParams
 */
FileOpenParams make_file_open_params(FileAccessMode access_mode = FileAccessMode::ReadOnly,
                                    FileAccessPattern access_pattern = FileAccessPattern::Sequential);

/**
 * @brief Create file open parameters for streaming
 * @param access_mode File access mode
 * @param buffer_size Stream buffer size
 * @return Configured FileOpenParams for streaming
 */
FileOpenParams make_streaming_params(FileAccessMode access_mode = FileAccessMode::ReadOnly,
                                   uint32_t buffer_size = 64 * 1024);

/**
 * @brief Create file open parameters for memory mapping
 * @param writable Whether mapping should be writable
 * @return Configured FileOpenParams for memory mapping
 */
FileOpenParams make_memory_mapped_params(bool writable = false);

/**
 * @brief Calculate optimal buffer size for file operations
 * @param file_size File size in bytes
 * @param access_pattern Access pattern
 * @return Optimal buffer size
 */
uint32_t calculate_optimal_buffer_size(uint64_t file_size, FileAccessPattern access_pattern);

/**
 * @brief Check if filename is valid for platform
 * @param filename Filename to check
 * @param filesystem_type Target filesystem type
 * @return true if filename is valid
 */
bool is_valid_filename(const std::string& filename, FileSystemType filesystem_type = FileSystemType::Native);

/**
 * @brief Sanitize filename for platform
 * @param filename Filename to sanitize
 * @param filesystem_type Target filesystem type
 * @return Sanitized filename
 */
std::string sanitize_filename(const std::string& filename, FileSystemType filesystem_type = FileSystemType::Native);

} // namespace file

} // namespace flight::hal
