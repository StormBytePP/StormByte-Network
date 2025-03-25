#pragma once

#include <StormByte/network/data/hash/typedefs.h>

/**
 * @namespace SHA256
 * @brief Namespace for SHA-256 hashing utilities.
 */
namespace StormByte::Network::Data::Hash::SHA256 {
	/**
	 * @brief Hashes the input data using SHA-256.
	 * @param input The input string to hash.
	 * @return Expected<std::string, CryptoException> containing the hash or an error.
	 */
	STORMBYTE_NETWORK_PUBLIC ExpectedHashString Hash(const std::string& input) noexcept;

	/**
	 * @brief Hashes the input data using SHA-256.
	 * @param buffer The input buffer to hash.
	 * @return Expected<std::string, CryptoException> containing the hash or an error.
	 */
	STORMBYTE_NETWORK_PUBLIC ExpectedHashString Hash(const Buffer& buffer) noexcept;

	/**
	 * @brief Hashes the input data using SHA-256.
	 * @param promisedBuffer The future buffer to hash.
	 * @return Expected<std::string, CryptoException> containing the hash or an error.
	 */
	STORMBYTE_NETWORK_PUBLIC ExpectedHashString Hash(FutureBuffer& promisedBuffer) noexcept;
}