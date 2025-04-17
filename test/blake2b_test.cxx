#include <StormByte/network/data/hash/blake2b.hxx>
#include <StormByte/test_handlers.h>

using namespace StormByte::Network::Data::Hash::Blake2b;

int TestBlake2bHashCorrectness() {
    const std::string fn_name = "TestBlake2bHashCorrectness";
    const std::string input_data = "HashThisString";

    // Correct expected Blake2b hash value (uppercase, split into two lines for readability)
    const std::string expected_hash = 
        "66CCD3A78741E16F894F2FB20045A8678D12B73D9CBA95D3473B1029781D6587"
        "648E839960BDA14F0FF075C0EC9E7ED1AA13197BEED8B027EEA32800453CC7F8";

    // Compute hash for the input string
    auto hash_result = Hash(input_data);
    ASSERT_TRUE(fn_name, hash_result.has_value());
    std::string actual_hash = hash_result.value();

    // Validate the hash matches the expected value
    ASSERT_EQUAL(fn_name, expected_hash, actual_hash);

    RETURN_TEST(fn_name, 0);
}

int TestBlake2bCollisionResistance() {
    const std::string fn_name = "TestBlake2bCollisionResistance";
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

int TestBlake2bProducesDifferentContent() {
    const std::string fn_name = "TestBlake2bProducesDifferentContent";
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

    result += TestBlake2bHashCorrectness();
    result += TestBlake2bCollisionResistance();
    result += TestBlake2bProducesDifferentContent();

    if (result == 0) {
        std::cout << "All tests passed!" << std::endl;
    } else {
        std::cout << result << " tests failed." << std::endl;
    }
    return result;
}