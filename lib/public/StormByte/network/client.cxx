#include <StormByte/network/client.hxx>

using namespace StormByte::Network;

Client::Client(std::shared_ptr<const Connection::Handler> handler, std::shared_ptr<Logger> logger, const PacketInstanceFunction& pf) noexcept:
m_logger(logger), m_socket(nullptr), m_status(Connection::Status::Disconnected), m_handler(handler), m_packet_instance_function(pf) {}

Client::~Client() noexcept {
	Disconnect();
}

StormByte::Expected<void, ConnectionError> Client::Connect(const std::string& hostname, const unsigned short& port, const Connection::Protocol& protocol) noexcept {
	if (Connection::IsConnected(m_status))
		return StormByte::Unexpected<ConnectionError>("Client is already connected");

	m_socket = std::make_unique<Socket::Client>(protocol, m_handler, m_logger);
	auto expected_connect = m_socket->Connect(hostname, port);
	if (!expected_connect)
		return StormByte::Unexpected(expected_connect.error());

	m_status = Connection::Status::Connected;

	return {};
}

void Client::Disconnect() noexcept {
	if (!Connection::IsConnected(m_status))
		return;

	m_status = Connection::Status::Disconnecting;
	m_socket.reset();
	m_status = Connection::Status::Disconnected;
}

ExpectedVoid Client::Send(const Packet& packet) noexcept {
	if (!Connection::IsConnected(m_status))
		return StormByte::Unexpected<ConnectionError>("Client is not connected");

	auto expected_write = m_socket->Send(packet);
	if (!expected_write)
		return StormByte::Unexpected(expected_write.error());

	return {};
}

ExpectedPacket Client::Receive() noexcept {
	if (!Connection::IsConnected(m_status))
		return StormByte::Unexpected<PacketError>("Client is not connected");

	auto expected_packet = Packet::Read(m_packet_instance_function, *m_socket);
	if (!expected_packet)
		return StormByte::Unexpected<PacketError>(expected_packet.error());

	return expected_packet;
}