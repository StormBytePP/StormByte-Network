#pragma once

#include <StormByte/network/data/hash/typedefs.h>

/**
 * @namespace Blake2b
 * @brief Namespace for Blake2b hashing utilities.
 */
namespace StormByte::Network::Data::Hash::Blake2b {
    /**
     * @brief Hashes the input data using Blake2b.
     * @param input The input string to hash.
     * @return Expected<std::string, CryptoException> containing the hash or an error.
     */
    STORMBYTE_NETWORK_PUBLIC ExpectedHashFutureString Hash(const std::string& input) noexcept;

    /**
     * @brief Hashes the input data using Blake2b.
     * @param buffer The input buffer to hash.
     * @return Expected<std::string, CryptoException> containing the hash or an error.
     */
    STORMBYTE_NETWORK_PUBLIC ExpectedHashFutureString Hash(const Buffers::Simple& buffer) noexcept;
}