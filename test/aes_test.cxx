#include <StormByte/network/data/encryption/aes.hxx>
#include <StormByte/test_handlers.h>

#include <thread>

using namespace StormByte::Network::Data::Encryption;

int TestAESEncryptDecryptConsistency() {
    const std::string fn_name = "TestAESEncryptDecryptConsistency";
    const std::string password = "SecurePassword123!";
    const std::string original_data = "Confidential information to encrypt and decrypt.";

    // Encrypt the data
    auto encrypt_result = AES::Encrypt(original_data, password);
    ASSERT_TRUE(fn_name, encrypt_result.has_value());

    auto encrypted_future = std::move(encrypt_result.value());
    StormByte::Buffers::Simple encrypted_buffer = encrypted_future.get();
    ASSERT_FALSE(fn_name, encrypted_buffer.Empty());

    // Decrypt the data
    auto decrypt_result = AES::Decrypt(encrypted_buffer, password);
    ASSERT_TRUE(fn_name, decrypt_result.has_value());

    auto decrypted_future = std::move(decrypt_result.value());
    StormByte::Buffers::Simple decrypted_buffer = decrypted_future.get();
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
    StormByte::Buffers::Simple encrypted_buffer = encrypted_future.get();
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
    StormByte::Buffers::Simple encrypted_buffer = encrypted_future.get();
    ASSERT_FALSE(fn_name, encrypted_buffer.Empty());

    // Corrupt the encrypted data (flip a bit in the buffer)
    auto corrupted_buffer = encrypted_buffer;
    auto corrupted_span = corrupted_buffer.Span();
    if (!corrupted_span.empty()) {
        corrupted_span[0] = std::byte(static_cast<uint8_t>(~std::to_integer<uint8_t>(corrupted_span[0]))); // Flip the first byte
    }

    // Attempt to decrypt the corrupted data
    auto decrypt_result = AES::Decrypt(corrupted_buffer, password);
    ASSERT_FALSE(fn_name, decrypt_result.has_value()); // Decryption must fail

    RETURN_TEST(fn_name, 0);
}

int TestAESEncryptionProducesDifferentContent() {
    const std::string fn_name = "TestAESEncryptionProducesDifferentContent";
    const std::string password = "SecurePassword123!";
    const std::string original_data = "Important data to encrypt";

    // Encrypt the data
    auto encrypt_result = AES::Encrypt(original_data, password);
    ASSERT_TRUE(fn_name, encrypt_result.has_value());

    auto encrypted_future = std::move(encrypt_result.value());
    StormByte::Buffers::Simple encrypted_buffer = encrypted_future.get();
    ASSERT_FALSE(fn_name, encrypted_buffer.Empty());

    // Verify encrypted content is different from original
    ASSERT_NOT_EQUAL(fn_name, original_data, std::string(reinterpret_cast<const char*>(encrypted_buffer.Data().data()), encrypted_buffer.Size()));

    RETURN_TEST(fn_name, 0);
}

int TestAESEncryptUsingConsumerProducer() {
    const std::string fn_name = "TestAESEncryptUsingConsumerProducer";
    const std::string input_data = "This is some data to encrypt using the Consumer/Producer model.";
    const std::string password = "SecurePassword123!";

    // Create a producer buffer and write the input data
    StormByte::Buffers::Producer producer;
    producer << input_data;
    producer << StormByte::Buffers::Status::EoF; // Mark the producer as EOF

    // Create a consumer buffer from the producer
    StormByte::Buffers::Consumer consumer(producer.Consumer());

    // Encrypt the data asynchronously
    auto encrypted_consumer = AES::Encrypt(consumer, password);

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

int TestAESDecryptUsingConsumerProducer() {
    const std::string fn_name = "TestAESDecryptUsingConsumerProducer";
    const std::string input_data = "This is some data to encrypt and decrypt using the Consumer/Producer model.";
    const std::string password = "SecurePassword123!";

    // Create a producer buffer and write the input data
    StormByte::Buffers::Producer producer;
    producer << input_data;
    producer << StormByte::Buffers::Status::EoF; // Mark the producer as EOF

    // Create a consumer buffer from the producer
    StormByte::Buffers::Consumer consumer(producer.Consumer());

    // Encrypt the data asynchronously
    auto encrypted_consumer = AES::Encrypt(consumer, password);

    // Decrypt the data asynchronously
    auto decrypted_consumer = AES::Decrypt(encrypted_consumer, password);

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

int TestAESEncryptDecryptInOneStep() {
    const std::string fn_name = "TestAESEncryptDecryptInOneStep";
    const std::string input_data = "This is the data to encrypt and decrypt in one step.";
    const std::string password = "SecurePassword123!";

    // Create a producer buffer and write the input data
    StormByte::Buffers::Producer producer;
    producer << input_data;
    producer << StormByte::Buffers::Status::EoF; // Mark the producer as EOF

    // Create a consumer buffer from the producer
    StormByte::Buffers::Consumer consumer(producer.Consumer());

    // Encrypt the data asynchronously
    auto encrypted_consumer = AES::Encrypt(consumer, password);

    // Decrypt the data asynchronously using the encrypted consumer
    auto decrypted_consumer = AES::Decrypt(encrypted_consumer, password);

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

int main() {
    int result = 0;

    result += TestAESEncryptDecryptConsistency();
    result += TestAESWrongDecryptionPassword();
    result += TestAESDecryptionWithCorruptedData();
    result += TestAESEncryptionProducesDifferentContent();
    result += TestAESEncryptUsingConsumerProducer();
    result += TestAESDecryptUsingConsumerProducer();
    result += TestAESEncryptDecryptInOneStep();

    if (result == 0) {
        return 0;
    } else {
        return result;
    }
}
