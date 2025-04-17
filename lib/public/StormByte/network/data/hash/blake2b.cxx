#include <StormByte/network/data/hash/blake2b.hxx>
#include <blake2.h>
#include <hex.h>
#include <future>
#include <vector>

using namespace StormByte::Network::Data::Hash;

namespace {
    ExpectedHashFutureString ComputeBlake2b(std::span<const std::byte> dataSpan) noexcept {
        try {
            std::vector<uint8_t> data(dataSpan.size());
            std::transform(dataSpan.begin(), dataSpan.end(), data.begin(),
                           [](std::byte b) { return static_cast<uint8_t>(b); });

            CryptoPP::BLAKE2b hash;
            std::string hashOutput;

            CryptoPP::StringSource ss(data.data(), data.size(), true,
                                      new CryptoPP::HashFilter(hash,
                                      new CryptoPP::HexEncoder(
                                          new CryptoPP::StringSink(hashOutput))));

            return hashOutput;
        } catch (const std::exception& e) {
            return StormByte::Unexpected<StormByte::Network::CryptoException>("Blake2b hashing failed: {}", e.what());
        }
    }
}

ExpectedHashFutureString Blake2b::Hash(const std::string& input) noexcept {
    std::span<const std::byte> dataSpan(reinterpret_cast<const std::byte*>(input.data()), input.size());
    return ComputeBlake2b(dataSpan);
}

ExpectedHashFutureString Blake2b::Hash(const Buffers::Simple& buffer) noexcept {
    return ComputeBlake2b(buffer.Data());
}