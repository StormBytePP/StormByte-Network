#include <StormByte/network/data/hash/sha512.hxx>

#include <hex.h>
#include <format>
#include <future>
#include <sha.h>
#include <vector>

using namespace StormByte::Network::Data::Hash;

namespace {
    /**
     * @brief Helper function to compute SHA-512 hash.
     * @param dataSpan The input data as std::span<const std::byte>.
     * @return Expected<std::string, CryptoException> containing the hash or an error.
     */
    ExpectedHashFutureString ComputeSHA512(std::span<const std::byte> dataSpan) noexcept {
        try {
            // Convert std::span<std::byte> to std::vector<uint8_t>
            std::vector<uint8_t> data;
            std::transform(dataSpan.begin(), dataSpan.end(), std::back_inserter(data),
                           [](std::byte b) { return static_cast<uint8_t>(b); });

            // Compute SHA-512 hash
            CryptoPP::SHA512 hash;
            std::string hashOutput;

            CryptoPP::StringSource ss(data.data(), data.size(), true,
                                      new CryptoPP::HashFilter(hash,
                                      new CryptoPP::HexEncoder(
                                          new CryptoPP::StringSink(hashOutput))));

            return hashOutput;
        } catch (const std::exception& e) {
            return StormByte::Unexpected<StormByte::Network::CryptoException>("SHA-512 hashing failed: {}", e.what());
        }
    }
}

ExpectedHashFutureString SHA512::Hash(const std::string& input) noexcept {
    // Create a std::span<std::byte> from the input string
    std::span<const std::byte> dataSpan(reinterpret_cast<const std::byte*>(input.data()), input.size());

    // Use the common helper function to compute the hash
    return ComputeSHA512(dataSpan);
}

ExpectedHashFutureString SHA512::Hash(const StormByte::Buffers::Simple& buffer) noexcept {
    // Use Buffer's Data() method to get std::span<std::byte>
    auto dataSpan = buffer.Data();

    // Use the common helper function to compute the hash
    return ComputeSHA512(dataSpan);
}