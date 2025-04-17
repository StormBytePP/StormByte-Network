#include <StormByte/network/data/compressor/gzip.hxx>

#include <gzip.h>
#include <cryptlib.h>
#include <filters.h>
#include <future>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace StormByte::Network::Data::Compressor;

namespace {
	ExpectedCompressorFutureBuffer CompressHelper(std::span<const std::byte> inputData) noexcept {
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

	ExpectedCompressorFutureBuffer DecompressHelper(std::span<const std::byte> compressedData) noexcept {
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
ExpectedCompressorFutureBuffer Gzip::Compress(const std::string& input) noexcept {
	std::span<const std::byte> dataSpan(reinterpret_cast<const std::byte*>(input.data()), input.size());
	return CompressHelper(dataSpan);
}

ExpectedCompressorFutureBuffer Gzip::Compress(const StormByte::Buffers::Simple& input) noexcept {
	return CompressHelper(input.Data());
}

StormByte::Buffers::Consumer Gzip::Compress(const Buffers::Consumer consumer) noexcept {
    // Create a producer buffer to store the compressed data
    SharedProducerBuffer producer = std::make_shared<StormByte::Buffers::Producer>();

    // Launch a detached thread to handle compression
    std::thread([consumer, producer]() {
        try {
            constexpr size_t chunkSize = 4096; // Define the chunk size for reading from the consumer
            std::vector<uint8_t> inputBuffer(chunkSize);
            std::string compressedString;

            while (true) {
                // Check how many bytes are available in the consumer
                size_t availableBytes = consumer.AvailableBytes();

                if (availableBytes == 0) {
                    if (consumer.IsEoF()) {
                        // If EOF is reached and no bytes are available, mark the producer as EOF
                        *producer << StormByte::Buffers::Status::EoF;
                        break;
                    } else {
                        // Wait for more data to become available
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        continue;
                    }
                }

                // Read the available bytes (up to chunkSize)
                size_t bytesToRead = std::min(availableBytes, chunkSize);
                auto readResult = consumer.Read(bytesToRead);
                if (!readResult.has_value()) {
                    *producer << StormByte::Buffers::Status::Error;
                    break;
                }

                const auto& inputData = readResult.value();

                // Use Crypto++'s Gzip for compression
                CryptoPP::StringSource ss(
                    reinterpret_cast<const uint8_t*>(inputData.data()), inputData.size(), true,
                    new CryptoPP::Gzip(new CryptoPP::StringSink(compressedString), CryptoPP::Gzip::MAX_DEFLATE_LEVEL));

                // Write the compressed data to the producer
                *producer << StormByte::Buffers::Simple(compressedString.data(), compressedString.size());
            }
        } catch (...) {
            *producer << StormByte::Buffers::Status::Error;
        }
    }).detach();

    // Return a consumer buffer for the producer
    return producer->Consumer();
}

// Public Decompress Methods
ExpectedCompressorFutureBuffer Gzip::Decompress(const std::string& input) noexcept {
	std::span<const std::byte> dataSpan(reinterpret_cast<const std::byte*>(input.data()), input.size());
	return DecompressHelper(dataSpan);
}

ExpectedCompressorFutureBuffer Gzip::Decompress(const StormByte::Buffers::Simple& input) noexcept {
	return DecompressHelper(input.Data());
}

StormByte::Buffers::Consumer Gzip::Decompress(const Buffers::Consumer consumer) noexcept {
    // Create a producer buffer to store the decompressed data
    SharedProducerBuffer producer = std::make_shared<StormByte::Buffers::Producer>();

    // Launch a detached thread to handle decompression
    std::thread([consumer, producer]() {
        try {
            constexpr size_t chunkSize = 4096; // Define the chunk size for reading from the consumer
            std::vector<uint8_t> compressedBuffer(chunkSize);
            std::string decompressedString;

            while (true) {
                // Check how many bytes are available in the consumer
                size_t availableBytes = consumer.AvailableBytes();

                if (availableBytes == 0) {
                    if (consumer.IsEoF()) {
                        // If EOF is reached and no bytes are available, mark the producer as EOF
                        *producer << StormByte::Buffers::Status::EoF;
                        break;
                    } else {
                        // Wait for more data to become available
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        continue;
                    }
                }

                // Read the available bytes (up to chunkSize)
                size_t bytesToRead = std::min(availableBytes, chunkSize);
                auto readResult = consumer.Read(bytesToRead);
                if (!readResult.has_value()) {
                    *producer << StormByte::Buffers::Status::Error;
                    break;
                }

                const auto& compressedData = readResult.value();

                // Use Crypto++'s Gunzip for decompression
                CryptoPP::StringSource ss(
                    reinterpret_cast<const uint8_t*>(compressedData.data()), compressedData.size(), true,
                    new CryptoPP::Gunzip(new CryptoPP::StringSink(decompressedString)));

                // Write the decompressed data to the producer
                *producer << StormByte::Buffers::Simple(decompressedString.data(), decompressedString.size());
            }
        } catch (...) {
            *producer << StormByte::Buffers::Status::Error;
        }
    }).detach();

    // Return a consumer buffer for the producer
    return producer->Consumer();
}
