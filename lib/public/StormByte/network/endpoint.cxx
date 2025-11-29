#include <StormByte/network/endpoint.hxx>
#include <StormByte/network/socket/client.hxx>

using namespace StormByte::Network;

EndPoint::EndPoint(const Connection::Protocol& protocol, std::shared_ptr<Connection::Handler> handler, std::shared_ptr<Logger> logger) noexcept:
m_protocol(protocol), m_handler(handler), m_logger(logger), m_status(Connection::Status::Disconnected), m_socket(nullptr) {}

EndPoint::~EndPoint() noexcept {
	Disconnect();
}

void EndPoint::Disconnect() noexcept {
	m_status.store(Connection::Status::Disconnecting);
	if (m_socket) {
		delete m_socket;
		m_socket = nullptr;
	}
	m_status.store(Connection::Status::Disconnected);
}

const Connection::Protocol& EndPoint::Protocol() const noexcept {
	return m_protocol;
}

Connection::Status EndPoint::Status() const noexcept {
	return m_status.load();
}

ExpectedVoid EndPoint::Handshake(Socket::Client&) noexcept {
	return {};
}

ExpectedVoid EndPoint::Authenticate(Socket::Client&) noexcept {
	return {};
}

ExpectedPacket EndPoint::Receive(Socket::Client& client) noexcept {
	if (!Connection::IsConnected(m_status))
		return StormByte::Unexpected<PacketError>("Client is not connected");

	return Receive(client, m_input_pipeline);
}

ExpectedPacket EndPoint::RawReceive(Socket::Client& client) noexcept {
	if (!Connection::IsConnected(m_status))
		return StormByte::Unexpected<PacketError>("Client is not connected");

	Buffer::Pipeline empty_pipeline;
	return Receive(client, empty_pipeline);
}

ExpectedVoid EndPoint::Send(Socket::Client& client, const Packet& packet) noexcept {
	return Send(client, packet, m_output_pipeline);
}

ExpectedVoid EndPoint::RawSend(Socket::Client& client, const Packet& packet) noexcept {
	Buffer::Pipeline empty_pipeline;
	return Send(client, packet, empty_pipeline);
}

ExpectedPacket EndPoint::Receive(Socket::Client& client, Buffer::Pipeline& pipeline) noexcept {
	auto expected_packet = Packet::Read(
		m_handler->PacketInstanceFunction(),
		[&client, &pipeline, logger = m_logger](const size_t& size) -> ExpectedBuffer {
			auto expected_buffer = client.Receive(size);
			if (!expected_buffer)
				return StormByte::Unexpected<ConnectionError>(expected_buffer.error()->what());
			Buffer::FIFO received_fifo = expected_buffer.value();
			// Extract all data from received FIFO
			auto expected_data = received_fifo.Extract();
			if (!expected_data)
				return StormByte::Unexpected<ConnectionError>("Failed to extract data from received buffer");
			// Create producer and write extracted data
			Buffer::Producer pipeline_input;
			pipeline_input.Write(expected_data.value());
			pipeline_input.Close();
			// Process through pipeline
			auto pipeline_buffer_result = pipeline.Process(pipeline_input.Consumer(), Buffer::ExecutionMode::Sync, logger);
			// Read processed data - block until we have the expected size or buffer is closed
			auto expected_processed_buffer = pipeline_buffer_result.Read(size);
			if (!expected_processed_buffer)
				return StormByte::Unexpected<ConnectionError>(expected_processed_buffer.error()->what());
			// Create result FIFO and write processed data
			Buffer::FIFO result;
			result.Write(expected_processed_buffer.value());
			return result;
		}
	);
	if (!expected_packet)
		return StormByte::Unexpected<PacketError>(expected_packet.error());

	return expected_packet;
}

ExpectedVoid EndPoint::Send(Socket::Client& client, const Packet& packet, Buffer::Pipeline& pipeline) noexcept {
	Buffer::Consumer packet_consumer = packet.Serialize();
	Buffer::Consumer pipeline_buffer = pipeline.Process(packet_consumer, Buffer::ExecutionMode::Async, m_logger);
	while (!pipeline_buffer.EoF()) {
		if (pipeline_buffer.AvailableBytes() == 0) {
			std::this_thread::yield();
			continue;
		}
		auto buff = pipeline_buffer.Extract();
		auto expected_write = client.Send(pipeline_buffer);
		if (!expected_write) {
			pipeline.SetError();
			return StormByte::Unexpected<ConnectionError>(expected_write.error()->what());
		}
	}
	

	return {};
}