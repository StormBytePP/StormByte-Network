#pragma once

#include <StormByte/network/data/encryption/typedefs.hxx>

/**
 * @namespace ECC
 * @brief The namespace containing Elliptic Curve Cryptography functions.
 */
namespace StormByte::Network::Data::Encryption::ECC {
	/**
	 * @brief Generates an ECC private/public key pair.
	 * @return ExpectedKeyPair containing the private and public key pair or an error.
	 */
	STORMBYTE_NETWORK_PUBLIC ExpectedKeyPair GenerateKeyPair() noexcept;

	/**
	 * @brief Encrypts a message using the ECC public key.
	 * @param message The message to encrypt.
	 * @param publicKey The ECC public key to use for encryption.
	 * @return ExpectedCryptoBuffer containing the encrypted Buffer or an error.
	 */
	STORMBYTE_NETWORK_PUBLIC ExpectedCryptoBuffer Encrypt(const std::string& message, const std::string& publicKey) noexcept;

	/**
	 * @brief Decrypts a message using the ECC private key.
	 * @param encryptedBuffer The buffer containing the encrypted data.
	 * @param privateKey The ECC private key to use for decryption.
	 * @return ExpectedCryptoString containing the decrypted message or an error.
	 */
	STORMBYTE_NETWORK_PUBLIC ExpectedCryptoString Decrypt(const StormByte::Util::Buffer& encryptedBuffer, const std::string& privateKey) noexcept;
}