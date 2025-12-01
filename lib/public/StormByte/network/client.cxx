#include <StormByte/network/client.hxx>
#include <StormByte/network/socket/client.hxx>

using namespace StormByte::Network;

Client::Client(const Connection::Protocol& protocol, Logger::ThreadedLog logger) noexcept:
EndPoint(protocol, logger) {}

ExpectedVoid Client::Connect(const std::string& hostname, const unsigned short& port) noexcept {
	if (Connection::IsConnected(m_status))
		return StormByte::Unexpected<ConnectionError>("Client is already connected");

	try {
		m_socket = new Socket::Client(m_protocol, m_logger);
	}
	catch (const std::bad_alloc& e) {
		return StormByte::Unexpected<ConnectionError>("Can not create connection: {}", e.what());
	}

	m_status = Connection::Status::Connecting;
	auto expected_connect = static_cast<Socket::Client&>(*m_socket).Connect(hostname, port);
	if (!expected_connect) {
		delete m_socket;
		m_socket = nullptr;
		m_status = Connection::Status::Disconnected;
		return StormByte::Unexpected(expected_connect.error());
	}
	m_status = Connection::Status::Connected;
	return {};
}

ExpectedPacket Client::Receive() noexcept {
	return EndPoint::Receive(static_cast<Socket::Client&>(*m_socket));
}

ExpectedVoid Client::Send(const Packet& packet) noexcept {
	return EndPoint::Send(static_cast<Socket::Client&>(*m_socket), packet);
}