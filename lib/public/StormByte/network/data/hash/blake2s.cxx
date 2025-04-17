#include <StormByte/network/data/hash/blake2s.hxx>
#include <blake2.h>
#include <hex.h>
#include <future>
#include <vector>

using namespace StormByte::Network::Data::Hash;

namespace {
    ExpectedHashFutureString ComputeBlake2s(std::span<const std::byte> dataSpan) noexcept {
        try {
            std::vector<uint8_t> data(dataSpan.size());
            std::transform(dataSpan.begin(), dataSpan.end(), data.begin(),
                           [](std::byte b) { return static_cast<uint8_t>(b); });

            CryptoPP::BLAKE2s hash;
            std::string hashOutput;

            CryptoPP::StringSource ss(data.data(), data.size(), true,
                                      new CryptoPP::HashFilter(hash,
                                      new CryptoPP::HexEncoder(
                                          new CryptoPP::StringSink(hashOutput))));

            return hashOutput;
        } catch (const std::exception& e) {
            return StormByte::Unexpected<StormByte::Network::CryptoException>("Blake2s hashing failed: {}", e.what());
        }
    }
}

ExpectedHashFutureString Blake2s::Hash(const std::string& input) noexcept {
    std::span<const std::byte> dataSpan(reinterpret_cast<const std::byte*>(input.data()), input.size());
    return ComputeBlake2s(dataSpan);
}

ExpectedHashFutureString Blake2s::Hash(const Buffers::Simple& buffer) noexcept {
    return ComputeBlake2s(buffer.Data());
}