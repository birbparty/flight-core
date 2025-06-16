#ifndef FLIGHT_HAL_FILE_HPP
#define FLIGHT_HAL_FILE_HPP

#include "result.hpp"
#include <cstdint>
#include <cstddef>
#include <memory>

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

        // Abstract file interface
        class File
        {
        public:
            virtual ~File() = default;

            // Read operations
            virtual Result<size_t> read(void *buffer, size_t size) = 0;
            virtual Result<size_t> read_all(void *buffer, size_t buffer_size) = 0;

            // Write operations
            virtual Result<size_t> write(const void *buffer, size_t size) = 0;
            virtual Result<void> flush() = 0;

            // Positioning
            virtual Result<int64_t> seek(int64_t offset, SeekOrigin origin) = 0;
            virtual Result<int64_t> tell() const = 0;

            // File info
            virtual Result<uint64_t> size() const = 0;
            virtual bool is_open() const = 0;
            virtual bool is_eof() const = 0;

            // Synchronous I/O (forces data to disk)
            virtual Result<void> sync() = 0;
        };

        // File system operations (free functions)

        // Open a file with specified mode
        Result<std::unique_ptr<File>> open_file(const char *path, FileMode mode);

        // Check if file exists
        Result<bool> file_exists(const char *path);

        // Get file information
        Result<FileInfo> get_file_info(const char *path);

        // Delete a file
        Result<void> delete_file(const char *path);

        // Rename/move a file
        Result<void> rename_file(const char *old_path, const char *new_path);

        // Create a directory
        Result<void> create_directory(const char *path);

        // Create directory and all parent directories
        Result<void> create_directories(const char *path);

        // Delete an empty directory
        Result<void> delete_directory(const char *path);

        // Check if path is a directory
        Result<bool> is_directory(const char *path);

        // Platform-specific path utilities

        // Get the platform's path separator ('/' or '\')
        constexpr char get_path_separator() noexcept
        {
#if defined(_WIN32)
            return '\\';
#else
            return '/';
#endif
        }

        // Join path components
        // Note: Implementation should handle platform-specific separators
        Result<std::string> join_path(const char *base, const char *relative);

        // Get the directory part of a path
        Result<std::string> get_directory(const char *path);

        // Get the filename part of a path
        Result<std::string> get_filename(const char *path);

        // Get file extension (including the dot)
        Result<std::string> get_extension(const char *path);

        // Normalize a path (resolve . and .., remove duplicate separators)
        Result<std::string> normalize_path(const char *path);

        // Get absolute path
        Result<std::string> get_absolute_path(const char *path);

        // Platform-specific temporary file support

        // Get temporary directory path
        Result<std::string> get_temp_directory();

        // Create a temporary file with optional prefix
        Result<std::pair<std::unique_ptr<File>, std::string>> create_temp_file(
            const char *prefix = nullptr);

        // Utility functions for common operations

        // Read entire file into a buffer
        // Note: On memory-constrained platforms, prefer streaming reads
        Result<std::vector<uint8_t>> read_file_contents(const char *path);

        // Write buffer to file (creates or overwrites)
        Result<void> write_file_contents(const char *path, const void *data, size_t size);

        // Copy file from source to destination
        Result<void> copy_file(const char *source, const char *destination);

        // Platform capabilities for file operations
        namespace file_capabilities
        {
            // Check if platform supports memory-mapped files
            constexpr bool has_memory_mapping() noexcept
            {
#if FLIGHT_HAS_MMAP
                return true;
#else
                return false;
#endif
            }

            // Check if platform supports file locking
            constexpr bool has_file_locking() noexcept
            {
#if defined(FLIGHT_PLATFORM_MACOS) || defined(FLIGHT_PLATFORM_POSIX)
                return true;
#else
                return false;
#endif
            }

            // Check if platform supports symbolic links
            constexpr bool has_symbolic_links() noexcept
            {
#if defined(FLIGHT_PLATFORM_POSIX) || defined(FLIGHT_PLATFORM_MACOS)
                return true;
#else
                return false;
#endif
            }
        }

    } // namespace hal
} // namespace flight

#endif // FLIGHT_HAL_FILE_HPP
