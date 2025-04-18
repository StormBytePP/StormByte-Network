#include <StormByte/network/data/hash/blake2s.hxx>
#include <StormByte/test_handlers.h>

#include <thread>

using namespace StormByte::Network::Data::Hash::Blake2s;

int TestBlake2sHashCorrectness() {
    const std::string fn_name = "TestBlake2sHashCorrectness";
    const std::string input_data = "HashThisString";

    // Correct expected Blake2s hash value (uppercase, split into two lines for readability)
    const std::string expected_hash = 
        "542412C16951C1538BDCB190255C363F7FA1986B500FFBAB8377EF5E67785F1D";

    // Compute hash for the input string
    auto hash_result = Hash(input_data);
    ASSERT_TRUE(fn_name, hash_result.has_value());
    std::string actual_hash = hash_result.value();

    // Validate the hash matches the expected value
    ASSERT_EQUAL(fn_name, expected_hash, actual_hash);

    RETURN_TEST(fn_name, 0);
}

int TestBlake2sCollisionResistance() {
    const std::string fn_name = "TestBlake2sCollisionResistance";
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

int TestBlake2sProducesDifferentContent() {
    const std::string fn_name = "TestBlake2sProducesDifferentContent";
    const std::string original_data = "Data to hash";

    // Generate the hash
    auto hash_result = Hash(original_data);
    ASSERT_TRUE(fn_name, hash_result.has_value());
    std::string hashed_data = hash_result.value();

    // Verify hashed content is different from original
    ASSERT_NOT_EQUAL(fn_name, original_data, hashed_data);

    RETURN_TEST(fn_name, 0);
}

int TestBlake2sHashUsingConsumerProducer() {
    const std::string fn_name = "TestBlake2sHashUsingConsumerProducer";
    const std::string input_data = "HashThisString";

    // Expected Blake2s hash value (from TestBlake2sHashCorrectness)
    const std::string expected_hash = "542412C16951C1538BDCB190255C363F7FA1986B500FFBAB8377EF5E67785F1D";

    // Create a producer buffer and write the input data
    StormByte::Buffers::Producer producer;
    producer << input_data;
    producer << StormByte::Buffers::Status::EoF; // Mark the producer as EOF

    // Create a consumer buffer from the producer
    StormByte::Buffers::Consumer consumer(producer.Consumer());

    // Hash the data asynchronously
    auto hash_consumer = Hash(consumer);

    // Wait for the hashing process to complete
    while (!hash_consumer.IsEoF()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Read the hash result from the hash_consumer
    std::string hash_result;
    while (true) {
        size_t available_bytes = hash_consumer.AvailableBytes();
        if (available_bytes == 0) {
            if (hash_consumer.IsEoF()) {
                break; // End of hash result
            } else {
                ASSERT_FALSE(fn_name, true); // Unexpected error
            }
        }

        auto read_result = hash_consumer.Read(available_bytes);
        if (!read_result.has_value()) {
            ASSERT_FALSE(fn_name, true); // Unexpected error
        }

        const auto& chunk = read_result.value();
        hash_result.append(reinterpret_cast<const char*>(chunk.data()), chunk.size());
    }

    // Ensure the hash result matches the expected hash
    ASSERT_EQUAL(fn_name, expected_hash, hash_result);

    RETURN_TEST(fn_name, 0);
}

int main() {
    int result = 0;

    result += TestBlake2sHashCorrectness();
    result += TestBlake2sCollisionResistance();
    result += TestBlake2sProducesDifferentContent();
    result += TestBlake2sHashUsingConsumerProducer();

    if (result == 0) {
        std::cout << "All tests passed!" << std::endl;
    } else {
        std::cout << result << " tests failed." << std::endl;
    }
    return result;
}