#include <StormByte/network/data/encryption/ecc.hxx>
#include <StormByte/test_handlers.h>

using namespace StormByte::Network::Data::Encryption;

int TestECCEncryptDecrypt() {
	const std::string fn_name = "TestECCEncryptDecrypt";
	const std::string message = "This is a test message.";

	auto keypair_result = ECC::GenerateKeyPair();
	if (!keypair_result.has_value()) {
		RETURN_TEST(fn_name, 1);
	}
	auto [private_key, public_key] = keypair_result.value();

	auto encrypt_result = ECC::Encrypt(message, public_key);
	if (!encrypt_result.has_value()) {
		RETURN_TEST(fn_name, 1);
	}

	auto encrypted_future = std::move(encrypt_result.value());
	StormByte::Buffers::Simple encrypted_buffer = encrypted_future.get();

	auto decrypt_result = ECC::Decrypt(encrypted_buffer, private_key);
	if (!decrypt_result.has_value()) {
		RETURN_TEST(fn_name, 1);
	}
	std::string decrypted_message = decrypt_result.value();

	ASSERT_EQUAL(fn_name, message, decrypted_message);
	RETURN_TEST(fn_name, 0);
}

int TestECCDecryptionWithCorruptedData() {
	const std::string fn_name = "TestECCDecryptionWithCorruptedData";
	const std::string message = "Important message!";

	auto keypair_result = ECC::GenerateKeyPair();
	if (!keypair_result.has_value()) {
		RETURN_TEST(fn_name, 1);
	}
	auto [private_key, public_key] = keypair_result.value();

	auto encrypt_result = ECC::Encrypt(message, public_key);
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

	auto decrypt_result = ECC::Decrypt(corrupted_buffer, private_key);
	if (!decrypt_result.has_value()) {
		RETURN_TEST(fn_name, 0);
	}

	RETURN_TEST(fn_name, 1);
}

int TestECCDecryptWithMismatchedKey() {
	const std::string fn_name = "TestECCDecryptWithMismatchedKey";
	const std::string message = "Sensitive message.";

	auto keypair_result_1 = ECC::GenerateKeyPair();
	if (!keypair_result_1.has_value()) {
		RETURN_TEST(fn_name, 1);
	}
	auto [private_key_1, public_key_1] = keypair_result_1.value();

	auto keypair_result_2 = ECC::GenerateKeyPair();
	if (!keypair_result_2.has_value()) {
		RETURN_TEST(fn_name, 1);
	}
	auto [private_key_2, public_key_2] = keypair_result_2.value();

	auto encrypt_result = ECC::Encrypt(message, public_key_1);
	if (!encrypt_result.has_value()) {
		RETURN_TEST(fn_name, 1);
	}

	auto encrypted_future = std::move(encrypt_result.value());
	StormByte::Buffers::Simple encrypted_buffer = encrypted_future.get();

	auto decrypt_result = ECC::Decrypt(encrypted_buffer, private_key_2);
	if (!decrypt_result.has_value()) {
		RETURN_TEST(fn_name, 0);
	}

	RETURN_TEST(fn_name, 1);
}

int TestECCWithCorruptedKeys() {
	const std::string fn_name = "TestECCWithCorruptedKeys";
	const std::string message = "This is a test message.";

	// Step 1: Generate a valid key pair
	auto keypair_result = ECC::GenerateKeyPair();
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
	auto encrypt_result = ECC::Encrypt(message, corrupted_public_key);
	if (encrypt_result.has_value()) {
		std::cerr << "[" << fn_name << "] Encryption unexpectedly succeeded with corrupted public key.\n";
		RETURN_TEST(fn_name, 1);
	}

	// Step 5: Attempt decryption with the corrupted private key
	auto encrypted_future = ECC::Encrypt(message, public_key);
	if (!encrypted_future.has_value()) {
		RETURN_TEST(fn_name, 1); // Encryption with a valid key should not fail
	}

	StormByte::Buffers::Simple encrypted_buffer = std::move(encrypted_future.value().get());
	auto decrypt_result = ECC::Decrypt(encrypted_buffer, corrupted_private_key);
	if (decrypt_result.has_value()) {
		std::cerr << "[" << fn_name << "] Decryption unexpectedly succeeded with corrupted private key.\n";
		RETURN_TEST(fn_name, 1);
	}

	// Step 6: Both operations failed gracefully
	RETURN_TEST(fn_name, 0);
}

int TestECCEncryptionProducesDifferentContent() {
	const std::string fn_name = "TestECCEncryptionProducesDifferentContent";
	const std::string original_data = "ECC test message";

	auto keypair_result = ECC::GenerateKeyPair();
	ASSERT_TRUE(fn_name, keypair_result.has_value());
	auto [private_key, public_key] = keypair_result.value();

	// Encrypt the data
	auto encrypt_result = ECC::Encrypt(original_data, public_key);
	ASSERT_TRUE(fn_name, encrypt_result.has_value());
	auto encrypted_future = std::move(encrypt_result.value());
	StormByte::Buffers::Simple encrypted_buffer = encrypted_future.get();

	// Verify encrypted content is different from original
	ASSERT_NOT_EQUAL(fn_name, original_data, std::string(reinterpret_cast<const char*>(encrypted_buffer.Data().data()), encrypted_buffer.Size()));

	RETURN_TEST(fn_name, 0);
}

int main() {
	int result = 0;

	result += TestECCEncryptDecrypt();
	result += TestECCDecryptionWithCorruptedData();
	result += TestECCDecryptWithMismatchedKey();
	result += TestECCWithCorruptedKeys();
	result += TestECCEncryptionProducesDifferentContent();

	if (result == 0) {
		std::cout << "All tests passed!" << std::endl;
	} else {
		std::cout << result << " tests failed." << std::endl;
	}
	return result;
}
