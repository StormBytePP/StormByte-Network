#pragma once

#include <StormByte/network/data/hash/typedefs.h>

/**
 * @namespace SHA512
 * @brief Namespace for SHA-512 hashing utilities.
 */
namespace StormByte::Network::Data::Hash::SHA512 {
    /**
     * @brief Hashes the input data using SHA-512.
     * @param input The input string to hash.
     * @return Expected<std::string, CryptoException> containing the hash or an error.
     */
    STORMBYTE_NETWORK_PUBLIC ExpectedHashFutureString Hash(const std::string& input) noexcept;

    /**
     * @brief Hashes the input data using SHA-512.
     * @param buffer The input buffer to hash.
     * @return Expected<std::string, CryptoException> containing the hash or an error.
     */
    STORMBYTE_NETWORK_PUBLIC ExpectedHashFutureString Hash(const Buffers::Simple& buffer) noexcept;
}