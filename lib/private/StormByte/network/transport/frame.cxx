#include <StormByte/network/socket/client.hxx>
#include <StormByte/network/transport/frame.hxx>
#include <StormByte/serializable.hxx>

using StormByte::Buffer::Consumer;
using StormByte::Buffer::DataType;
using StormByte::Buffer::FIFO;
using StormByte::Buffer::Pipeline;
using StormByte::Buffer::Producer;
using namespace StormByte::Network::Transport;

Frame::Frame(const Packet& packet) noexcept {
	FIFO packet_raw = packet.Serialize();
	m_opcode = packet.Opcode();

	// Drop opcode data, we already have it
	packet_raw.Drop(sizeof(Packet::OpcodeType));

	// Read payload if not empty
	if (packet_raw.AvailableBytes() > 0) {
		packet_raw.Read(0, m_payload);
	}
}

Frame Frame::ProcessInput(std::shared_ptr<Socket::Client> client, Buffer::Pipeline& in_pipeline, Logger::Log& logger) noexcept {
	// Read opcode
	ExpectedBuffer expected_opcode_buffer = client->Receive(sizeof(Packet::OpcodeType));
	if (!expected_opcode_buffer) {
		logger << Logger::Level::Error << "Failed to read opcode from socket: " << expected_opcode_buffer.error()->what();
		return Frame();
	}

	auto expected_opcode = Serializable<Packet::OpcodeType>::Deserialize(expected_opcode_buffer->Data());
	if (!expected_opcode) {
		logger << Logger::Level::Error << "Failed to deserialize opcode from socket: insufficient data" << std::endl;
		return Frame();
	}
	const Packet::OpcodeType opcode = *expected_opcode;

	// Read payload size
	ExpectedBuffer expected_size_buffer = client->Receive(sizeof(std::size_t));
	if (!expected_size_buffer) {
		logger << Logger::Level::Error << "Failed to read payload size from socket: " << expected_size_buffer.error()->what() << std::endl;
		return Frame();
	}
	auto expected_payload_size = Serializable<std::size_t>::Deserialize(expected_size_buffer->Data());
	if (!expected_payload_size) {
		logger << Logger::Level::Error << "Failed to deserialize payload size from socket: insufficient data" << std::endl;
		return Frame();
	}
	std::size_t payload_size = expected_payload_size.value();
	DataType payload;

	if (payload_size > 0) {
		// Read payload data
		ExpectedBuffer expected_payload_buffer = client->Receive(payload_size);
		if (!expected_payload_buffer) {
			logger << Logger::Level::Error << "Failed to read full frame from socket: " << expected_payload_buffer.error()->what() << std::endl;
			return Frame();
		}

		expected_payload_buffer->Extract(payload);

		// Process payload if needed
		if (payload_size >= Packet::PROCESS_THRESHOLD) {
			Producer payload_producer;
			payload_producer.Write(std::move(payload));
			payload_producer.Close();
			Consumer processed_payload = in_pipeline.Process(payload_producer.Consumer(), Buffer::ExecutionMode::Async, logger);
			payload.clear();
			processed_payload.ExtractUntilEoF(payload);
		}
	}

	return Frame(opcode, std::move(payload));
}

StormByte::Network::PacketPointer Frame::ProcessPacket(const DeserializePacketFunction& packet_fn, Logger::Log& logger) const noexcept {
	Producer payload_producer;
	payload_producer.Write(m_payload);
	payload_producer.Close();

	PacketPointer packet = packet_fn(m_opcode, payload_producer.Consumer(), logger);

	return packet;
}

Consumer Frame::ProcessOutput(Buffer::Pipeline& pipeline, Logger::Log& logger) const noexcept {
	Producer producer;

	// Write opcode
	producer.Write(sizeof(Packet::OpcodeType), Serializable<Packet::OpcodeType>(m_opcode).Serialize());

	DataType payload = m_payload;

	// Process payload if needed
	if (m_opcode >= Packet::PROCESS_THRESHOLD) {
		Producer payload_producer;
		payload_producer.Write(std::move(payload));
		payload_producer.Close();
		Consumer processed_payload = pipeline.Process(payload_producer.Consumer(), Buffer::ExecutionMode::Async, logger);
		payload.clear();
		processed_payload.ExtractUntilEoF(payload);
	}
	
	// Write payload size
	producer.Write(sizeof(std::size_t), Serializable<std::size_t>(payload.size()).Serialize());

	// Write payload data
	if (!payload.empty()) {
		producer.Write(std::move(payload));
	}

	// Close producer and return consumer
	producer.Close();
	return producer.Consumer();
}