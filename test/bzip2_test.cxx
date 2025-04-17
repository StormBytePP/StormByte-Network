#include <StormByte/network/data/compressor/bzip2.hxx>
#include <StormByte/test_handlers.h>

using namespace StormByte::Network::Data::Compressor::BZip2;

int TestBZip2CompressConsistencyAcrossFormats() {
	const std::string fn_name = "TestBZip2CompressConsistencyAcrossFormats";
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

int TestBZip2DecompressConsistencyAcrossFormats() {
	const std::string fn_name = "TestBZip2DecompressConsistencyAcrossFormats";
	const std::string input_data = "DataToCompressAndDecompress";

	// Compress and then decompress a string
	auto compress_string_result = Compress(input_data);
	ASSERT_TRUE(fn_name, compress_string_result.has_value());
	auto compressed_string_future = std::move(compress_string_result.value());
	StormByte::Buffers::Simple compressed_string = compressed_string_future.get();
	auto decompress_string_result = Decompress(compressed_string, input_data.size());
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
	auto decompress_buffer_result = Decompress(compressed_buffer, input_data.size());
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

int TestBZip2CompressionDecompressionIntegrity() {
	const std::string fn_name = "TestBZip2CompressionDecompressionIntegrity";
	const std::string input_data = "OriginalDataForIntegrityCheck";

	// Compress the input string
	auto compress_result = Compress(input_data);
	ASSERT_TRUE(fn_name, compress_result.has_value());
	auto compressed_data_future = std::move(compress_result.value());
	StormByte::Buffers::Simple compressed_data = compressed_data_future.get();

	// Decompress the compressed data
	auto decompress_result = Decompress(compressed_data, input_data.size());
	ASSERT_TRUE(fn_name, decompress_result.has_value());
	auto decompressed_data_future = std::move(decompress_result.value());
	StormByte::Buffers::Simple decompressed_data = decompressed_data_future.get();
	std::string decompressed_result(reinterpret_cast<const char*>(decompressed_data.Data().data()),
									decompressed_data.Size());

	// Ensure the decompressed data matches the original input data
	ASSERT_EQUAL(fn_name, input_data, decompressed_result);

	RETURN_TEST(fn_name, 0);
}

int TestBzip2CompressionProducesDifferentContent() {
	const std::string fn_name = "TestBzip2CompressionProducesDifferentContent";
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

int TestBZip2DecompressCorruptedData() {
    const std::string fn_name = "TestBZip2DecompressCorruptedData";

    // Original valid data
    const std::string original_data = "This is some valid data to compress and corrupt.";

    // Compress the valid data
    auto compress_result = Compress(original_data);
    ASSERT_TRUE(fn_name, compress_result.has_value());
    auto compressed_future = std::move(compress_result.value());
    StormByte::Buffers::Simple compressed_buffer = compressed_future.get();
    ASSERT_FALSE(fn_name, compressed_buffer.Empty());

    // Flip a single bit in the compressed data to simulate corruption
    std::vector<std::byte> corrupted_data = compressed_buffer.Data();
    corrupted_data[0] ^= std::byte(0x01); // Flip the least significant bit of the first byte

    // Wrap the corrupted data in a StormByte::Buffers::Simple buffer
    StormByte::Buffers::Simple corrupted_buffer;
    corrupted_buffer << corrupted_data;

    // Attempt to decompress the corrupted data
    auto decompress_result = Decompress(corrupted_buffer, original_data.size());
    ASSERT_FALSE(fn_name, decompress_result.has_value()); // Expect decompression to fail

    RETURN_TEST(fn_name, 0);
}

int main() {
	int result = 0;

	result += TestBZip2CompressConsistencyAcrossFormats();
	result += TestBZip2DecompressConsistencyAcrossFormats();
	result += TestBZip2CompressionDecompressionIntegrity();
	result += TestBzip2CompressionProducesDifferentContent();
	result += TestBZip2DecompressCorruptedData();

	if (result == 0) {
		std::cout << "All tests passed!" << std::endl;
	} else {
		std::cout << result << " tests failed." << std::endl;
	}
	return result;
}
