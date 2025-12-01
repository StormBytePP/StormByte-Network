#include <StormByte/network/codec.hxx>
#include <StormByte/serializable.hxx>

using namespace StormByte::Network;

std::shared_ptr<Packet> Codec::Encode(Buffer::Consumer consumer) const noexcept {
	// First, read the opcode from the consumer
	auto opcode_data = consumer.Extract(sizeof(unsigned short));
	if (!opcode_data.has_value()) {
		// Not enough data to read opcode
		m_log << Logger::Level::LowLevel << "Codec::Encode: insufficient data to read opcode " << opcode_data.error()->what() << ")" << std::endl;
		return nullptr;
	}
	auto opcode_deserialized = Serializable<unsigned short>::Deserialize(*opcode_data);
	if (!opcode_deserialized.has_value()) {
		m_log << Logger::Level::LowLevel << "Codec::Encode: failed to deserialize opcode (" << opcode_deserialized.error()->what() << ")" << std::endl;
		return nullptr;
	}
	return DoEncode(*opcode_deserialized, m_in_pipeline.Process(consumer, Buffer::ExecutionMode::Sync, m_log));
}

StormByte::Buffer::FIFO Codec::Process(const Packet& packet) const noexcept {
	// Opcodes are never ran into pipeline as they must be sent/received raw
	auto opcode_serializer = Serializable<unsigned short>(packet.Opcode());
	Buffer::FIFO result = opcode_serializer.Serialize(), packet_buff = packet.Serialize();
	packet_buff.Skip(packet_buff.Size());
	Buffer::Producer producer;
	producer.Write(packet_buff);
	producer.Close();
	auto postprocess_buff = m_out_pipeline.Process(producer.Consumer(), Buffer::ExecutionMode::Sync, m_log);
	auto postprocess_all_data = postprocess_buff.ReadUntilEoF();
	result.Write(postprocess_buff.ReadUntilEoF());
	return result;
}