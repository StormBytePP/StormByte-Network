#include <StormByte/network/codec.hxx>
#include <StormByte/serializable.hxx>

using namespace StormByte::Network;

Codec::Codec(const Logger::ThreadedLog& log) noexcept: m_log(log) {}

ExpectedPacket Codec::Encode(const Buffer::Consumer& consumer) const noexcept {
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

	return DoEncode(opcode, size, consumer);
}

StormByte::Buffer::Consumer Codec::Process(const Packet& packet) const noexcept {
	// Out format: [opcode (2 bytes)] [size (8 bytes)] [packet data...]
	Buffer::Producer final_producer;

	// Get all packet data
	Buffer::FIFO packet_buffer = packet.Serialize();

	// Get opcode data
	auto opcode_bytes_expected = packet_buffer.Extract(sizeof(unsigned short));
	if (!opcode_bytes_expected) {
		m_log << Logger::Level::Error << "Codec::Process: failed to extract opcode bytes from packet serialization (" << opcode_bytes_expected.error()->what() << ")" << std::endl;
		final_producer.SetError();
		return final_producer.Consumer();
	}
	const auto& opcode_bytes = opcode_bytes_expected.value();
	auto opcode_expected = Serializable<unsigned short>::Deserialize(opcode_bytes);
	if (!opcode_expected) {
		m_log << Logger::Level::Error << "Codec::Process: failed to deserialize opcode (" << opcode_expected.error()->what() << ")" << std::endl;
		final_producer.SetError();
		return final_producer.Consumer();
	}
	const unsigned short& opcode = *opcode_expected;

	// Write opcode to final producer
	final_producer.Write(opcode_bytes);

	// Get packet data
	auto packet_data_expected = packet_buffer.Extract();
	if (!packet_data_expected) {
		m_log << Logger::Level::Error << "Codec::Process: failed to extract packet data from packet serialization (" << packet_data_expected.error()->what() << ")" << std::endl;
		final_producer.SetError();
		return final_producer.Consumer();
	}
	auto packet_data = std::move(packet_data_expected.value());

	// Write size of packet data
	auto size_bytes_data = Serializable<std::size_t>(packet_data.size()).Serialize();
	final_producer.Write(size_bytes_data);

	// Write packet data
	final_producer.Write(packet_data);

	// Close and return
	final_producer.Close();
	return final_producer.Consumer();
}