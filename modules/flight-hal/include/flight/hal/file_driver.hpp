#ifndef FLIGHT_HAL_FILE_DRIVER_HPP
#define FLIGHT_HAL_FILE_DRIVER_HPP

#include "driver.hpp"
#include "result.hpp"
#include <cstdint>
#include <memory>
#include <vector>
#include <string>

namespace flight
{
    namespace hal
    {

        // File open modes
        enum class FileMode : uint32_t
        {
            Read = 0x01,
            Write = 0x02,
            Append = 0x04,
            Create = 0x08,
            Truncate = 0x10,
            Binary = 0x20,

            // Common combinations
            ReadBinary = Read | Binary,
            WriteBinary = Write | Binary,
            ReadWrite = Read | Write,
            ReadWriteBinary = Read | Write | Binary,
            CreateWrite = Create | Write | Truncate,
            CreateWriteBinary = Create | Write | Truncate | Binary
        };

        // Bitwise operations for FileMode
        inline FileMode operator|(FileMode lhs, FileMode rhs)
        {
            return static_cast<FileMode>(
                static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
        }

        inline FileMode operator&(FileMode lhs, FileMode rhs)
        {
            return static_cast<FileMode>(
                static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
        }

        inline bool operator!(FileMode mode)
        {
            return static_cast<uint32_t>(mode) == 0;
        }

        // Seek origin for file positioning
        enum class SeekOrigin : uint32_t
        {
            Begin = 0,   // Seek from beginning of file
            Current = 1, // Seek from current position
            End = 2      // Seek from end of file
        };

        // File information structure
        struct FileInfo
        {
            uint64_t size;          // File size in bytes
            uint64_t modified_time; // Modification time (platform-specific epoch)
            uint64_t created_time;  // Creation time (platform-specific epoch)
            bool is_directory;      // True if this is a directory
            bool is_regular_file;   // True if this is a regular file
            bool is_readable;       // True if file can be read
            bool is_writable;       // True if file can be written
            bool is_executable;     // True if file can be executed
        };

        // File handle
        struct FileHandle
        {
            uint32_t id;
            bool operator==(const FileHandle &other) const { return id == other.id; }
            bool operator!=(const FileHandle &other) const { return id != other.id; }
        };

        constexpr FileHandle INVALID_FILE_HANDLE = {0};

        // Directory entry
        struct DirectoryEntry
        {
            std::string name;
            FileInfo info;
        };

        // File system capabilities
        struct FileSystemCapabilities
        {
            bool supports_directories;    // Can create/navigate directories
            bool supports_long_filenames; // Not 8.3 format
            bool supports_unicode_paths;  // UTF-8 paths
            bool supports_symbolic_links;
            bool supports_memory_mapping;
            bool supports_file_locking;
            bool supports_async_io;
            bool case_sensitive;          // File system is case sensitive
            uint32_t max_path_length;     // Maximum path length
            uint32_t max_filename_length; // Maximum filename length
            char path_separator;          // '/' or '\'
        };

        // File driver interface (RetroArch pattern)
        class FileDriver : public Driver
        {
        public:
            // Driver interface
            DriverType type() const override { return DriverType::File; }

            // Capability queries
            virtual FileSystemCapabilities capabilities() const = 0;

            // File operations
            virtual Result<FileHandle> open(const char *path, FileMode mode) = 0;
            virtual Result<void> close(FileHandle handle) = 0;

            // Read/write operations
            virtual Result<size_t> read(FileHandle handle, void *buffer, size_t size) = 0;
            virtual Result<size_t> write(FileHandle handle, const void *buffer, size_t size) = 0;
            virtual Result<void> flush(FileHandle handle) = 0;

            // File positioning
            virtual Result<int64_t> seek(FileHandle handle, int64_t offset, SeekOrigin origin) = 0;
            virtual Result<int64_t> tell(FileHandle handle) const = 0;
            virtual Result<bool> is_eof(FileHandle handle) const = 0;

            // File information
            virtual Result<FileInfo> get_info(const char *path) const = 0;
            virtual Result<bool> exists(const char *path) const = 0;
            virtual Result<uint64_t> get_size(FileHandle handle) const = 0;

            // File system operations
            virtual Result<void> delete_file(const char *path) = 0;
            virtual Result<void> rename(const char *old_path, const char *new_path) = 0;
            virtual Result<void> copy(const char *source, const char *destination) = 0;

            // Directory operations
            virtual Result<void> create_directory(const char *path) = 0;
            virtual Result<void> delete_directory(const char *path) = 0;
            virtual Result<std::vector<DirectoryEntry>> list_directory(const char *path) = 0;
            virtual Result<bool> is_directory(const char *path) const = 0;

            // Path utilities
            virtual Result<std::string> get_absolute_path(const char *path) const = 0;
            virtual Result<std::string> get_current_directory() const = 0;
            virtual Result<void> set_current_directory(const char *path) = 0;

            // Temporary files
            virtual Result<std::string> get_temp_directory() const = 0;
            virtual Result<std::pair<FileHandle, std::string>> create_temp_file(const char *prefix) = 0;

            // Platform-specific operations

            // Memory mapping (if supported)
            virtual Result<void *> map_file(FileHandle handle, size_t offset, size_t length, bool read_only)
            {
                return ErrorCode::NotSupported;
            }

            virtual Result<void> unmap_file(void *mapping, size_t length)
            {
                return ErrorCode::NotSupported;
            }

            // File locking (if supported)
            virtual Result<void> lock_file(FileHandle handle, bool exclusive)
            {
                return ErrorCode::NotSupported;
            }

            virtual Result<void> unlock_file(FileHandle handle)
            {
                return ErrorCode::NotSupported;
            }

            // Async I/O (if supported)
            virtual bool supports_async_io() const { return false; }

            // Platform-specific path handling
            virtual char get_path_separator() const = 0;
            virtual Result<std::string> normalize_path(const char *path) const = 0;
            virtual Result<std::string> join_paths(const char *base, const char *relative) const = 0;
            virtual Result<std::string> get_directory_name(const char *path) const = 0;
            virtual Result<std::string> get_file_name(const char *path) const = 0;
            virtual Result<std::string> get_extension(const char *path) const = 0;
        };

        // Helper functions for working with paths
        inline std::string join_path(const std::string &base, const std::string &relative, char separator)
        {
            if (base.empty())
                return relative;
            if (relative.empty())
                return base;

            bool base_has_sep = base.back() == separator;
            bool rel_has_sep = relative.front() == separator;

            if (base_has_sep && rel_has_sep)
            {
                return base + relative.substr(1);
            }
            else if (!base_has_sep && !rel_has_sep)
            {
                return base + separator + relative;
            }
            else
            {
                return base + relative;
            }
        }

        // Platform-specific file system defaults
        namespace filesystem_defaults
        {
            // Desktop defaults
            constexpr FileSystemCapabilities DESKTOP_CAPABILITIES = {
                .supports_directories = true,
                .supports_long_filenames = true,
                .supports_unicode_paths = true,
                .supports_symbolic_links = true,
                .supports_memory_mapping = true,
                .supports_file_locking = true,
                .supports_async_io = true,
                .case_sensitive = true, // Unix/macOS
                .max_path_length = 4096,
                .max_filename_length = 255,
                .path_separator = '/'};

            // Dreamcast defaults (ISO9660 CD-ROM)
            constexpr FileSystemCapabilities DREAMCAST_CAPABILITIES = {
                .supports_directories = true,
                .supports_long_filenames = false, // 8.3 format
                .supports_unicode_paths = false,
                .supports_symbolic_links = false,
                .supports_memory_mapping = false,
                .supports_file_locking = false,
                .supports_async_io = false,
                .case_sensitive = false,
                .max_path_length = 255,
                .max_filename_length = 12, // 8.3
                .path_separator = '/'};

            // PlayStation 1 defaults (ISO9660 CD-ROM)
            constexpr FileSystemCapabilities PSX_CAPABILITIES = {
                .supports_directories = true,
                .supports_long_filenames = false, // 8.3 format
                .supports_unicode_paths = false,
                .supports_symbolic_links = false,
                .supports_memory_mapping = false,
                .supports_file_locking = false,
                .supports_async_io = false,
                .case_sensitive = false,
                .max_path_length = 128,
                .max_filename_length = 12, // 8.3
                .path_separator = '\\'};

            // PSP defaults (Memory Stick)
            constexpr FileSystemCapabilities PSP_CAPABILITIES = {
                .supports_directories = true,
                .supports_long_filenames = true,
                .supports_unicode_paths = false,
                .supports_symbolic_links = false,
                .supports_memory_mapping = false,
                .supports_file_locking = false,
                .supports_async_io = true,
                .case_sensitive = false,
                .max_path_length = 512,
                .max_filename_length = 255,
                .path_separator = '/'};

            // Web/Emscripten defaults (Virtual FS)
            constexpr FileSystemCapabilities WEB_CAPABILITIES = {
                .supports_directories = true,
                .supports_long_filenames = true,
                .supports_unicode_paths = true,
                .supports_symbolic_links = false,
                .supports_memory_mapping = false,
                .supports_file_locking = false,
                .supports_async_io = true,
                .case_sensitive = true,
                .max_path_length = 4096,
                .max_filename_length = 255,
                .path_separator = '/'};
        }

    } // namespace hal
} // namespace flight

#endif // FLIGHT_HAL_FILE_DRIVER_HPP
