#include <StormByte/network/data/encryption/ecc.hxx>

#include <base64.h>
#include <cryptlib.h>
#include <eccrypto.h>
#include <filters.h>
#include <format>
#include <osrng.h>
#include <oids.h>
#include <string>

using namespace StormByte::Network::Data::Encryption;

namespace {
	using ECIES = CryptoPP::ECIES<CryptoPP::ECP>;

	std::string SerializeKey(const ECIES::PrivateKey& key) {
		std::string keyString;
		CryptoPP::ByteQueue queue;
		key.Save(queue); // Save the key in ASN.1 format
		CryptoPP::Base64Encoder encoder(new CryptoPP::StringSink(keyString), false); // Base64 encode
		queue.CopyTo(encoder);
		encoder.MessageEnd();
		return keyString;
	}

	std::string SerializeKey(const ECIES::PublicKey& key) {
		std::string keyString;
		CryptoPP::ByteQueue queue;
		key.Save(queue); // Save the key in ASN.1 format
		CryptoPP::Base64Encoder encoder(new CryptoPP::StringSink(keyString), false); // Base64 encode
		queue.CopyTo(encoder);
		encoder.MessageEnd();
		return keyString;
	}

	ECIES::PrivateKey DeserializePrivateKey(const std::string& keyString) {
		ECIES::PrivateKey key;
		CryptoPP::Base64Decoder decoder;
		CryptoPP::StringSource(keyString, true, new CryptoPP::Redirector(decoder));
		key.Load(decoder); // Load the decoded key
	
		// Explicitly initialize curve parameters
		key.AccessGroupParameters().Initialize(CryptoPP::ASN1::secp256r1());
	
		return key;
	}	

	ECIES::PublicKey DeserializePublicKey(const std::string& keyString) {
		ECIES::PublicKey key;
		CryptoPP::Base64Decoder decoder;
		CryptoPP::StringSource(keyString, true, new CryptoPP::Redirector(decoder));
		key.Load(decoder); // Load the decoded key
	
		// Explicitly initialize curve parameters
		key.AccessGroupParameters().Initialize(CryptoPP::ASN1::secp256r1());
	
		return key;
	}	
}

ExpectedKeyPair ECC::GenerateKeyPair() noexcept {
	try {
		CryptoPP::AutoSeededRandomPool rng;

		ECIES::PrivateKey privateKey;
		privateKey.Initialize(rng, CryptoPP::ASN1::secp256r1()); // Use the NIST P-256 curve

		ECIES::PublicKey publicKey;
		privateKey.MakePublicKey(publicKey);

		KeyPair keyPair{
			.Private = SerializeKey(privateKey), // Correct field name
			.Public = SerializeKey(publicKey),  // Correct field name
		};

		return keyPair;
	} catch (const std::exception& e) {
		return StormByte::Unexpected<CryptoException>(std::format("Failed to generate ECC keys: {}", e.what()));
	}
}

ExpectedCryptoBuffer ECC::Encrypt(const std::string& message, const std::string& publicKey) noexcept {
	try {
		CryptoPP::AutoSeededRandomPool rng;

		// Deserialize, initialize, and validate the public key
		ECIES::PublicKey key = DeserializePublicKey(publicKey);
		if (!key.Validate(rng, 3)) {
			return StormByte::Unexpected<CryptoException>("Public key validation failed");
		}

		// Initialize the encryptor
		ECIES::Encryptor encryptor(key);

		// Perform encryption
		std::string encryptedMessage;
		CryptoPP::StringSource ss(message, true,
									new CryptoPP::PK_EncryptorFilter(rng, encryptor,
																	new CryptoPP::StringSink(encryptedMessage)));

		// Convert the encrypted message into a buffer
		StormByte::Util::Buffer buffer;
		buffer << encryptedMessage;

		std::promise<StormByte::Util::Buffer> promise;
		promise.set_value(std::move(buffer));
		return promise.get_future();
	} catch (const std::exception& e) {
		return StormByte::Unexpected<CryptoException>(std::string("ECC encryption failed: ") + e.what());
	}
}	

ExpectedCryptoString ECC::Decrypt(const StormByte::Util::Buffer& encryptedBuffer, const std::string& privateKey) noexcept {
	try {
		CryptoPP::AutoSeededRandomPool rng;

		// Deserialize, initialize, and validate the private key
		ECIES::PrivateKey key = DeserializePrivateKey(privateKey);
		if (!key.Validate(rng, 3)) {
			return StormByte::Unexpected<CryptoException>("Private key validation failed");
		}

		// Initialize the decryptor
		ECIES::Decryptor decryptor(key);

		// Perform decryption
		std::string decryptedMessage;
		std::string encryptedString(reinterpret_cast<const char*>(encryptedBuffer.Data().data()), encryptedBuffer.Size());
		CryptoPP::StringSource ss(encryptedString, true,
									new CryptoPP::PK_DecryptorFilter(rng, decryptor,
																	new CryptoPP::StringSink(decryptedMessage)));

		return decryptedMessage;
	} catch (const std::exception& e) {
		return StormByte::Unexpected<CryptoException>(std::string("ECC decryption failed: ") + e.what());
	}
}