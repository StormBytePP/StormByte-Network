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
	STORMBYTE_NETWORK_PUBLIC ExpectedKeyPair 							GenerateKeyPair(const int& keyStrength) noexcept;

	/**
	 * @brief Encrypts a message using the RSA public key.
	 * @param message The message to encrypt.
	 * @param publicKey The RSA public key to use for encryption.
	 * @return ExpectedCryptoFutureBuffer containing the encrypted Buffer or an error.
	 */
	STORMBYTE_NETWORK_PUBLIC ExpectedCryptoFutureBuffer 				Encrypt(const std::string& message, const std::string& publicKey) noexcept;

	/**
	 * @brief Decrypts a message using the RSA private key.
	 * @param encryptedBuffer The buffer containing the encrypted data.
	 * @param privateKey The RSA private key to use for decryption.
	 * @return ExpectedCryptoFutureString containing the decrypted message or an error.
	 */
	STORMBYTE_NETWORK_PUBLIC ExpectedCryptoFutureString 				Decrypt(const StormByte::Buffers::Simple& encryptedBuffer, const std::string& privateKey) noexcept;

    /**
     * @brief Encrypts data asynchronously using the Consumer/Producer model.
     * 
     * @param consumer The Consumer buffer containing the input data.
     * @param publicKey The RSA public key to use for encryption.
     * @return A Consumer buffer containing the encrypted data.
     */
    STORMBYTE_NETWORK_PUBLIC StormByte::Buffers::Consumer Encrypt(const Buffers::Consumer consumer, const std::string& publicKey) noexcept;

    /**
     * @brief Decrypts data asynchronously using the Consumer/Producer model.
     * 
     * @param consumer The Consumer buffer containing the encrypted data.
     * @param privateKey The RSA private key to use for decryption.
     * @return A Consumer buffer containing the decrypted data.
     */
    STORMBYTE_NETWORK_PUBLIC StormByte::Buffers::Consumer Decrypt(const Buffers::Consumer consumer, const std::string& privateKey) noexcept;
}