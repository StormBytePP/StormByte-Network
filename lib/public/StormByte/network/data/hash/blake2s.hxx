#pragma once

#include <StormByte/network/data/hash/typedefs.h>

/**
 * @namespace Blake2s
 * @brief Namespace for Blake2s hashing utilities.
 */
namespace StormByte::Network::Data::Hash::Blake2s {
    /**
     * @brief Hashes the input data using Blake2s.
     * @param input The input string to hash.
     * @return Expected<std::string, CryptoException> containing the hash or an error.
     */
    STORMBYTE_NETWORK_PUBLIC ExpectedHashFutureString Hash(const std::string& input) noexcept;

    /**
     * @brief Hashes the input data using Blake2s.
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