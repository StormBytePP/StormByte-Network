#include <StormByte/network/data/hash/sha512.hxx>
#include <StormByte/test_handlers.h>

#include <thread>

using namespace StormByte::Network::Data::Hash::SHA512;

int TestSHA512HashConsistencyAcrossFormats() {
    const std::string fn_name = "TestSHA512HashConsistencyAcrossFormats";
    const std::string input_data = "DataToHash";

    // Compute hash for a string
    auto hash_string_result = Hash(input_data);
    ASSERT_TRUE(fn_name, hash_string_result.has_value());
    std::string hash_from_string = hash_string_result.value();

    // Compute hash for a Buffer
    StormByte::Buffers::Simple input_buffer;
    input_buffer << input_data;
    auto hash_buffer_result = Hash(input_buffer);
    ASSERT_TRUE(fn_name, hash_buffer_result.has_value());
    std::string hash_from_buffer = hash_buffer_result.value();

    // Validate all hashes are identical
    ASSERT_EQUAL(fn_name, hash_from_string, hash_from_buffer);

    RETURN_TEST(fn_name, 0);
}

int TestSHA512HashCorrectness() {
    const std::string fn_name = "TestSHA512HashCorrectness";
    const std::string input_data = "HashThisString";

    // Correct expected SHA-512 hash value (uppercase, split into two lines for readability)
    const std::string expected_hash = 
        "6D69A62B60C16398A2482B03FB56FB041E5014E3D8E1480833EB8427C3F45910"
        "B5B1ED812EC8C04087C92F47B50016C1495F358DD34E98723795E6E852B92875";

    // Compute hash for the input string
    auto hash_result = Hash(input_data);
    ASSERT_TRUE(fn_name, hash_result.has_value());
    std::string actual_hash = hash_result.value();

    // Validate the hash matches the expected value
    ASSERT_EQUAL(fn_name, expected_hash, actual_hash);

    RETURN_TEST(fn_name, 0);
}

int TestSHA512CollisionResistance() {
    const std::string fn_name = "TestSHA512CollisionResistance";
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

int TestSHA512ProducesDifferentContent() {
    const std::string fn_name = "TestSHA512ProducesDifferentContent";
    const std::string original_data = "Data to hash";

    // Generate the hash
    auto hash_result = Hash(original_data);
    ASSERT_TRUE(fn_name, hash_result.has_value());
    std::string hashed_data = hash_result.value();

    // Verify hashed content is different from original
    ASSERT_NOT_EQUAL(fn_name, original_data, hashed_data);

    RETURN_TEST(fn_name, 0);
}

int TestSHA512HashUsingConsumerProducer() {
    const std::string fn_name = "TestSHA512HashUsingConsumerProducer";
    const std::string input_data = "HashThisString";

    // Expected SHA-512 hash value (from TestSHA512HashCorrectness)
    const std::string expected_hash = 
        "6D69A62B60C16398A2482B03FB56FB041E5014E3D8E1480833EB8427C3F45910"
        "B5B1ED812EC8C04087C92F47B50016C1495F358DD34E98723795E6E852B92875";

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

    result += TestSHA512HashConsistencyAcrossFormats();
    result += TestSHA512HashCorrectness();
    result += TestSHA512CollisionResistance();
    result += TestSHA512ProducesDifferentContent();
    result += TestSHA512HashUsingConsumerProducer();

    if (result == 0) {
        std::cout << "All tests passed!" << std::endl;
    } else {
        std::cout << result << " tests failed." << std::endl;
    }
    return result;
}