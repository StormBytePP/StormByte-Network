#pragma once

#include <StormByte/network/data/compressor/typedefs.hxx>

/**
 * @namespace BZip2
 * @brief The namespace containing BZip2 compression functions.
 */
namespace StormByte::Network::Data::Compressor::BZip2 {
	/**
	 * @brief Compresses the input string using the BZip2 compression algorithm.
	 * 
	 * @param input The input string to compress.
	 * @return An `ExpectedCompressorFutureBuffer` containing the compressed data as a future, or an error if compression fails.
	 * 
	 * @note The compression is performed asynchronously, and the result is returned as a future.
	 */
	STORMBYTE_NETWORK_PUBLIC ExpectedCompressorFutureBuffer Compress(const std::string& input) noexcept;

	/**
	 * @brief Compresses the input buffer using the BZip2 compression algorithm.
	 * 
	 * @param input The input buffer to compress.
	 * @return An `ExpectedCompressorFutureBuffer` containing the compressed data as a future, or an error if compression fails.
	 * 
	 * @note The compression is performed asynchronously, and the result is returned as a future.
	 */
	STORMBYTE_NETWORK_PUBLIC ExpectedCompressorFutureBuffer Compress(const StormByte::Buffers::Simple& input) noexcept;

	/**
	 * @brief Decompresses the input string using the BZip2 decompression algorithm.
	 * 
	 * @param input The compressed input string to decompress.
	 * @param originalSize The expected size of the decompressed data.
	 * @return An `ExpectedCompressorFutureBuffer` containing the decompressed data as a future, or an error if decompression fails.
	 * 
	 * @note The decompression is performed asynchronously, and the result is returned as a future.
	 *       The `originalSize` parameter must match the size of the original uncompressed data.
	 */
	STORMBYTE_NETWORK_PUBLIC ExpectedCompressorFutureBuffer Decompress(const std::string& input, size_t originalSize) noexcept;

	/**
	 * @brief Decompresses the input buffer using the BZip2 decompression algorithm.
	 * 
	 * @param input The compressed input buffer to decompress.
	 * @param originalSize The expected size of the decompressed data.
	 * @return An `ExpectedCompressorFutureBuffer` containing the decompressed data as a future, or an error if decompression fails.
	 * 
	 * @note The decompression is performed asynchronously, and the result is returned as a future.
	 *       The `originalSize` parameter must match the size of the original uncompressed data.
	 */
	STORMBYTE_NETWORK_PUBLIC ExpectedCompressorFutureBuffer Decompress(const StormByte::Buffers::Simple& input, size_t originalSize) noexcept;
}