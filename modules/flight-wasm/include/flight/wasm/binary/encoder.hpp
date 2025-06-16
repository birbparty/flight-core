#ifndef FLIGHT_WASM_BINARY_ENCODER_HPP
#define FLIGHT_WASM_BINARY_ENCODER_HPP

/**
 * @file encoder.hpp
 * @brief WebAssembly binary format encoding functionality
 * 
 * This header provides forward declarations and basic structure for encoding
 * WebAssembly modules to binary format. Full implementation will be completed
 * in future encoding tasks.
 */

#include <cstdint>
#include <vector>

namespace flight::wasm {

    // Forward declarations
    class Module;
    class Error;
    template<typename T> class Result;
    template<typename T> class span;

    /**
     * @brief Binary writer for encoding WebAssembly binary format
     * 
     * This class provides low-level binary writing functionality with
     * proper buffer management and endianness handling.
     */
    class BinaryWriter {
    public:
        /**
         * @brief Construct a binary writer
         */
        BinaryWriter() = default;

        /**
         * @brief Get the current write position
         */
        size_t position() const noexcept;

        /**
         * @brief Get the total written size
         */
        size_t size() const noexcept;

        /**
         * @brief Get the written data
         */
        const std::vector<uint8_t>& data() const noexcept;

        /**
         * @brief Clear the buffer
         */
        void clear() noexcept;

        /**
         * @brief Reserve buffer capacity
         */
        void reserve(size_t capacity);

        /**
         * @brief Write a single byte
         */
        void write_byte(uint8_t value);

        /**
         * @brief Write multiple bytes
         */
        void write_bytes(span<const uint8_t> data);

        /**
         * @brief Write a 32-bit unsigned integer (little-endian)
         */
        void write_u32(uint32_t value);

        /**
         * @brief Write a 64-bit unsigned integer (little-endian)
         */
        void write_u64(uint64_t value);

        /**
         * @brief Write a 32-bit float (little-endian)
         */
        void write_f32(float value);

        /**
         * @brief Write a 64-bit float (little-endian)
         */
        void write_f64(double value);

        /**
         * @brief Write a LEB128 unsigned integer
         */
        void write_leb128_u32(uint32_t value);

        /**
         * @brief Write a LEB128 signed integer
         */
        void write_leb128_i32(int32_t value);

        /**
         * @brief Write a LEB128 64-bit unsigned integer
         */
        void write_leb128_u64(uint64_t value);

        /**
         * @brief Write a LEB128 64-bit signed integer
         */
        void write_leb128_i64(int64_t value);

        /**
         * @brief Write a UTF-8 string (with length prefix)
         */
        void write_string(const std::string& str);

    private:
        std::vector<uint8_t> buffer_;
    };

    /**
     * @brief WebAssembly binary encoder
     * 
     * High-level encoder that converts Module objects to binary format.
     */
    class BinaryEncoder {
    public:
        /**
         * @brief Encode a WebAssembly module to binary format
         */
        static Result<std::vector<uint8_t>> encode(const Module& module) noexcept;

        /**
         * @brief Encode a WebAssembly module to a file
         */
        static Result<void> encode_to_file(const Module& module, const std::string& filename) noexcept;

        /**
         * @brief Get the encoded size of a module without actually encoding
         */
        static Result<size_t> calculate_size(const Module& module) noexcept;

    private:
        BinaryEncoder() = default;

        // Internal encoding methods (to be implemented)
        Result<void> encode_module(const Module& module, BinaryWriter& writer) noexcept;
        Result<void> encode_header(BinaryWriter& writer) noexcept;
        Result<void> encode_sections(const Module& module, BinaryWriter& writer) noexcept;

        // Section-specific encoders
        Result<void> encode_type_section(const Module& module, BinaryWriter& writer) noexcept;
        Result<void> encode_import_section(const Module& module, BinaryWriter& writer) noexcept;
        Result<void> encode_function_section(const Module& module, BinaryWriter& writer) noexcept;
        Result<void> encode_table_section(const Module& module, BinaryWriter& writer) noexcept;
        Result<void> encode_memory_section(const Module& module, BinaryWriter& writer) noexcept;
        Result<void> encode_global_section(const Module& module, BinaryWriter& writer) noexcept;
        Result<void> encode_export_section(const Module& module, BinaryWriter& writer) noexcept;
        Result<void> encode_start_section(const Module& module, BinaryWriter& writer) noexcept;
        Result<void> encode_element_section(const Module& module, BinaryWriter& writer) noexcept;
        Result<void> encode_code_section(const Module& module, BinaryWriter& writer) noexcept;
        Result<void> encode_data_section(const Module& module, BinaryWriter& writer) noexcept;
        Result<void> encode_custom_sections(const Module& module, BinaryWriter& writer) noexcept;
    };

    /**
     * @brief Streaming binary encoder for large modules
     * 
     * This encoder can handle very large WebAssembly modules by encoding
     * sections on-demand to avoid memory usage spikes.
     */
    class StreamingBinaryEncoder {
    public:
        /**
         * @brief Begin encoding a module
         */
        explicit StreamingBinaryEncoder(const Module& module);

        /**
         * @brief Encode the next chunk of data
         * @return Empty vector when encoding is complete
         */
        Result<std::vector<uint8_t>> encode_chunk(size_t max_chunk_size = 65536) noexcept;

        /**
         * @brief Check if encoding is complete
         */
        bool is_complete() const noexcept;

        /**
         * @brief Get the total encoded size so far
         */
        size_t encoded_size() const noexcept;

    private:
        const Module* module_;
        size_t current_section_ = 0;
        size_t section_offset_ = 0;
        size_t total_encoded_ = 0;
        bool complete_ = false;
    };

} // namespace flight::wasm

#endif // FLIGHT_WASM_BINARY_ENCODER_HPP
