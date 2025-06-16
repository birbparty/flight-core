#ifndef FLIGHT_WASM_BINARY_PARSER_HPP
#define FLIGHT_WASM_BINARY_PARSER_HPP

/**
 * @file parser.hpp
 * @brief WebAssembly binary format parsing functionality
 * 
 * This header provides forward declarations and basic structure for parsing
 * WebAssembly binary format files. Full implementation will be completed in
 * the binary format parsing tasks.
 */

#include <cstdint>
#include <vector>
#include <string>

namespace flight::wasm {

    // Forward declarations
    class Module;
    class Error;
    template<typename T> class Result;

    /**
     * @brief WebAssembly binary format constants
     */
    namespace binary_constants {
        constexpr uint32_t WASM_MAGIC = 0x6d736100;  // "\0asm"
        constexpr uint32_t WASM_VERSION = 1;
        constexpr size_t MAGIC_SIZE = 4;
        constexpr size_t VERSION_SIZE = 4;
        constexpr size_t HEADER_SIZE = MAGIC_SIZE + VERSION_SIZE;
    }

    /**
     * @brief Simple span-like view for C++17 compatibility
     */
    template<typename T>
    class span {
    public:
        constexpr span() noexcept : data_(nullptr), size_(0) {}
        constexpr span(T* data, size_t size) noexcept : data_(data), size_(size) {}
        constexpr span(const std::vector<T>& vec) noexcept : data_(vec.data()), size_(vec.size()) {}
        
        constexpr T* data() const noexcept { return data_; }
        constexpr size_t size() const noexcept { return size_; }
        constexpr bool empty() const noexcept { return size_ == 0; }
        
        constexpr T& operator[](size_t index) const noexcept { return data_[index]; }
        constexpr T* begin() const noexcept { return data_; }
        constexpr T* end() const noexcept { return data_ + size_; }
        
    private:
        T* data_;
        size_t size_;
    };

    /**
     * @brief Binary reader for parsing WebAssembly binary format
     * 
     * This class provides low-level binary reading functionality with
     * proper error handling and bounds checking.
     */
    class BinaryReader {
    public:
        /**
         * @brief Construct a binary reader from a data span
         */
        explicit BinaryReader(span<const uint8_t> data) noexcept;

        /**
         * @brief Check if there are more bytes to read
         */
        bool has_data() const noexcept;

        /**
         * @brief Get the current read position
         */
        size_t position() const noexcept;

        /**
         * @brief Get the total data size
         */
        size_t size() const noexcept;

        /**
         * @brief Get remaining bytes count
         */
        size_t remaining() const noexcept;

        /**
         * @brief Peek at the next byte without consuming it
         */
        Result<uint8_t> peek_byte() const noexcept;

        /**
         * @brief Read a single byte
         */
        Result<uint8_t> read_byte() noexcept;

        /**
         * @brief Read multiple bytes
         */
        Result<std::vector<uint8_t>> read_bytes(size_t count) noexcept;

        /**
         * @brief Read a 32-bit unsigned integer (little-endian)
         */
        Result<uint32_t> read_u32() noexcept;

        /**
         * @brief Read a 64-bit unsigned integer (little-endian)
         */
        Result<uint64_t> read_u64() noexcept;

        /**
         * @brief Read a 32-bit float (little-endian)
         */
        Result<float> read_f32() noexcept;

        /**
         * @brief Read a 64-bit float (little-endian)
         */
        Result<double> read_f64() noexcept;

        /**
         * @brief Read a LEB128 unsigned integer
         */
        Result<uint32_t> read_leb128_u32() noexcept;

        /**
         * @brief Read a LEB128 signed integer
         */
        Result<int32_t> read_leb128_i32() noexcept;

        /**
         * @brief Read a LEB128 64-bit unsigned integer
         */
        Result<uint64_t> read_leb128_u64() noexcept;

        /**
         * @brief Read a LEB128 64-bit signed integer
         */
        Result<int64_t> read_leb128_i64() noexcept;

        /**
         * @brief Read a UTF-8 string
         */
        Result<std::string> read_string() noexcept;

        /**
         * @brief Skip bytes
         */
        Result<void> skip_bytes(size_t count) noexcept;

        /**
         * @brief Set read position
         */
        Result<void> seek(size_t position) noexcept;

    private:
        span<const uint8_t> data_;
        size_t position_ = 0;
    };

    /**
     * @brief WebAssembly binary parser
     * 
     * High-level parser that constructs Module objects from binary data.
     */
    class BinaryParser {
    public:
        /**
         * @brief Parse a WebAssembly binary module
         */
        static Result<Module> parse(span<const uint8_t> data) noexcept;

        /**
         * @brief Parse a WebAssembly binary module from a file
         */
        static Result<Module> parse_file(const std::string& filename) noexcept;

        /**
         * @brief Validate WebAssembly binary format without full parsing
         */
        static Result<void> validate(span<const uint8_t> data) noexcept;

        /**
         * @brief Check if data appears to be a valid WebAssembly binary
         */
        static bool is_wasm_binary(span<const uint8_t> data) noexcept;

    private:
        BinaryParser() = default;

        // Internal parsing methods (to be implemented)
        Result<Module> parse_module(BinaryReader& reader) noexcept;
        Result<void> parse_header(BinaryReader& reader) noexcept;
        Result<void> parse_sections(BinaryReader& reader, Module& module) noexcept;
        Result<void> parse_section(BinaryReader& reader, Module& module) noexcept;

        // Section-specific parsers
        Result<void> parse_type_section(BinaryReader& reader, Module& module) noexcept;
        Result<void> parse_import_section(BinaryReader& reader, Module& module) noexcept;
        Result<void> parse_function_section(BinaryReader& reader, Module& module) noexcept;
        Result<void> parse_table_section(BinaryReader& reader, Module& module) noexcept;
        Result<void> parse_memory_section(BinaryReader& reader, Module& module) noexcept;
        Result<void> parse_global_section(BinaryReader& reader, Module& module) noexcept;
        Result<void> parse_export_section(BinaryReader& reader, Module& module) noexcept;
        Result<void> parse_start_section(BinaryReader& reader, Module& module) noexcept;
        Result<void> parse_element_section(BinaryReader& reader, Module& module) noexcept;
        Result<void> parse_code_section(BinaryReader& reader, Module& module) noexcept;
        Result<void> parse_data_section(BinaryReader& reader, Module& module) noexcept;
        Result<void> parse_custom_section(BinaryReader& reader, Module& module) noexcept;
    };

    /**
     * @brief Streaming binary parser for large files
     * 
     * This parser can handle very large WebAssembly files by parsing
     * sections on-demand rather than loading everything into memory.
     */
    class StreamingBinaryParser {
    public:
        /**
         * @brief Parse module header and section headers
         */
        static Result<void> parse_headers(span<const uint8_t> data) noexcept;

        /**
         * @brief Get information about sections without parsing content
         */
        struct SectionInfo {
            uint8_t id;
            size_t offset;
            size_t size;
            std::string name; // For custom sections
        };

        static Result<std::vector<SectionInfo>> get_section_info(span<const uint8_t> data) noexcept;

        /**
         * @brief Parse a specific section by index
         */
        static Result<void> parse_section_at(span<const uint8_t> data, size_t section_index, Module& module) noexcept;
    };

} // namespace flight::wasm

#endif // FLIGHT_WASM_BINARY_PARSER_HPP
