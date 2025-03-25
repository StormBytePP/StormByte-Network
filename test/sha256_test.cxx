#include <StormByte/network/data/hash/sha256.hxx>
#include <StormByte/buffer.hxx>
#include <StormByte/test_handlers.h>
#include <iostream>
#include <string>

using namespace StormByte::Network::Data::Hash::SHA256;

int TestSHA256HashConsistencyAcrossFormats() {
	const std::string fn_name = "TestSHA256HashConsistencyAcrossFormats";
	const std::string input_data = "DataToHash";

	// Compute hash for a string
	auto hash_string_result = Hash(input_data);
	ASSERT_TRUE(fn_name, hash_string_result.has_value());
	std::string hash_from_string = hash_string_result.value();

	// Compute hash for a Buffer
	StormByte::Buffer input_buffer;
	input_buffer << input_data;
	auto hash_buffer_result = Hash(input_buffer);
	ASSERT_TRUE(fn_name, hash_buffer_result.has_value());
	std::string hash_from_buffer = hash_buffer_result.value();

	// Compute hash for a FutureBuffer
	std::promise<StormByte::Buffer> promise;
	promise.set_value(input_buffer);
	auto future_buffer = promise.get_future();
	auto hash_future_result = Hash(future_buffer);
	ASSERT_TRUE(fn_name, hash_future_result.has_value());
	std::string hash_from_future = hash_future_result.value();

	// Validate all hashes are identical
	ASSERT_EQUAL(fn_name, hash_from_string, hash_from_buffer);
	ASSERT_EQUAL(fn_name, hash_from_buffer, hash_from_future);

	RETURN_TEST(fn_name, 0);
}

int TestSHA256HashCorrectness() {
	const std::string fn_name = "TestSHA256HashCorrectness";
	const std::string input_data = "HashThisString";

	// Expected SHA-256 hash value (use an external tool or verified source to precompute the correct hash)
	const std::string expected_hash = "BE767EABA134CB2F01E8D1755A8DD3B18BC8B063049CFF5E6228F5F7143FF777";

	// Compute hash for the input string
	auto hash_result = Hash(input_data);
	ASSERT_TRUE(fn_name, hash_result.has_value());
	std::string actual_hash = hash_result.value();

	// Validate the hash matches the expected value
	ASSERT_EQUAL(fn_name, expected_hash, actual_hash);

	RETURN_TEST(fn_name, 0);
}

int TestSHA256CollisionResistance() {
	const std::string fn_name = "TestSHA256CollisionResistance";
	const std::string input_data_1 = "Original Input Data";
	const std::string input_data_2 = "Original Input Data!"; // Slightly different input

	// Compute hash for input_data_1
	auto hash_result_1 = Hash(input_data_1);
	ASSERT_TRUE(fn_name, hash_result_1.has_value());
	std::string hash_1 = hash_result_1.value();

	// Compute hash for input_data_2
	auto hash_result_2 = Hash(input_data_2);
	ASSERT_TRUE(fn_name, hash_result_2.has_value());
	std::string hash_2 = hash_result_2.value();

	// Ensure the hashes are different
	ASSERT_NOT_EQUAL(fn_name, hash_1, hash_2);

	RETURN_TEST(fn_name, 0);
}

int TestSHA256ProducesDifferentContent() {
	const std::string fn_name = "TestSHA256ProducesDifferentContent";
	const std::string original_data = "Data to hash";

	// Generate the hash
	auto hash_result = Hash(original_data);
	ASSERT_TRUE(fn_name, hash_result.has_value());
	std::string hashed_data = hash_result.value();

	// Verify hashed content is different from original
	ASSERT_NOT_EQUAL(fn_name, original_data, hashed_data);

	RETURN_TEST(fn_name, 0);
}

int main() {
	int result = 0;

	result += TestSHA256HashConsistencyAcrossFormats();
	result += TestSHA256HashCorrectness();
	result += TestSHA256CollisionResistance();
	result += TestSHA256ProducesDifferentContent();

	if (result == 0) {
		std::cout << "All tests passed!" << std::endl;
	} else {
		std::cout << result << " tests failed." << std::endl;
	}
	return result;
}
