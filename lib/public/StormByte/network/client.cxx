#include <StormByte/network/client.hxx>
#include <StormByte/network/socket/client.hxx>

using namespace StormByte::Network;

Client::Client(const Connection::Protocol& protocol, std::shared_ptr<Connection::Handler> handler, std::shared_ptr<Logger> logger, const PacketInstanceFunction& pf) noexcept:
EndPoint(protocol, handler, logger), m_packet_instance_function(pf) {
	m_socket = new Socket::Client(protocol, handler, logger);
}

Client::~Client() noexcept {
	Disconnect();
}

StormByte::Expected<void, ConnectionError> Client::Connect(const std::string& hostname, const unsigned short& port, const Connection::Protocol& protocol) noexcept {
	if (Connection::IsConnected(m_status))
		return StormByte::Unexpected<ConnectionError>("Client is already connected");

	auto expected_connect = static_cast<Socket::Client*>(m_socket)->Connect(hostname, port);
	if (!expected_connect)
		return StormByte::Unexpected(expected_connect.error());

	m_status = Connection::Status::Connected;

	return {};
}

void Client::Disconnect() noexcept {
	if (!Connection::IsConnected(m_status))
		return;

	m_status = Connection::Status::Disconnecting;
	m_socket->Disconnect();
	m_status = Connection::Status::Disconnected;
}

ExpectedVoid Client::Send(const Packet& packet) noexcept {
	if (!Connection::IsConnected(m_status))
		return StormByte::Unexpected<ConnectionError>("Client is not connected");

	auto expected_write = static_cast<Socket::Client*>(m_socket)->Send(packet);
	if (!expected_write)
		return StormByte::Unexpected(expected_write.error());

	return {};
}

ExpectedPacket Client::Receive() noexcept {
	if (!Connection::IsConnected(m_status))
		return StormByte::Unexpected<PacketError>("Client is not connected");

	auto expected_packet = Packet::Read(
		m_packet_instance_function,
		[this](const size_t& size) -> ExpectedBuffer {
			auto expected_buffer = static_cast<Socket::Client*>(m_socket)->Receive(size);
			if (!expected_buffer)
				return StormByte::Unexpected<ConnectionError>(expected_buffer.error()->what());
			return expected_buffer.value().get();
		}
	);
	if (!expected_packet)
		return StormByte::Unexpected<PacketError>(expected_packet.error());

	return expected_packet;
}