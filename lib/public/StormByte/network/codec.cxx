#include <StormByte/network/codec.hxx>

using namespace StormByte::Network;

std::vector<std::byte> Codec::Process(const Packet& packet) noexcept {
	Buffer::Producer producer;
	producer.Write(packet.Serialize());
	producer.Close();
	auto postprocess_buf = m_out_pipeline.Process(producer.Consumer(), Buffer::ExecutionMode::Sync, m_log);
	std::vector<std::byte> result;
	do {
		auto chunk = postprocess_buf.Read();
		if (!chunk)
			break;
		producer.Write(chunk.value());
	} while (true);
	return result;
}