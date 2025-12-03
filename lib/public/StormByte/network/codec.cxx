#include <StormByte/network/codec.hxx>
#include <StormByte/serializable.hxx>

using namespace StormByte::Network;

Codec::Codec(const Logger::ThreadedLog& log) noexcept: m_log(log) {}

ExpectedPacket Codec::Encode(const Buffer::Consumer& consumer, std::shared_ptr<Buffer::Pipeline> pipeline) const noexcept {
	// Expected format: [opcode (2 bytes)] [size (8 bytes)] [packet data...]
	// Read opcode
	auto opcode_bytes_expected = consumer.Read(sizeof(unsigned short));
	if (!opcode_bytes_expected) {
		return StormByte::Unexpected<PacketError>("Codec::Encode: failed to extract opcode bytes from consumer ({})", opcode_bytes_expected.error()->what());
	}
	const auto& opcode_bytes = opcode_bytes_expected.value();
	auto opcode_expected = Serializable<unsigned short>::Deserialize(opcode_bytes);
	if (!opcode_expected) {
		return StormByte::Unexpected<PacketError>("Codec::Encode: failed to deserialize opcode ({})", opcode_expected.error()->what());
	}
	const unsigned short& opcode = *opcode_expected;
	m_log << Logger::Level::LowLevel << "Codec::Encode: parsed opcode=" << opcode << std::endl;

	// Read size
	auto size_bytes_expected = consumer.Read(sizeof(std::size_t));
	if (!size_bytes_expected) {
		return StormByte::Unexpected<PacketError>("Codec::Encode: failed to extract size bytes from consumer ({})", size_bytes_expected.error()->what());
	}
	const auto& size_bytes = size_bytes_expected.value();
	auto size_expected = Serializable<std::size_t>::Deserialize(size_bytes);
	if (!size_expected) {
		return StormByte::Unexpected<PacketError>("Codec::Encode: failed to deserialize size ({})", size_expected.error()->what());
	}
	const std::size_t& size = *size_expected;
	m_log << Logger::Level::LowLevel << "Codec::Encode: payload size=" << Logger::humanreadable_bytes << size << Logger::nohumanreadable << std::endl;

	Buffer::Consumer processed = pipeline->Process(consumer, Buffer::ExecutionMode::Async, m_log);
	m_log << Logger::Level::LowLevel << "Codec::Encode: pipeline processed consumer, delegating DoEncode" << std::endl;
	auto result = DoEncode(opcode, size, processed);
	if (result) {
		m_log << Logger::Level::LowLevel << "Codec::Encode: DoEncode succeeded, returning packet opcode=" << result.value()->Opcode() << std::endl;
	} else {
		m_log << Logger::Level::LowLevel << "Codec::Encode: DoEncode failed error=" << result.error()->what() << std::endl;
	}
	return result;
}

ExpectedConsumer Codec::Process(const Packet& packet, std::shared_ptr<Buffer::Pipeline> pipeline) const noexcept {
	// Out format: [opcode (2 bytes)] [size (8 bytes)] [packet data...]
	Buffer::Producer final_producer;

	// Get all packet data
	Buffer::FIFO packet_buffer = packet.Serialize();
	m_log << Logger::Level::LowLevel << "Codec::Process: serialize packet opcode=" << packet.Opcode() << std::endl;

	// Get opcode data
	auto opcode_bytes_expected = packet_buffer.Extract(sizeof(unsigned short));
	if (!opcode_bytes_expected) {
		return StormByte::Unexpected<PacketError>("Codec::Process: failed to extract opcode bytes from packet serialization ({})", opcode_bytes_expected.error()->what());
	}
	const auto& opcode_bytes = opcode_bytes_expected.value();
	auto opcode_expected = Serializable<unsigned short>::Deserialize(opcode_bytes);
	if (!opcode_expected) {
		return StormByte::Unexpected<PacketError>("Codec::Process: failed to deserialize opcode ({})", opcode_expected.error()->what());
	}

	// Write opcode to final producer
	(void)final_producer.Write(std::move(opcode_bytes));
	m_log << Logger::Level::LowLevel << "Codec::Process: wrote opcode to output" << std::endl;

	// Get packet data (may be empty)
	std::vector<std::byte> packet_data;
	if (packet_buffer.Size() > 0) {
		auto packet_data_expected = packet_buffer.Extract();
		if (!packet_data_expected) {
			return StormByte::Unexpected<PacketError>("Codec::Process: failed to extract packet data from packet serialization ({})", packet_data_expected.error()->what());
		}
		Buffer::Producer packet_data_producer;
		(void)packet_data_producer.Write(std::move(*packet_data_expected));
		Buffer::Consumer processed_packet_data = pipeline->Process(packet_data_producer.Consumer(), Buffer::ExecutionMode::Async, m_log);
		auto data = processed_packet_data.ExtractUntilEoF();
		auto data_expected = data.Extract();
		if (!data_expected) {
			return StormByte::Unexpected<PacketError>("Codec::Process: failed to extract processed packet data after pipeline ({})", data_expected.error()->what());
		}
		packet_data = std::move(data_expected.value());
		m_log << Logger::Level::LowLevel << "Codec::Process: payload after pipeline size=" << Logger::humanreadable_bytes << packet_data.size() << Logger::nohumanreadable << std::endl;
	}

	// Write size of packet data
	auto size_bytes_data = Serializable<std::size_t>(packet_data.size()).Serialize();
	(void)final_producer.Write(std::move(size_bytes_data));
	m_log << Logger::Level::LowLevel << "Codec::Process: wrote payload size=" << Logger::humanreadable_bytes << packet_data.size() << Logger::nohumanreadable << std::endl;

	// Write packet data
	(void)final_producer.Write(std::move(packet_data));
	m_log << Logger::Level::LowLevel << "Codec::Process: wrote payload bytes" << std::endl;

	// Close and return
	final_producer.Close();
	m_log << Logger::Level::LowLevel << "Codec::Process: final consumer ready" << std::endl;
	return final_producer.Consumer();
}