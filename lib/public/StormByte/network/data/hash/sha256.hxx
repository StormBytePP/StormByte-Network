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
	STORMBYTE_NETWORK_PUBLIC ExpectedHashFutureString Hash(const std::string& input) noexcept;

	/**
	 * @brief Hashes the input data using SHA-256.
	 * @param buffer The input buffer to hash.
	 * @return Expected<std::string, CryptoException> containing the hash or an error.
	 */
	STORMBYTE_NETWORK_PUBLIC ExpectedHashFutureString Hash(const Buffers::Simple& buffer) noexcept;

	/**
	 * @brief Hashes data asynchronously using the Consumer/Producer model.
	 * 
	 * @param consumer The Consumer buffer containing the input data.
	 * @return A Consumer buffer containing the hash result.
	 */
	STORMBYTE_NETWORK_PUBLIC StormByte::Buffers::Consumer Hash(const Buffers::Consumer consumer) noexcept;
}