#pragma once

#include <StormByte/network/data/compressor/typedefs.hxx>

namespace StormByte::Network::Data::Compressor::BZip2 {
	STORMBYTE_NETWORK_PUBLIC ExpectedCompressorBuffer Compress(const std::string& input) noexcept;
	STORMBYTE_NETWORK_PUBLIC ExpectedCompressorBuffer Compress(const StormByte::Buffers::Simple& input) noexcept;
	STORMBYTE_NETWORK_PUBLIC ExpectedCompressorBuffer Compress(FutureBuffer& bufferFuture) noexcept;

	STORMBYTE_NETWORK_PUBLIC ExpectedCompressorBuffer Decompress(const std::string& input, size_t originalSize) noexcept;
	STORMBYTE_NETWORK_PUBLIC ExpectedCompressorBuffer Decompress(const StormByte::Buffers::Simple& input, size_t originalSize) noexcept;
	STORMBYTE_NETWORK_PUBLIC ExpectedCompressorBuffer Decompress(FutureBuffer& bufferFuture, size_t originalSize) noexcept;
}