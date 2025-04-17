#include <StormByte/network/data/compressor/gzip.hxx>

#include <gzip.h>
#include <cryptlib.h>
#include <filters.h>
#include <future>
#include <stdexcept>
#include <vector>

using namespace StormByte::Network::Data::Compressor;

namespace {
	ExpectedCompressorBuffer CompressHelper(std::span<const std::byte> inputData) noexcept {
		try {
			std::string compressedString;

			// Use Crypto++'s Gzip for compression
			CryptoPP::StringSource ss(
				reinterpret_cast<const uint8_t*>(inputData.data()), inputData.size_bytes(), true,
				new CryptoPP::Gzip(
					new CryptoPP::StringSink(compressedString), CryptoPP::Gzip::MAX_DEFLATE_LEVEL));

			// Convert the compressed string to std::vector<std::byte>
			std::vector<std::byte> compressedData(compressedString.size());
			std::transform(compressedString.begin(), compressedString.end(), compressedData.begin(),
				[](char c) { return static_cast<std::byte>(c); });

			// Create Buffer from std::vector<std::byte>
			StormByte::Buffers::Simple buffer(std::move(compressedData));

			// Wrap the result into a promise and future
			std::promise<StormByte::Buffers::Simple> promise;
			promise.set_value(std::move(buffer));
			return promise.get_future();
		}
		catch (const CryptoPP::Exception& e) {
			return StormByte::Unexpected<StormByte::Network::CryptoException>(e.what());
		}
	}

	ExpectedCompressorBuffer DecompressHelper(std::span<const std::byte> compressedData) noexcept {
		try {
			std::string decompressedString;

			// Use Crypto++'s Gunzip for decompression
			CryptoPP::StringSource ss(
				reinterpret_cast<const uint8_t*>(compressedData.data()), compressedData.size_bytes(), true,
				new CryptoPP::Gunzip(new CryptoPP::StringSink(decompressedString)));

			// Convert the decompressed string to std::vector<std::byte>
			std::vector<std::byte> decompressedData(decompressedString.size());
			std::transform(decompressedString.begin(), decompressedString.end(), decompressedData.begin(),
				[](char c) { return static_cast<std::byte>(c); });

			// Create Buffer from std::vector<std::byte>
			StormByte::Buffers::Simple buffer(std::move(decompressedData));

			// Wrap the result into a promise and future
			std::promise<StormByte::Buffers::Simple> promise;
			promise.set_value(std::move(buffer));
			return promise.get_future();
		}
		catch (const CryptoPP::Exception& e) {
			return StormByte::Unexpected<StormByte::Network::CryptoException>(e.what());
		}
	}
}

// Public Compress Methods
ExpectedCompressorBuffer Gzip::Compress(const std::string& input) noexcept {
	std::span<const std::byte> dataSpan(reinterpret_cast<const std::byte*>(input.data()), input.size());
	return CompressHelper(dataSpan);
}

ExpectedCompressorBuffer Gzip::Compress(const StormByte::Buffers::Simple& input) noexcept {
	return CompressHelper(input.Data());
}

ExpectedCompressorBuffer Gzip::Compress(FutureBuffer& bufferFuture) noexcept {
	try {
		StormByte::Buffers::Simple buffer = bufferFuture.get();
		return CompressHelper(buffer.Data());
	} catch (const std::exception& e) {
		return StormByte::Unexpected<StormByte::Network::CryptoException>(e.what());
	}
}

// Public Decompress Methods
ExpectedCompressorBuffer Gzip::Decompress(const std::string& input) noexcept {
	std::span<const std::byte> dataSpan(reinterpret_cast<const std::byte*>(input.data()), input.size());
	return DecompressHelper(dataSpan);
}

ExpectedCompressorBuffer Gzip::Decompress(const StormByte::Buffers::Simple& input) noexcept {
	return DecompressHelper(input.Data());
}

ExpectedCompressorBuffer Gzip::Decompress(FutureBuffer& bufferFuture) noexcept {
	try {
		StormByte::Buffers::Simple buffer = bufferFuture.get();
		return DecompressHelper(buffer.Data());
	} catch (const std::exception& e) {
		return StormByte::Unexpected<StormByte::Network::CryptoException>(e.what());
	}
}
