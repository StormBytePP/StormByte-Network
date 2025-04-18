#include <StormByte/network/data/encryption/rsa.hxx>
#include <StormByte/test_handlers.h>

#include <thread>

using namespace StormByte::Network::Data::Encryption;

int TestRSAEncryptDecrypt() {
	const std::string fn_name = "TestRSAEncryptDecrypt";
	const std::string message = "This is a test message.";
	const int key_strength = 2048;

	auto keypair_result = RSA::GenerateKeyPair(key_strength);
	if (!keypair_result.has_value()) {
		RETURN_TEST(fn_name, 1);
	}
	auto [private_key, public_key] = keypair_result.value();

	auto encrypt_result = RSA::Encrypt(message, public_key);
	if (!encrypt_result.has_value()) {
		RETURN_TEST(fn_name, 1);
	}

	auto encrypted_future = std::move(encrypt_result.value());
	StormByte::Buffers::Simple encrypted_buffer = encrypted_future.get();

	auto decrypt_result = RSA::Decrypt(encrypted_buffer, private_key);
	if (!decrypt_result.has_value()) {
		RETURN_TEST(fn_name, 1);
	}
	std::string decrypted_message = decrypt_result.value();

	ASSERT_EQUAL(fn_name, message, decrypted_message);
	RETURN_TEST(fn_name, 0);
}

int TestRSADecryptionWithCorruptedData() {
	const std::string fn_name = "TestRSADecryptionWithCorruptedData";
	const std::string message = "Important message!";
	const int key_strength = 2048;

	auto keypair_result = RSA::GenerateKeyPair(key_strength);
	if (!keypair_result.has_value()) {
		RETURN_TEST(fn_name, 1);
	}
	auto [private_key, public_key] = keypair_result.value();

	auto encrypt_result = RSA::Encrypt(message, public_key);
	if (!encrypt_result.has_value()) {
		RETURN_TEST(fn_name, 1);
	}

	auto encrypted_future = std::move(encrypt_result.value());
	StormByte::Buffers::Simple encrypted_buffer = encrypted_future.get();

	auto corrupted_buffer = encrypted_buffer;
	auto corrupted_span = corrupted_buffer.Span();
	if (!corrupted_span.empty()) {
		corrupted_span[0] = std::byte(static_cast<uint8_t>(~std::to_integer<uint8_t>(corrupted_span[0])));
	} else {
		RETURN_TEST(fn_name, 1);
	}

	auto decrypt_result = RSA::Decrypt(corrupted_buffer, private_key);
	if (!decrypt_result.has_value()) {
		RETURN_TEST(fn_name, 0);
	}

	RETURN_TEST(fn_name, 1);
}

int TestRSADecryptWithMismatchedKey() {
	const std::string fn_name = "TestRSADecryptWithMismatchedKey";
	const std::string message = "Sensitive message.";
	const int key_strength = 2048;

	auto keypair_result_1 = RSA::GenerateKeyPair(key_strength);
	if (!keypair_result_1.has_value()) {
		RETURN_TEST(fn_name, 1);
	}
	auto [private_key_1, public_key_1] = keypair_result_1.value();

	auto keypair_result_2 = RSA::GenerateKeyPair(key_strength);
	if (!keypair_result_2.has_value()) {
		RETURN_TEST(fn_name, 1);
	}
	auto [private_key_2, public_key_2] = keypair_result_2.value();

	auto encrypt_result = RSA::Encrypt(message, public_key_1);
	if (!encrypt_result.has_value()) {
		RETURN_TEST(fn_name, 1);
	}

	auto encrypted_future = std::move(encrypt_result.value());
	StormByte::Buffers::Simple encrypted_buffer = encrypted_future.get();

	auto decrypt_result = RSA::Decrypt(encrypted_buffer, private_key_2);
	if (!decrypt_result.has_value()) {
		RETURN_TEST(fn_name, 0);
	}

	RETURN_TEST(fn_name, 1);
}

int TestRSAWithCorruptedKeys() {
	const std::string fn_name = "TestRSAWithCorruptedKeys";
	const std::string message = "This is a test message.";
	const int key_strength = 2048;

	// Step 1: Generate a valid RSA key pair
	auto keypair_result = RSA::GenerateKeyPair(key_strength);
	if (!keypair_result.has_value()) {
		RETURN_TEST(fn_name, 1);
	}
	auto [private_key, public_key] = keypair_result.value();

	// Step 2: Corrupt the public key
	std::string corrupted_public_key = public_key;
	if (!corrupted_public_key.empty()) {
		corrupted_public_key[0] = static_cast<char>(~corrupted_public_key[0]);
	}

	// Step 3: Corrupt the private key
	std::string corrupted_private_key = private_key;
	if (!corrupted_private_key.empty()) {
		corrupted_private_key[0] = static_cast<char>(~corrupted_private_key[0]);
	}

	// Step 4: Attempt encryption with the corrupted public key
	auto encrypt_result = RSA::Encrypt(message, corrupted_public_key);
	if (encrypt_result.has_value()) {
		std::cerr << "[" << fn_name << "] Encryption unexpectedly succeeded with corrupted public key.\n";
		RETURN_TEST(fn_name, 1);
	}

	// Step 5: Attempt decryption with the corrupted private key
	auto encrypted_future = RSA::Encrypt(message, public_key);
	if (!encrypted_future.has_value()) {
		RETURN_TEST(fn_name, 1); // Encryption with a valid key should not fail
	}

	StormByte::Buffers::Simple encrypted_buffer = std::move(encrypted_future.value().get());
	auto decrypt_result = RSA::Decrypt(encrypted_buffer, corrupted_private_key);
	if (decrypt_result.has_value()) {
		std::cerr << "[" << fn_name << "] Decryption unexpectedly succeeded with corrupted private key.\n";
		RETURN_TEST(fn_name, 1);
	}

	// Step 6: Both operations failed gracefully
	RETURN_TEST(fn_name, 0);
}

