#include <StormByte/network/data/encryption/dsa.hxx>
#include <StormByte/test_handlers.h>

#include <thread>
#include <iostream>

using namespace StormByte::Network::Data::Encryption;

int TestDSASignAndVerify() {
    const std::string fn_name = "TestDSASignAndVerify";
    const std::string message = "This is a test message.";
    const int key_strength = 2048;

    // Generate a key pair
    auto keypair_result = DSA::GenerateKeyPair(key_strength);
    ASSERT_TRUE(fn_name, keypair_result.has_value());
    auto [private_key, public_key] = keypair_result.value();

    // Sign the message
    auto sign_result = DSA::Sign(message, private_key);
    ASSERT_TRUE(fn_name, sign_result.has_value());
    std::string signature = sign_result.value();

    // Verify the signature
    bool verify_result = DSA::Verify(message, signature, public_key);
    ASSERT_TRUE(fn_name, verify_result);

    RETURN_TEST(fn_name, 0);
}

int TestDSAVerifyWithCorruptedSignature() {
    const std::string fn_name = "TestDSAVerifyWithCorruptedSignature";
    const std::string message = "This is a test message.";
    const int key_strength = 2048;

    // Generate a key pair
    auto keypair_result = DSA::GenerateKeyPair(key_strength);
    ASSERT_TRUE(fn_name, keypair_result.has_value());
    auto [private_key, public_key] = keypair_result.value();

    // Sign the message
    auto sign_result = DSA::Sign(message, private_key);
    ASSERT_TRUE(fn_name, sign_result.has_value());
    std::string signature = sign_result.value();

    // Corrupt the signature
    if (!signature.empty()) {
        signature[0] = static_cast<char>(~signature[0]);
    }

    // Verify the corrupted signature
    bool verify_result = DSA::Verify(message, signature, public_key);
    ASSERT_FALSE(fn_name, verify_result);

    RETURN_TEST(fn_name, 0);
}

int TestDSAVerifyWithMismatchedKey() {
    const std::string fn_name = "TestDSAVerifyWithMismatchedKey";
    const std::string message = "This is a test message.";
    const int key_strength = 2048;

    // Generate two key pairs
    auto keypair_result_1 = DSA::GenerateKeyPair(key_strength);
    ASSERT_TRUE(fn_name, keypair_result_1.has_value());
    auto [private_key_1, public_key_1] = keypair_result_1.value();

    auto keypair_result_2 = DSA::GenerateKeyPair(key_strength);
    ASSERT_TRUE(fn_name, keypair_result_2.has_value());
    auto [private_key_2, public_key_2] = keypair_result_2.value();

    // Sign the message with the first private key
    auto sign_result = DSA::Sign(message, private_key_1);
    ASSERT_TRUE(fn_name, sign_result.has_value());
    std::string signature = sign_result.value();

    // Verify the signature with the second public key
    bool verify_result = DSA::Verify(message, signature, public_key_2);
    ASSERT_FALSE(fn_name, verify_result);

    RETURN_TEST(fn_name, 0);
}

int main() {
    int result = 0;

    result += TestDSASignAndVerify();
    result += TestDSAVerifyWithCorruptedSignature();
    result += TestDSAVerifyWithMismatchedKey();

	if (result == 0) {
        std::cout << "All tests passed!" << std::endl;
    } else {
        std::cout << result << " tests failed." << std::endl;
    }
    return result;
}