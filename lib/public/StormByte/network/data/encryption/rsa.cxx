#include <StormByte/network/data/encryption/rsa.hxx>

#include <cryptlib.h>
#include <base64.h>
#include <filters.h>
#include <format>
#include <osrng.h>
#include <rsa.h>
#include <secblock.h>

#include <string>

using namespace StormByte::Network::Data::Encryption;

namespace {
	std::string SerializeKey(const CryptoPP::RSA::PrivateKey& key) {
		std::string keyString;
		CryptoPP::ByteQueue queue;
		key.Save(queue); // Save key in ASN.1 format
		CryptoPP::Base64Encoder encoder(new CryptoPP::StringSink(keyString), false); // Base64 encode
		queue.CopyTo(encoder);
		encoder.MessageEnd();
		return keyString;
	}

	std::string SerializeKey(const CryptoPP::RSA::PublicKey& key) {
		std::string keyString;
		CryptoPP::ByteQueue queue;
		key.Save(queue); // Save key in ASN.1 format
		CryptoPP::Base64Encoder encoder(new CryptoPP::StringSink(keyString), false); // Base64 encode
		queue.CopyTo(encoder);
		encoder.MessageEnd();
		return keyString;
	}

	CryptoPP::RSA::PublicKey DeserializePublicKey(const std::string& keyString) {
		CryptoPP::RSA::PublicKey key;
		CryptoPP::Base64Decoder decoder;
		CryptoPP::StringSource(keyString, true, new CryptoPP::Redirector(decoder));
		key.Load(decoder); // Load the decoded key
		return key;
	}

	CryptoPP::RSA::PrivateKey DeserializePrivateKey(const std::string& keyString) {
		CryptoPP::RSA::PrivateKey key;
		CryptoPP::Base64Decoder decoder;
		CryptoPP::StringSource(keyString, true, new CryptoPP::Redirector(decoder));
		key.Load(decoder); // Load the decoded key
		return key;
	}
}

ExpectedKeyPair RSA::GenerateKeyPair(const int& keyStrength) noexcept {
	try {
		CryptoPP::AutoSeededRandomPool rng;

		CryptoPP::RSA::PrivateKey privateKey;
		privateKey.GenerateRandomWithKeySize(rng, keyStrength);

		CryptoPP::RSA::PublicKey publicKey;
		publicKey.AssignFrom(privateKey);

		KeyPair keyPair{
			.Private = SerializeKey(privateKey),
			.Public = SerializeKey(publicKey),
		};

		return keyPair;
	} catch (const std::exception& e) {
		return StormByte::Unexpected<CryptoException>("Failed to generate RSA keys: {}", e.what());
	}
}

ExpectedCryptoFutureBuffer RSA::Encrypt(const std::string& message, const std::string& publicKey) noexcept {
	try {
		CryptoPP::AutoSeededRandomPool rng;
		CryptoPP::RSA::PublicKey key = DeserializePublicKey(publicKey);

		if (!key.Validate(rng, 3)) {
			return StormByte::Unexpected<CryptoException>("Public Key Validation Failed");
		}

		std::string encryptedMessage;
		CryptoPP::RSAES_OAEP_SHA_Encryptor encryptor(key);
		CryptoPP::StringSource ss(message, true,
								new CryptoPP::PK_EncryptorFilter(rng, encryptor,
																	new CryptoPP::StringSink(encryptedMessage)));

		StormByte::Buffers::Simple buffer;
		buffer << encryptedMessage;

		std::promise<StormByte::Buffers::Simple> promise;
		promise.set_value(std::move(buffer));
		return promise.get_future();
	} catch (const std::exception& e) {
		return StormByte::Unexpected<CryptoException>("RSA encryption failed: {}", e.what());
	}
}

ExpectedCryptoFutureString RSA::Decrypt(const StormByte::Buffers::Simple& encryptedBuffer, const std::string& privateKey) noexcept {
	try {
		CryptoPP::AutoSeededRandomPool rng;
		CryptoPP::RSA::PrivateKey key = DeserializePrivateKey(privateKey);

		std::string decryptedMessage;
		std::string encryptedString(reinterpret_cast<const char*>(encryptedBuffer.Data().data()), encryptedBuffer.Size());
		CryptoPP::RSAES_OAEP_SHA_Decryptor decryptor(key);
		CryptoPP::StringSource ss(encryptedString, true,
								new CryptoPP::PK_DecryptorFilter(rng, decryptor,
																	new CryptoPP::StringSink(decryptedMessage)));

		return decryptedMessage;
	} catch (const std::exception& e) {
		return StormByte::Unexpected<CryptoException>("RSA decryption failed: {}", e.what());
	}
}