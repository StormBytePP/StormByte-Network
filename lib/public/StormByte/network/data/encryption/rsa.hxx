#pragma once

#include <StormByte/network/data/encryption/typedefs.hxx>

/**
 * @namespace RSA
 * @brief The namespace containing RSA encryption functions.
 */
namespace StormByte::Network::Data::Encryption::RSA {
	/**
	 * @brief Generates an RSA private/public key pair.
	 * @param keyStrength The strength of the key (e.g., 2048, 4096 bits).
	 * @return ExpectedKeyPair containing the private and public key pair or an error.
	 */
	STORMBYTE_NETWORK_PUBLIC ExpectedKeyPair 					GenerateKeyPair(const int& keyStrength) noexcept;

	/**
	 * @brief Encrypts a message using the RSA public key.
	 * @param message The message to encrypt.
	 * @param publicKey The RSA public key to use for encryption.
	 * @return ExpectedCryptoBuffer containing the encrypted Buffer or an error.
	 */
	STORMBYTE_NETWORK_PUBLIC ExpectedCryptoBuffer 				Encrypt(const std::string& message, const std::string& publicKey) noexcept;

	/**
	 * @brief Decrypts a message using the RSA private key.
	 * @param encryptedBuffer The buffer containing the encrypted data.
	 * @param privateKey The RSA private key to use for decryption.
	 * @return ExpectedCryptoString containing the decrypted message or an error.
	 */
	STORMBYTE_NETWORK_PUBLIC ExpectedCryptoString 				Decrypt(const StormByte::Buffers::Simple& encryptedBuffer, const std::string& privateKey) noexcept;
}