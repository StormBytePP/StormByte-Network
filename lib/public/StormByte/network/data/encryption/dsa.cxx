#include <StormByte/network/data/encryption/dsa.hxx>

#include <cryptlib.h>
#include <base64.h>
#include <dsa.h>
#include <hex.h>
#include <osrng.h>
#include <filters.h>
#include <string>
#include <future>
#include <iostream>

using namespace StormByte::Network::Data::Encryption;

namespace {
    std::string SerializeKey(const CryptoPP::DSA::PrivateKey& key) {
        std::string keyString;
        CryptoPP::ByteQueue queue;
        key.Save(queue); // Save key in ASN.1 format
        CryptoPP::Base64Encoder encoder(new CryptoPP::StringSink(keyString), false); // Base64 encode
        queue.CopyTo(encoder);
        encoder.MessageEnd();
        return keyString;
    }

    std::string SerializeKey(const CryptoPP::DSA::PublicKey& key) {
        std::string keyString;
        CryptoPP::ByteQueue queue;
        key.Save(queue); // Save key in ASN.1 format
        CryptoPP::Base64Encoder encoder(new CryptoPP::StringSink(keyString), false); // Base64 encode
        queue.CopyTo(encoder);
        encoder.MessageEnd();
        return keyString;
    }

    CryptoPP::DSA::PublicKey DeserializePublicKey(const std::string& keyString) {
        CryptoPP::DSA::PublicKey key;
        CryptoPP::Base64Decoder decoder;
        CryptoPP::StringSource(keyString, true, new CryptoPP::Redirector(decoder));
        key.Load(decoder); // Load the decoded key
        return key;
    }

    CryptoPP::DSA::PrivateKey DeserializePrivateKey(const std::string& keyString) {
        CryptoPP::DSA::PrivateKey key;
        CryptoPP::Base64Decoder decoder;
        CryptoPP::StringSource(keyString, true, new CryptoPP::Redirector(decoder));
        key.Load(decoder); // Load the decoded key
        return key;
    }
}

ExpectedKeyPair DSA::GenerateKeyPair(const int& keyStrength) noexcept {
    try {
        CryptoPP::AutoSeededRandomPool rng;

        // Generate the private key
        CryptoPP::DSA::PrivateKey privateKey;
        privateKey.GenerateRandomWithKeySize(rng, keyStrength);

        // Generate the public key
        CryptoPP::DSA::PublicKey publicKey;
        publicKey.AssignFrom(privateKey);

        // Validate the keys
        if (!privateKey.Validate(rng, 3)) {
            return StormByte::Unexpected<CryptoException>("Private key validation failed");
        }
        if (!publicKey.Validate(rng, 3)) {
            return StormByte::Unexpected<CryptoException>("Public key validation failed");
        }

        // Serialize the keys
        std::string serializedPrivateKey = SerializeKey(privateKey);
        std::string serializedPublicKey = SerializeKey(publicKey);

        // Return the key pair
        return KeyPair{
            .Private = serializedPrivateKey,
            .Public = serializedPublicKey,
        };
    } catch (const std::exception& e) {
        return StormByte::Unexpected<CryptoException>("Unexpected error during key generation: " + std::string(e.what()));
    }
}

ExpectedCryptoFutureString DSA::Sign(const std::string& message, const std::string& privateKey) noexcept {
    try {
        CryptoPP::AutoSeededRandomPool rng;

        // Deserialize and validate the private key
        CryptoPP::DSA::PrivateKey key = DeserializePrivateKey(privateKey);
        if (!key.Validate(rng, 3)) {
            return StormByte::Unexpected<CryptoException>("Private key validation failed");
        }

        // Initialize the signer
        CryptoPP::DSA::Signer signer(key);

        // Sign the message
        std::string signature;
        CryptoPP::StringSource ss(
            message, true,
            new CryptoPP::SignerFilter(
                rng, signer,
                new CryptoPP::StringSink(signature)
            )
        );

        return signature;
    } catch (const std::exception& e) {
        return StormByte::Unexpected<CryptoException>("DSA signing failed: " + std::string(e.what()));
    }
}

bool DSA::Verify(const std::string& message, const std::string& signature, const std::string& publicKey) noexcept {
    try {
        CryptoPP::AutoSeededRandomPool rng;

        // Deserialize and validate the public key
        CryptoPP::DSA::PublicKey key = DeserializePublicKey(publicKey);
        if (!key.Validate(rng, 3)) {
            return false; // Public key validation failed
        }

        // Initialize the verifier
        CryptoPP::DSA::Verifier verifier(key);

        // Verify the signature
        bool result = false;
        CryptoPP::StringSource ss(
            signature + message, true,
            new CryptoPP::SignatureVerificationFilter(
                verifier,
                new CryptoPP::ArraySink((CryptoPP::byte*)&result, sizeof(result)),
                CryptoPP::SignatureVerificationFilter::PUT_RESULT | CryptoPP::SignatureVerificationFilter::SIGNATURE_AT_BEGIN
            )
        );

        return result;
    } catch (const CryptoPP::Exception&) {
        return false; // Signature verification failed
    } catch (const std::exception&) {
        return false; // Other errors
    }
}