#pragma once

#include <StormByte/network/data/encryption/typedefs.hxx>

/**
 * @namespace AES
 * @brief The namespace containing AES encryption functions.
 */
namespace StormByte::Network::Data::Encryption::AES {
	/**
	 * @brief Encrypts a string using AES.
	 * @param input The string to encrypt.
	 * @param password The password to use for encryption.
	 * @return Expected<FutureBuffer, CryptoException> containing the encrypted Buffer or an error.
	 */
	STORMBYTE_NETWORK_PUBLIC ExpectedCryptoFutureBuffer Encrypt(const std::string& input, const std::string& password) noexcept;

	/**
	 * @brief Encrypts a Buffer using AES.
	 * @param input The Buffer to encrypt.
	 * @param password The password to use for encryption.
	 * @return Expected<FutureBuffer, CryptoException> containing the encrypted Buffer or an error.
	 */
	STORMBYTE_NETWORK_PUBLIC ExpectedCryptoFutureBuffer Encrypt(const StormByte::Buffers::Simple& input, const std::string& password) noexcept;

	/**
	 * @brief Decrypts a string using AES.
	 * @param input The string to decrypt.
	 * @param password The password to use for decryption.
	 * @return Expected<FutureBuffer, CryptoException> containing the decrypted Buffer or an error.
	 */
	STORMBYTE_NETWORK_PUBLIC ExpectedCryptoFutureBuffer Decrypt(const std::string& input, const std::string& password) noexcept;

	/**
	 * @brief Decrypts a Buffer using AES.
	 * @param input The Buffer to decrypt.
	 * @param password The password to use for decryption.
	 * @return Expected<FutureBuffer, CryptoException> containing the decrypted Buffer or an error.
	 */
	STORMBYTE_NETWORK_PUBLIC ExpectedCryptoFutureBuffer Decrypt(const StormByte::Buffers::Simple& input, const std::string& password) noexcept;

	/**
	 * @brief Generates a random password.
	 * @param passwordSize The size of the password to generate.
	 * @return The generated password.
	 */
	STORMBYTE_NETWORK_PUBLIC ExpectedCryptoFutureString RandomPassword(const size_t& passwordSize = 16) noexcept;
}