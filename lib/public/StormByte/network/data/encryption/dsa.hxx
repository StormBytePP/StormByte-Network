#pragma once

#include <StormByte/network/data/encryption/typedefs.hxx>

/**
 * @namespace DSA
 * @brief The namespace containing DSA signing and verification functions.
 */
namespace StormByte::Network::Data::Encryption::DSA {
    /**
     * @brief Generates a DSA private/public key pair.
     * @param keyStrength The strength of the key (e.g., 1024, 2048 bits).
     * @return ExpectedKeyPair containing the private and public key pair or an error.
     */
    STORMBYTE_NETWORK_PUBLIC ExpectedKeyPair GenerateKeyPair(const int& keyStrength) noexcept;

    /**
     * @brief Signs a message using the DSA private key.
     * @param message The message to sign.
     * @param privateKey The DSA private key to use for signing.
     * @return ExpectedCryptoFutureString containing the signature or an error.
     */
    STORMBYTE_NETWORK_PUBLIC ExpectedCryptoFutureString Sign(const std::string& message, const std::string& privateKey) noexcept;

    /**
     * @brief Verifies a signature using the DSA public key.
     * @param message The original message.
     * @param signature The signature to verify.
     * @param publicKey The DSA public key to use for verification.
     * @return True if the signature is valid, false otherwise.
     */
    STORMBYTE_NETWORK_PUBLIC bool Verify(const std::string& message, const std::string& signature, const std::string& publicKey) noexcept;
}