int TestRSAEncryptionProducesDifferentContent() {
	const std::string fn_name = "TestRSAEncryptionProducesDifferentContent";
	const std::string original_data = "Sensitive message";
	const int key_strength = 2048;

	auto keypair_result = RSA::GenerateKeyPair(key_strength);
	ASSERT_TRUE(fn_name, keypair_result.has_value());
	auto [private_key, public_key] = keypair_result.value();

	// Encrypt the data
	auto encrypt_result = RSA::Encrypt(original_data, public_key);
	ASSERT_TRUE(fn_name, encrypt_result.has_value());
	auto encrypted_future = std::move(encrypt_result.value());
	StormByte::Buffers::Simple encrypted_buffer = encrypted_future.get();

	// Verify encrypted content is different from original
	ASSERT_NOT_EQUAL(fn_name, original_data, std::string(reinterpret_cast<const char*>(encrypted_buffer.Data().data()), encrypted_buffer.Size()));

	RETURN_TEST(fn_name, 0);
}

int TestRSAEncryptDecryptInOneStep() {
    const std::string fn_name = "TestRSAEncryptDecryptInOneStep";
    const std::string input_data = "This is the data to encrypt and decrypt in one step.";
    const int key_strength = 2048;

    // Generate a key pair
    auto keypair_result = RSA::GenerateKeyPair(key_strength);
    ASSERT_TRUE(fn_name, keypair_result.has_value());
    auto [private_key, public_key] = keypair_result.value();

    // Create a producer buffer and write the input data
    StormByte::Buffers::Producer producer;
    producer << input_data;
    producer << StormByte::Buffers::Status::EoF; // Mark the producer as EOF

    // Create a consumer buffer from the producer
    StormByte::Buffers::Consumer consumer(producer.Consumer());

    // Encrypt the data asynchronously
    auto encrypted_consumer = RSA::Encrypt(consumer, public_key);

    // Decrypt the data asynchronously using the encrypted consumer
    auto decrypted_consumer = RSA::Decrypt(encrypted_consumer, private_key);

    // Wait for the decryption process to complete
    while (!decrypted_consumer.IsEoF()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Read the decrypted data from the decrypted_consumer
    std::string decrypted_data;
    while (true) {
        size_t available_bytes = decrypted_consumer.AvailableBytes();
        if (available_bytes == 0) {
            if (decrypted_consumer.IsEoF()) {
                break; // End of decrypted data
            } else {
                ASSERT_FALSE(fn_name, true); // Unexpected error
            }
        }

        auto read_result = decrypted_consumer.Read(available_bytes);
        if (!read_result.has_value()) {
            ASSERT_FALSE(fn_name, true); // Unexpected error
        }

        const auto& chunk = read_result.value();
        decrypted_data.append(reinterpret_cast<const char*>(chunk.data()), chunk.size());
    }

    // Ensure the decrypted data matches the original input data
    ASSERT_EQUAL(fn_name, input_data, decrypted_data);

    RETURN_TEST(fn_name, 0);
}

int TestRSAEncryptUsingConsumerProducer() {
    const std::string fn_name = "TestRSAEncryptUsingConsumerProducer";
    const std::string input_data = "This is some data to encrypt using the Consumer/Producer model.";
    const int key_strength = 2048;

    // Generate a key pair
    auto keypair_result = RSA::GenerateKeyPair(key_strength);
    ASSERT_TRUE(fn_name, keypair_result.has_value());
    auto [private_key, public_key] = keypair_result.value();

    // Create a producer buffer and write the input data
    StormByte::Buffers::Producer producer;
    producer << input_data;
    producer << StormByte::Buffers::Status::EoF; // Mark the producer as EOF

    // Create a consumer buffer from the producer
    StormByte::Buffers::Consumer consumer(producer.Consumer());

    // Encrypt the data asynchronously
    auto encrypted_consumer = RSA::Encrypt(consumer, public_key);

    // Wait for the encryption process to complete
    while (!encrypted_consumer.IsEoF()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Read the encrypted data from the encrypted_consumer
    std::vector<std::byte> encrypted_data;
    while (true) {
        size_t available_bytes = encrypted_consumer.AvailableBytes();
        if (available_bytes == 0) {
            if (encrypted_consumer.IsEoF()) {
                break; // End of encrypted data
            } else {
                ASSERT_FALSE(fn_name, true); // Unexpected error
            }
        }

        auto read_result = encrypted_consumer.Read(available_bytes);
        if (!read_result.has_value()) {
            ASSERT_FALSE(fn_name, true); // Unexpected error
        }

        const auto& chunk = read_result.value();
        encrypted_data.insert(encrypted_data.end(), chunk.begin(), chunk.end());
    }
    ASSERT_FALSE(fn_name, encrypted_data.empty()); // Ensure encrypted data is not empty

    RETURN_TEST(fn_name, 0);
}

int main() {
	int result = 0;

	result += TestRSAEncryptDecrypt();
	result += TestRSADecryptionWithCorruptedData();
	result += TestRSADecryptWithMismatchedKey();
	result += TestRSAWithCorruptedKeys();
	result += TestRSAEncryptionProducesDifferentContent();
	result += TestRSAEncryptDecryptInOneStep();
	result += TestRSAEncryptUsingConsumerProducer();

	if (result == 0) {
		std::cout << "All tests passed!" << std::endl;
	} else {
		std::cout << result << " tests failed." << std::endl;
	}
	return result;
}