#include <StormByte/network/transport/packet.hxx>
#include <StormByte/serializable.hxx>

using StormByte::Buffer::DataType;
using StormByte::Buffer::FIFO;
using namespace StormByte::Network::Transport;

FIFO Packet::Serialize() const noexcept {
	FIFO result;

	result.Write(Serializable<OpcodeType>(m_opcode).Serialize());

	DataType payload = DoSerialize();
	if (!payload.empty()) {
		result.Write(std::move(payload));
	}

	return result;
}
