#include <StormByte/network/transport/packet.hxx>
#include <StormByte/serializable.hxx>

using StormByte::Buffer::DataType;
using StormByte::Buffer::FIFO;
using namespace StormByte::Network::Transport;

FIFO Packet::Serialize() const noexcept {
	FIFO result;

	// Write opcode
	result.Write(Serializable<OpcodeType>(m_opcode).Serialize());

	// Get payload
	DataType payload = DoSerialize();

	// Write payload (if not empty)
	if (!payload.empty())
		result.Write(std::move(payload));

	return result;
}