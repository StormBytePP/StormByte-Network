#include <StormByte/network/packet.hxx>
#include <StormByte/serializable.hxx>

using namespace StormByte::Network;

Packet::Packet(const unsigned short& opcode) noexcept: m_opcode(opcode) {}

const unsigned short& Packet::Opcode() const noexcept {
	return m_opcode;
}

StormByte::Buffer::FIFO Packet::Serialize() const noexcept {
	auto opcode_serial = Serializable<unsigned short>(m_opcode);
	Buffer::FIFO result(opcode_serial.Serialize());
	result.Write(DoSerialize());
	return result;
}