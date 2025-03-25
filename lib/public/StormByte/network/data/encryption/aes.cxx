#include <StormByte/network/data/encryption/aes.hxx>

#include <aes.h>
#include <cryptlib.h>
#include <hex.h>
#include <modes.h>
#include <filters.h>
#include <format>
#include <future>
#include <osrng.h>
#include <secblock.h>
#include <pwdbased.h>

using namespace StormByte::Network::Data::Encryption;

namespace {
	ExpectedCryptoBuffer EncryptHelper(std::span<const std::byte> dataSpan, const std::string& password) noexcept {
		try {
			// Generate key and IV securely
			CryptoPP::SecByteBlock salt(16);
			CryptoPP::SecByteBlock iv(CryptoPP::AES::BLOCKSIZE);
			CryptoPP::AutoSeededRandomPool rng;
			rng.GenerateBlock(salt, salt.size());
			rng.GenerateBlock(iv, iv.size());

			// Derive the key using PBKDF2
			CryptoPP::SecByteBlock key(CryptoPP::AES::DEFAULT_KEYLENGTH);
			CryptoPP::PKCS5_PBKDF2_HMAC<CryptoPP::SHA256> pbkdf2;
			pbkdf2.DeriveKey(key, key.size(), 0, reinterpret_cast<const uint8_t*>(password.data()),
							password.size(), salt, salt.size(), 10000);

			// AES encryption in CBC mode
			std::vector<uint8_t> encryptedData;
			CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption encryption(key, key.size(), iv);
			CryptoPP::StringSource ss(reinterpret_cast<const uint8_t*>(dataSpan.data()), dataSpan.size_bytes(), true,
									new CryptoPP::StreamTransformationFilter(encryption,
																			new CryptoPP::VectorSink(encryptedData)));

			// Combine salt and IV with encrypted data
			encryptedData.insert(encryptedData.begin(), salt.begin(), salt.end());
			encryptedData.insert(encryptedData.begin() + salt.size(), iv.begin(), iv.end());

			// Convert to std::vector<std::byte>
			std::vector<std::byte> convertedData(encryptedData.size());
			std::transform(encryptedData.begin(), encryptedData.end(), convertedData.begin(),
						[](uint8_t byte) { return static_cast<std::byte>(byte); });

			// Wrap the result into a std::promise and std::future
			std::promise<StormByte::Buffer> promise;
			promise.set_value(StormByte::Buffer(std::move(convertedData)));
			return promise.get_future();
		} catch (const std::exception& e) {
			return StormByte::Unexpected<StormByte::Network::CryptoException>(e.what());
		}
	}

	ExpectedCryptoBuffer DecryptHelper(std::span<const std::byte> encryptedSpan, const std::string& password) noexcept {
		try {
			const size_t saltSize = 16;
			const size_t ivSize = CryptoPP::AES::BLOCKSIZE;

			if (encryptedSpan.size_bytes() < saltSize + ivSize) {
				return StormByte::Unexpected<StormByte::Network::CryptoException>("Encrypted data too short to contain salt and IV");
			}

			// Extract salt and IV
			CryptoPP::SecByteBlock salt(saltSize), iv(ivSize);
			std::memcpy(salt.data(), encryptedSpan.data(), saltSize);
			std::memcpy(iv.data(), encryptedSpan.data() + saltSize, ivSize);

			// Extract encrypted data
			encryptedSpan = encryptedSpan.subspan(saltSize + ivSize);

			// Derive key using PBKDF2
			CryptoPP::SecByteBlock key(CryptoPP::AES::DEFAULT_KEYLENGTH);
			CryptoPP::PKCS5_PBKDF2_HMAC<CryptoPP::SHA256> pbkdf2;
			pbkdf2.DeriveKey(key, key.size(), 0, reinterpret_cast<const uint8_t*>(password.data()),
							password.size(), salt, salt.size(), 10000);

			// AES decryption in CBC mode
			std::vector<uint8_t> decryptedData;
			CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption decryption(key, key.size(), iv);
			CryptoPP::StringSource ss(reinterpret_cast<const uint8_t*>(encryptedSpan.data()), encryptedSpan.size_bytes(), true,
									new CryptoPP::StreamTransformationFilter(decryption,
																			new CryptoPP::VectorSink(decryptedData)));

			// Convert to std::vector<std::byte>
			std::vector<std::byte> convertedData(decryptedData.size());
			std::transform(decryptedData.begin(), decryptedData.end(), convertedData.begin(),
						[](uint8_t byte) { return static_cast<std::byte>(byte); });

			// Wrap the result into a std::promise and std::future
			std::promise<StormByte::Buffer> promise;
			promise.set_value(StormByte::Buffer(std::move(convertedData)));
			return promise.get_future();
		} catch (const std::exception& e) {
			return StormByte::Unexpected<StormByte::Network::CryptoException>(e.what());
		}
	}
}

// Encrypt Function Overloads
ExpectedCryptoBuffer AES::Encrypt(const std::string& input, const std::string& password) noexcept {
	std::span<const std::byte> dataSpan(reinterpret_cast<const std::byte*>(input.data()), input.size());
	return EncryptHelper(dataSpan, password);
}

ExpectedCryptoBuffer AES::Encrypt(const StormByte::Buffer& input, const std::string& password) noexcept {
	return EncryptHelper(input.Data(), password);
}

ExpectedCryptoBuffer AES::Encrypt(FutureBuffer& bufferFuture, const std::string& password) noexcept {
	try {
		StormByte::Buffer buffer = bufferFuture.get();
		return EncryptHelper(buffer.Data(), password);
	} catch (const std::exception& e) {
		return StormByte::Unexpected<StormByte::Network::CryptoException>(e.what());
	}
}

// Decrypt Function Overloads
ExpectedCryptoBuffer AES::Decrypt(const std::string& input, const std::string& password) noexcept {
	std::span<const std::byte> dataSpan(reinterpret_cast<const std::byte*>(input.data()), input.size());
	return DecryptHelper(dataSpan, password);
}

ExpectedCryptoBuffer AES::Decrypt(const StormByte::Buffer& input, const std::string& password) noexcept {
	return DecryptHelper(input.Data(), password);
}

ExpectedCryptoBuffer AES::Decrypt(FutureBuffer& bufferFuture, const std::string& password) noexcept {
	try {
		StormByte::Buffer buffer = bufferFuture.get();
		return DecryptHelper(buffer.Data(), password);
	} catch (const std::exception& e) {
		return StormByte::Unexpected<StormByte::Network::CryptoException>(e.what());
	}
}

// RandomPassword Function
ExpectedCryptoString AES::RandomPassword(const size_t& passwordSize) noexcept {
	try {
		// Random number generator
		CryptoPP::AutoSeededRandomPool rng;

		// Buffer for random password
		CryptoPP::SecByteBlock password(passwordSize);

		// Fill buffer with random bytes
		rng.GenerateBlock(password, passwordSize);

		// Convert random bytes to printable characters (Hex format)
		std::string passwordString;
		CryptoPP::HexEncoder encoder(new CryptoPP::StringSink(passwordString));
		encoder.Put(password.data(), password.size());
		encoder.MessageEnd();

		// Return the generated password
		return passwordString;
	} catch (const std::exception& e) {
		return StormByte::Unexpected<CryptoException>("Failed to generate random password: {}", e.what());
	}
}