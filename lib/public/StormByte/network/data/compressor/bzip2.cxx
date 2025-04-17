#include <StormByte/network/data/compressor/bzip2.hxx>

#include <bzlib.h>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace StormByte::Network::Data::Compressor;

namespace {
    ExpectedCompressorFutureBuffer CompressHelper(std::span<const std::byte> inputData, int blockSize = 9) noexcept {
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

    ExpectedCompressorFutureBuffer DecompressHelper(std::span<const std::byte> compressedData, size_t originalSize) noexcept {
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
ExpectedCompressorFutureBuffer BZip2::Compress(const std::string& input) noexcept {
    std::span<const std::byte> dataSpan(reinterpret_cast<const std::byte*>(input.data()), input.size());
    return CompressHelper(dataSpan);
}

ExpectedCompressorFutureBuffer BZip2::Compress(const StormByte::Buffers::Simple& input) noexcept {
    return CompressHelper(input.Data());
}

StormByte::Buffers::Consumer BZip2::Compress(const Buffers::Consumer consumer) noexcept {
    // Create a producer buffer to store the compressed data
    SharedProducerBuffer producer = std::make_shared<StormByte::Buffers::Producer>();

    // Launch a detached thread to handle compression
    std::thread([consumer, producer]() {
        try {
            constexpr size_t chunkSize = 4096; // Define the chunk size for reading from the consumer
            std::vector<uint8_t> inputBuffer(chunkSize);
            std::vector<uint8_t> compressedBuffer(chunkSize * 2); // Allocate a larger buffer for compressed data

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
                    // If reading fails, mark the producer as Error
                    *producer << StormByte::Buffers::Status::Error;
                    break;
                }

                const auto& inputData = readResult.value();

                if (inputData.empty()) {
                    // No more data to read, mark the producer as EOF
                    *producer << StormByte::Buffers::Status::EoF;
                    break;
                }

                // Compress the chunk
                unsigned int compressedSize = static_cast<unsigned int>(std::ceil(inputData.size() * 1.01 + 600));
                compressedBuffer.resize(compressedSize);

                int result = BZ2_bzBuffToBuffCompress(
                    reinterpret_cast<char*>(compressedBuffer.data()), &compressedSize,
                    const_cast<char*>(reinterpret_cast<const char*>(inputData.data())),
                    static_cast<unsigned int>(inputData.size()), 9, 0, 30);

                if (result != BZ_OK) {
                    // Compression failed, mark the producer as Error
                    *producer << StormByte::Buffers::Status::Error;
                    break;
                }

                // Resize the compressed buffer to the actual size
                compressedBuffer.resize(compressedSize);

                // Write the compressed data to the producer
                *producer << StormByte::Buffers::Simple(reinterpret_cast<const char*>(compressedBuffer.data()), compressedSize);
            }
        } catch (...) {
            // Handle any unexpected exceptions and mark the producer as Error
            *producer << StormByte::Buffers::Status::Error;
        }
    }).detach();

    // Return a consumer buffer for the producer
    return producer->Consumer();
}

// Public Decompress Methods
ExpectedCompressorFutureBuffer BZip2::Decompress(const std::string& input, size_t originalSize) noexcept {
    std::span<const std::byte> dataSpan(reinterpret_cast<const std::byte*>(input.data()), input.size());
    return DecompressHelper(dataSpan, originalSize);
}

ExpectedCompressorFutureBuffer BZip2::Decompress(const StormByte::Buffers::Simple& input, size_t originalSize) noexcept {
    return DecompressHelper(input.Data(), originalSize);
}

StormByte::Buffers::Consumer BZip2::Decompress(const Buffers::Consumer consumer, size_t originalSize) noexcept {
    // Create a producer buffer to store the decompressed data
    SharedProducerBuffer producer = std::make_shared<StormByte::Buffers::Producer>();

    // Launch a detached thread to handle decompression
    std::thread([consumer, producer, originalSize]() {
        try {
            constexpr size_t chunkSize = 4096; // Define the chunk size for reading from the consumer
            std::vector<uint8_t> compressedBuffer(chunkSize);
            std::vector<uint8_t> decompressedBuffer(originalSize); // Allocate a buffer for decompressed data

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
                    // If reading fails, mark the producer as Error
                    *producer << StormByte::Buffers::Status::Error;
                    break;
                }

                const auto& compressedData = readResult.value();

                if (compressedData.empty()) {
                    // No more data to read, mark the producer as EOF
                    *producer << StormByte::Buffers::Status::EoF;
                    break;
                }

                // Decompress the chunk
                unsigned int decompressedSize = static_cast<unsigned int>(originalSize);
                decompressedBuffer.resize(decompressedSize);

                int result = BZ2_bzBuffToBuffDecompress(
                    reinterpret_cast<char*>(decompressedBuffer.data()), &decompressedSize,
                    const_cast<char*>(reinterpret_cast<const char*>(compressedData.data())),
                    static_cast<unsigned int>(compressedData.size()), 0, 0);

                if (result != BZ_OK) {
                    // Decompression failed, mark the producer as Error
                    *producer << StormByte::Buffers::Status::Error;
                    break;
                }

                // Resize the decompressed buffer to the actual size
                decompressedBuffer.resize(decompressedSize);

                // Write the decompressed data to the producer
                *producer << StormByte::Buffers::Simple(reinterpret_cast<const char*>(decompressedBuffer.data()), decompressedSize);
            }
        } catch (...) {
            // Handle any unexpected exceptions and mark the producer as Error
            *producer << StormByte::Buffers::Status::Error;
        }
    }).detach();

    // Return a consumer buffer for the producer
    return producer->Consumer();
}
