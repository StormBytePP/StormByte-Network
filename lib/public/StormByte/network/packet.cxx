#include <StormByte/network/packet.hxx>

#include <span>

using namespace StormByte::Network;

Packet::Packet(const unsigned short& opcode) noexcept: m_opcode(opcode) {}

StormByte::Util::Buffer Packet::Data() const noexcept {
	return std::to_string(m_opcode);
}