#include <StormByte/network/data/packet.hxx>

#include <span>

using namespace StormByte::Network::Data;

const std::span<const std::byte> Packet::Data() const noexcept {
	PrepareBuffer();
	return m_buffer.Data();
}