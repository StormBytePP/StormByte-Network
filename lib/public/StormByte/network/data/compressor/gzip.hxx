#pragma once

#include <StormByte/network/data/compressor/typedefs.hxx>

/**
 * @namespace Gzip
 * @brief The namespace containing Gzip compression and decompression functions.
 */
namespace StormByte::Network::Data::Compressor::Gzip {
    /**
     * @brief Compresses the input string using the Gzip compression algorithm.
     * 
     * @param input The input string to compress.
     * @return An `ExpectedCompressorBuffer` containing the compressed data as a future, or an error if compression fails.
     * 
     * @note The compression is performed asynchronously, and the result is returned as a future.
     */
    STORMBYTE_NETWORK_PUBLIC ExpectedCompressorBuffer Compress(const std::string& input) noexcept;

    /**
     * @brief Compresses the input buffer using the Gzip compression algorithm.
     * 
     * @param input The input buffer to compress.
     * @return An `ExpectedCompressorBuffer` containing the compressed data as a future, or an error if compression fails.
     * 
     * @note The compression is performed asynchronously, and the result is returned as a future.
     */
    STORMBYTE_NETWORK_PUBLIC ExpectedCompressorBuffer Compress(const StormByte::Buffers::Simple& input) noexcept;

    /**
     * @brief Decompresses the input string using the Gzip decompression algorithm.
     * 
     * @param input The compressed input string to decompress.
     * @return An `ExpectedCompressorBuffer` containing the decompressed data as a future, or an error if decompression fails.
     * 
     * @note The decompression is performed asynchronously, and the result is returned as a future.
     */
    STORMBYTE_NETWORK_PUBLIC ExpectedCompressorBuffer Decompress(const std::string& input) noexcept;

    /**
     * @brief Decompresses the input buffer using the Gzip decompression algorithm.
     * 
     * @param input The compressed input buffer to decompress.
     * @return An `ExpectedCompressorBuffer` containing the decompressed data as a future, or an error if decompression fails.
     * 
     * @note The decompression is performed asynchronously, and the result is returned as a future.
     */
    STORMBYTE_NETWORK_PUBLIC ExpectedCompressorBuffer Decompress(const StormByte::Buffers::Simple& input) noexcept;
}
