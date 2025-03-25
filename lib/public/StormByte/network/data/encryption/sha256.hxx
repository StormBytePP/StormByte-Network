#pragma once

#include <StormByte/network/data/encryption/typedefs.hxx>

/**
 * @namespace SHA256
 * @brief Namespace for SHA-256 hashing utilities.
 */
namespace StormByte::Network::Data::Encryption::SHA256 {
	/**
	 * @brief Hashes the input data using SHA-256.
	 * @param input The input string to hash.
	 * @return Expected<std::string, CryptoException> containing the hash or an error.
	 */
	STORMBYTE_NETWORK_PUBLIC ExpectedCryptoString Hash(const std::string& input) noexcept;

	/**
	 * @brief Hashes the input data using SHA-256.
	 * @param buffer The input buffer to hash.
	 * @return Expected<std::string, CryptoException> containing the hash or an error.
	 */
	STORMBYTE_NETWORK_PUBLIC ExpectedCryptoString Hash(const Util::Buffer& buffer) noexcept;

	/**
	 * @brief Hashes the input data using SHA-256.
	 * @param promisedBuffer The future buffer to hash.
	 * @return Expected<std::string, CryptoException> containing the hash or an error.
	 */
	STORMBYTE_NETWORK_PUBLIC ExpectedCryptoString Hash(FutureBuffer& promisedBuffer) noexcept;
}