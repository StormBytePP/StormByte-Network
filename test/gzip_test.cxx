#include <StormByte/network/data/compressor/gzip.hxx>
#include <StormByte/test_handlers.h>

#include <thread>

using namespace StormByte::Network::Data::Compressor::Gzip;

int TestGzipCompressConsistencyAcrossFormats() {
	const std::string fn_name = "TestGzipCompressConsistencyAcrossFormats";
	const std::string input_data = "DataToCompress";

	// Compress a string
	auto compress_string_result = Compress(input_data);
	ASSERT_TRUE(fn_name, compress_string_result.has_value());
	auto compressed_from_string_future = std::move(compress_string_result.value());
	StormByte::Buffers::Simple compressed_from_string = compressed_from_string_future.get();

	// Compress a Buffer
	StormByte::Buffers::Simple input_buffer;
	input_buffer << input_data;
	auto compress_buffer_result = Compress(input_buffer);
	ASSERT_TRUE(fn_name, compress_buffer_result.has_value());
	auto compressed_from_buffer_future = std::move(compress_buffer_result.value());
	StormByte::Buffers::Simple compressed_from_buffer = compressed_from_buffer_future.get();

	// Compress a Simple buffer
	auto compress_future_result = Compress(input_buffer);
	ASSERT_TRUE(fn_name, compress_future_result.has_value());
	auto compressed_from_future_future = std::move(compress_future_result.value());
	StormByte::Buffers::Simple compressed_from_future = compressed_from_future_future.get();

	// Validate all compressed outputs are identical in size
	ASSERT_EQUAL(fn_name, compressed_from_string.Size(), compressed_from_buffer.Size());
	ASSERT_EQUAL(fn_name, compressed_from_buffer.Size(), compressed_from_future.Size());

	RETURN_TEST(fn_name, 0);
}

int TestGzipDecompressConsistencyAcrossFormats() {
	const std::string fn_name = "TestGzipDecompressConsistencyAcrossFormats";
	const std::string input_data = "DataToCompressAndDecompress";

	// Compress and then decompress a string
	auto compress_string_result = Compress(input_data);
	ASSERT_TRUE(fn_name, compress_string_result.has_value());
	auto compressed_string_future = std::move(compress_string_result.value());
	StormByte::Buffers::Simple compressed_string = compressed_string_future.get();
	auto decompress_string_result = Decompress(compressed_string);
	ASSERT_TRUE(fn_name, decompress_string_result.has_value());
	auto decompressed_from_string_future = std::move(decompress_string_result.value());
	StormByte::Buffers::Simple decompressed_from_string = decompressed_from_string_future.get();
	std::string decompressed_string_result(reinterpret_cast<const char*>(decompressed_from_string.Data().data()),
										decompressed_from_string.Size());

	// Compress and then decompress a Buffer
	StormByte::Buffers::Simple input_buffer;
	input_buffer << input_data;
	auto compress_buffer_result = Compress(input_buffer);
	ASSERT_TRUE(fn_name, compress_buffer_result.has_value());
	auto compressed_buffer_future = std::move(compress_buffer_result.value());
	StormByte::Buffers::Simple compressed_buffer = compressed_buffer_future.get();
	auto decompress_buffer_result = Decompress(compressed_buffer);
	ASSERT_TRUE(fn_name, decompress_buffer_result.has_value());
	auto decompressed_from_buffer_future = std::move(decompress_buffer_result.value());
	StormByte::Buffers::Simple decompressed_from_buffer = decompressed_from_buffer_future.get();
	std::string decompressed_buffer_result(reinterpret_cast<const char*>(decompressed_from_buffer.Data().data()),
										decompressed_from_buffer.Size());

	// Ensure decompressed results match the original data
	ASSERT_EQUAL(fn_name, input_data, decompressed_string_result);
	ASSERT_EQUAL(fn_name, input_data, decompressed_buffer_result);

	RETURN_TEST(fn_name, 0);
}

int TestGzipCompressionDecompressionIntegrity() {
	const std::string fn_name = "TestGzipCompressionDecompressionIntegrity";
	const std::string input_data = "OriginalDataForIntegrityCheck";

	// Compress the input string
	auto compress_result = Compress(input_data);
	ASSERT_TRUE(fn_name, compress_result.has_value());
	auto compressed_data_future = std::move(compress_result.value());
	StormByte::Buffers::Simple compressed_data = compressed_data_future.get();

	// Decompress the compressed data
	auto decompress_result = Decompress(compressed_data);
	ASSERT_TRUE(fn_name, decompress_result.has_value());
	auto decompressed_data_future = std::move(decompress_result.value());
	StormByte::Buffers::Simple decompressed_data = decompressed_data_future.get();
	std::string decompressed_result(reinterpret_cast<const char*>(decompressed_data.Data().data()),
									decompressed_data.Size());

	// Ensure the decompressed data matches the original input data
	ASSERT_EQUAL(fn_name, input_data, decompressed_result);

	RETURN_TEST(fn_name, 0);
}

