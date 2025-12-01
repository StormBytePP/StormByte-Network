#include <StormByte/network/packet.hxx>
#include <StormByte/serializable.hxx>

using namespace StormByte::Network;

Packet::Packet(const unsigned short& opcode) noexcept: m_opcode(opcode) {}

Packet::~Packet() noexcept = default;

const unsigned short& Packet::Opcode() const noexcept {
	return m_opcode;
}

StormByte::Buffer::FIFO Packet::DoSerialize() const noexcept {
	auto opcode_serial = Serializable<unsigned short>(m_opcode);
	return opcode_serial.Serialize();
}