#include <StormByte/network/data/compressor/bzip2.hxx>

#include <bzlib.h>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <vector>

using namespace StormByte::Network::Data::Compressor;

namespace {
	ExpectedCompressorBuffer CompressHelper(std::span<const std::byte> inputData, int blockSize = 9) noexcept {
		try {
			unsigned int compressedSize = static_cast<unsigned int>(std::ceil(inputData.size_bytes() * 1.01 + 600));
			std::vector<uint8_t> compressedData(compressedSize);

			if (BZ2_bzBuffToBuffCompress(reinterpret_cast<char*>(compressedData.data()), &compressedSize,
				const_cast<char*>(reinterpret_cast<const char*>(inputData.data())),
				static_cast<unsigned int>(inputData.size_bytes()), blockSize, 0, 30) != BZ_OK) {
				return StormByte::Unexpected<StormByte::Network::CryptoException>("BZip2 compression failed");
			}

			compressedData.resize(compressedSize);
			StormByte::Buffers::Simple buffer(reinterpret_cast<const char*>(compressedData.data()), compressedSize);

			std::promise<StormByte::Buffers::Simple> promise;
			promise.set_value(std::move(buffer));
			return promise.get_future();
		}
		catch (const std::exception& e) {
			return StormByte::Unexpected<StormByte::Network::CryptoException>(e.what());
		}
	}

	ExpectedCompressorBuffer DecompressHelper(std::span<const std::byte> compressedData, size_t originalSize) noexcept {
		try {
			std::vector<uint8_t> decompressedData(originalSize);
			unsigned int decompressedSize = static_cast<unsigned int>(originalSize);

			if (BZ2_bzBuffToBuffDecompress(reinterpret_cast<char*>(decompressedData.data()), &decompressedSize,
				const_cast<char*>(reinterpret_cast<const char*>(compressedData.data())),
				static_cast<unsigned int>(compressedData.size_bytes()), 0, 0) != BZ_OK) {
				return StormByte::Unexpected<StormByte::Network::CryptoException>("BZip2 decompression failed");
			}

			decompressedData.resize(decompressedSize);
			StormByte::Buffers::Simple buffer(reinterpret_cast<const char*>(decompressedData.data()), decompressedSize);

			std::promise<StormByte::Buffers::Simple> promise;
			promise.set_value(std::move(buffer));
			return promise.get_future();
		}
		catch (const std::exception& e) {
			return StormByte::Unexpected<StormByte::Network::CryptoException>(e.what());
		}
	}
}

// Public Compress Methods
ExpectedCompressorBuffer BZip2::Compress(const std::string& input) noexcept {
	std::span<const std::byte> dataSpan(reinterpret_cast<const std::byte*>(input.data()), input.size());
	return CompressHelper(dataSpan);
}

ExpectedCompressorBuffer BZip2::Compress(const StormByte::Buffers::Simple& input) noexcept {
	return CompressHelper(input.Data());
}

ExpectedCompressorBuffer BZip2::Compress(FutureBuffer& bufferFuture) noexcept {
	try {
		StormByte::Buffers::Simple buffer = bufferFuture.get();
		return CompressHelper(buffer.Data());
	}
	catch (const std::exception& e) {
		return StormByte::Unexpected<StormByte::Network::CryptoException>(e.what());
	}
}

// Public Decompress Methods
ExpectedCompressorBuffer BZip2::Decompress(const std::string& input, size_t originalSize) noexcept {
	std::span<const std::byte> dataSpan(reinterpret_cast<const std::byte*>(input.data()), input.size());
	return DecompressHelper(dataSpan, originalSize);
}

ExpectedCompressorBuffer BZip2::Decompress(const StormByte::Buffers::Simple& input, size_t originalSize) noexcept {
	return DecompressHelper(input.Data(), originalSize);
}

ExpectedCompressorBuffer BZip2::Decompress(FutureBuffer& bufferFuture, size_t originalSize) noexcept {
	try {
		StormByte::Buffers::Simple buffer = bufferFuture.get();
		return DecompressHelper(buffer.Data(), originalSize);
	}
	catch (const std::exception& e) {
		return StormByte::Unexpected<StormByte::Network::CryptoException>(e.what());
	}
}