int TestGzipCompressionProducesDifferentContent() {
	const std::string fn_name = "TestGzipCompressionProducesDifferentContent";
	const std::string original_data = "Compress this data";

	// Compress the data
	auto compress_result = Compress(original_data);
	ASSERT_TRUE(fn_name, compress_result.has_value());
	auto compressed_future = std::move(compress_result.value());
	StormByte::Buffers::Simple compressed_buffer = compressed_future.get();
	ASSERT_FALSE(fn_name, compressed_buffer.Empty());

	// Verify compressed content is different from original
	ASSERT_NOT_EQUAL(fn_name, original_data, std::string(reinterpret_cast<const char*>(compressed_buffer.Data().data()), compressed_buffer.Size()));

	RETURN_TEST(fn_name, 0);
}

int TestGzipCompressUsingConsumerProducer() {
    const std::string fn_name = "TestGzipCompressUsingConsumerProducer";
    const std::string input_data = "This is some data to compress using the Consumer/Producer model.";

    // Create a producer buffer and write the input data
    StormByte::Buffers::Producer producer;
    producer << input_data;
    producer << StormByte::Buffers::Status::EoF; // Mark the producer as EOF

    // Create a consumer buffer from the producer
    StormByte::Buffers::Consumer consumer(producer.Consumer());

    // Compress the data asynchronously
    auto compressed_consumer = Compress(consumer);

    // Wait for the compression process to complete
    while (!compressed_consumer.IsEoF()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Read the compressed data from the compressed_consumer
    std::vector<std::byte> compressed_data;
    while (true) {
        size_t available_bytes = compressed_consumer.AvailableBytes();
        if (available_bytes == 0) {
            if (compressed_consumer.IsEoF()) {
                break; // End of compressed data
            } else {
                ASSERT_FALSE(fn_name, true); // Unexpected error
            }
        }

        auto read_result = compressed_consumer.Read(available_bytes);
        if (!read_result.has_value()) {
            ASSERT_FALSE(fn_name, true); // Unexpected error
        }

        const auto& chunk = read_result.value();
        compressed_data.insert(compressed_data.end(), chunk.begin(), chunk.end());
    }
    ASSERT_FALSE(fn_name, compressed_data.empty()); // Ensure compressed data is not empty

    RETURN_TEST(fn_name, 0);
}

int TestGzipDecompressUsingConsumerProducer() {
    const std::string fn_name = "TestGzipDecompressUsingConsumerProducer";
    const std::string input_data = "This is some data to compress and decompress using the Consumer/Producer model.";

    // Create a producer buffer and write the input data
    StormByte::Buffers::Producer producer;
    producer << input_data;
    producer << StormByte::Buffers::Status::EoF; // Mark the producer as EOF

    // Create a consumer buffer from the producer
    StormByte::Buffers::Consumer consumer(producer.Consumer());

    // Compress the data asynchronously
    auto compressed_consumer = Compress(consumer);

    // Decompress the data asynchronously
    auto decompressed_consumer = Decompress(compressed_consumer);

    // Wait for the decompression process to complete
    while (!decompressed_consumer.IsEoF()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Read the decompressed data from the decompressed_consumer
    std::string decompressed_data;
    while (true) {
        size_t available_bytes = decompressed_consumer.AvailableBytes();
        if (available_bytes == 0) {
            if (decompressed_consumer.IsEoF()) {
                break; // End of decompressed data
            } else {
                ASSERT_FALSE(fn_name, true); // Unexpected error
            }
        }

        auto read_result = decompressed_consumer.Read(available_bytes);
        if (!read_result.has_value()) {
            ASSERT_FALSE(fn_name, true); // Unexpected error
        }

        const auto& chunk = read_result.value();
        decompressed_data.append(reinterpret_cast<const char*>(chunk.data()), chunk.size());
    }

    // Ensure the decompressed data matches the original input data
    ASSERT_EQUAL(fn_name, input_data, decompressed_data);

    RETURN_TEST(fn_name, 0);
}

int main() {
	int result = 0;

	result += TestGzipCompressConsistencyAcrossFormats();
	result += TestGzipDecompressConsistencyAcrossFormats();
	result += TestGzipCompressionDecompressionIntegrity();
	result += TestGzipCompressionProducesDifferentContent();
	result += TestGzipCompressUsingConsumerProducer();
	result += TestGzipDecompressUsingConsumerProducer();

	if (result == 0) {
		std::cout << "All tests passed!" << std::endl;
	} else {
		std::cout << result << " tests failed." << std::endl;
	}
	return result;
}
