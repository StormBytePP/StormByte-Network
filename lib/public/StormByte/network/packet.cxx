#include <StormByte/network/packet.hxx>

#include <span>

using namespace StormByte::Network;

Packet::Packet(const unsigned short& opcode) noexcept: m_opcode(opcode) {}

const std::span<const std::byte> Packet::Data() const noexcept {
	this->PrepareBuffer();
	return m_buffer.Data();
}