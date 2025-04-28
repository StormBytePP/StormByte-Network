#include <StormByte/network/packet.hxx>
#include <StormByte/serializable.hxx>

using namespace StormByte::Network;

Packet::Packet(const unsigned short& opcode) noexcept:
m_opcode(opcode) {
	auto opcode_serial = Serializable<unsigned short>(opcode);
	m_buffer << opcode_serial.Serialize();
}

const StormByte::Buffers::ConstByteSpan Packet::Data() const noexcept {
	return m_buffer.Span();
}

const unsigned short& Packet::Opcode() const noexcept {
	return m_opcode;
}

ExpectedPacket Packet::Read(const PacketInstanceFunction& pif, PacketReaderFunction reader) noexcept {
	auto opcode_buffer = reader(sizeof(unsigned short));
	if (!opcode_buffer) {
		return StormByte::Unexpected<PacketError>("Failed to read opcode");
	}
	auto opcode_serial = Serializable<unsigned short>::Deserialize(opcode_buffer.value());
	if (!opcode_serial) {
		return StormByte::Unexpected<PacketError>("Failed to deserialize opcode");
	}
	const unsigned short opcode = opcode_serial.value();
	std::shared_ptr<Packet> packet = pif(opcode);
	if (!packet) {
		return StormByte::Unexpected<PacketError>("Unknown opcode {}", opcode);
	}
	auto initialize_result = packet->Initialize(reader);
	if (!initialize_result) {
		return StormByte::Unexpected(initialize_result.error());
	}
	return packet;
}