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
		[&client, &pipeline](const size_t& size) -> ExpectedBuffer {
			auto expected_buffer = client.Receive(size);
			if (!expected_buffer)
				return StormByte::Unexpected<ConnectionError>(expected_buffer.error()->what());
			Buffer::Producer pipeline_buffer(expected_buffer.value().get());
			pipeline_buffer << Buffer::Status::ReadOnly;
			auto pipeline_buffer_result = pipeline.Process(pipeline_buffer.Consumer());
			if (!pipeline_buffer_result.IsReadable())
				return StormByte::Unexpected<ConnectionError>("Error processing input pipeline");
			auto expected_processed_buffer = pipeline_buffer_result.Read(size);
			if (!expected_processed_buffer)
				return StormByte::Unexpected<ConnectionError>(expected_processed_buffer.error()->what());
			return expected_processed_buffer.value();
		}
	);
	if (!expected_packet)
		return StormByte::Unexpected<PacketError>(expected_packet.error());

	return expected_packet;
}

ExpectedVoid EndPoint::Send(Socket::Client& client, const Packet& packet, Buffer::Pipeline& pipeline) noexcept {
	Buffer::Producer packet_buffer(packet.Serialize());
	packet_buffer << Buffer::Status::ReadOnly;
	Buffer::Consumer pipeline_buffer = pipeline.Process(packet_buffer.Consumer());
	auto expected_write = client.Send(pipeline_buffer);
	if (!expected_write)
		return StormByte::Unexpected<ConnectionError>(expected_write.error()->what());

	return {};
}