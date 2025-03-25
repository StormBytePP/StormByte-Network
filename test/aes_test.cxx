#include <StormByte/network/data/encryption/aes.hxx>
#include <StormByte/util/buffer.hxx>
#include <StormByte/test_handlers.h>
#include <iostream>
#include <string>

using namespace StormByte::Network::Data::Encryption;

int TestAESEncryptDecryptConsistency() {
	const std::string fn_name = "TestAESEncryptDecryptConsistency";
	const std::string password = "SecurePassword123!";
	const std::string original_data = "Confidential information to encrypt and decrypt.";

	// Encrypt the data
	auto encrypt_result = AES::Encrypt(original_data, password);
	ASSERT_TRUE(fn_name, encrypt_result.has_value());

	auto encrypted_future = std::move(encrypt_result.value());
	StormByte::Util::Buffer encrypted_buffer = encrypted_future.get();
	ASSERT_FALSE(fn_name, encrypted_buffer.Empty());

	// Decrypt the data
	auto decrypt_result = AES::Decrypt(encrypted_buffer, password);
	ASSERT_TRUE(fn_name, decrypt_result.has_value());

	auto decrypted_future = std::move(decrypt_result.value());
	StormByte::Util::Buffer decrypted_buffer = decrypted_future.get();
	ASSERT_FALSE(fn_name, decrypted_buffer.Empty());

	// Validate decrypted data matches the original data
	std::string decrypted_data(reinterpret_cast<const char*>(decrypted_buffer.Data().data()), decrypted_buffer.Size());
	ASSERT_EQUAL(fn_name, original_data, decrypted_data);

	RETURN_TEST(fn_name, 0);
}

int TestAESWrongDecryptionPassword() {
	const std::string fn_name = "TestAESWrongDecryptionPassword";
	const std::string password = "SecurePassword123!";
	const std::string wrong_password = "WrongPassword456!";
	const std::string original_data = "This is sensitive data.";

	// Encrypt the data
	auto encrypt_result = AES::Encrypt(original_data, password);
	ASSERT_TRUE(fn_name, encrypt_result.has_value());

	auto encrypted_future = std::move(encrypt_result.value());
	StormByte::Util::Buffer encrypted_buffer = encrypted_future.get();
	ASSERT_FALSE(fn_name, encrypted_buffer.Empty());

	// Attempt to decrypt with a wrong password
	auto decrypt_result = AES::Decrypt(encrypted_buffer, wrong_password);
	ASSERT_FALSE(fn_name, decrypt_result.has_value()); // Decryption must fail

	RETURN_TEST(fn_name, 0);
}

int TestAESDecryptionWithCorruptedData() {
	const std::string fn_name = "TestAESDecryptionWithCorruptedData";
	const std::string password = "StrongPassword123!";
	const std::string original_data = "Important confidential data";

	// Encrypt the data
	auto encrypt_result = AES::Encrypt(original_data, password);
	ASSERT_TRUE(fn_name, encrypt_result.has_value());

	auto encrypted_future = std::move(encrypt_result.value());
	StormByte::Util::Buffer encrypted_buffer = encrypted_future.get();
	ASSERT_FALSE(fn_name, encrypted_buffer.Empty());

	// Corrupt the encrypted data (flip a bit in the buffer)
	auto corrupted_buffer = encrypted_buffer;
	auto corrupted_span = corrupted_buffer.Data();
	if (!corrupted_span.empty()) {
		corrupted_buffer.Data()[0] = std::byte(static_cast<uint8_t>(~std::to_integer<uint8_t>(corrupted_span[0]))); // Flip the first byte
	}

	// Attempt to decrypt the corrupted data
	auto decrypt_result = AES::Decrypt(corrupted_buffer, password);
	ASSERT_FALSE(fn_name, decrypt_result.has_value()); // Decryption must fail

	RETURN_TEST(fn_name, 0);
}

int main() {
	int result = 0;

	result += TestAESEncryptDecryptConsistency();
	result += TestAESWrongDecryptionPassword();
	result += TestAESDecryptionWithCorruptedData();

	if (result == 0) {
		std::cout << "All tests passed!" << std::endl;
	} else {
		std::cout << result << " tests failed." << std::endl;
	}
	return result;
}
