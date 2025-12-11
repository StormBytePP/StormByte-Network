#include <StormByte/network/connection/client.hxx>
#include <StormByte/network/client.hxx>
#include <StormByte/network/transport/frame.hxx>
#include <StormByte/network/transport/packet.hxx>

using namespace StormByte::Network;

Client::~Client() noexcept {
	Disconnect(); // Not really needed but explicit to make sure logger is not destructed before disconnection
}

bool Client::Connect(const Connection::Protocol& protocol, const std::string& address, const unsigned short& port) {
	if (m_connection) {
		m_logger << Logger::Level::Error << "Client is already connected." << std::endl;
	}

	try {
		std::shared_ptr<Socket::Client> socket = std::make_shared<Socket::Client>(protocol, m_logger);

		if (!socket->Connect(address, port)) {
			m_logger << Logger::Level::Error << "Failed to connect to " << address << ":" << port << " using protocol " << Connection::ProtocolString(protocol) << std::endl;
			return false;
		}

		m_connection = CreateConnection(socket);
		m_logger << Logger::Level::LowLevel << "Successfully connected to " << address << ":" << port << " using protocol " << Connection::ProtocolString(protocol) << std::endl;
		return true;
	} catch (const std::bad_alloc& bd) {
		m_logger << Logger::Level::Error << "Failed to allocate memory for socket: " << bd.what() << std::endl;
		return false;
	}
}

void Client::Disconnect() noexcept {
	if (m_connection) {
		m_logger << Logger::Level::LowLevel << "Disconnecting client." << std::endl;
		m_connection.reset();
	}
}

Connection::Status Client::Status() const noexcept {
	return m_connection ? m_connection->Status() : Connection::Status::Disconnected;
}

PacketPointer Client::Send(const Transport::Packet& packet) noexcept {
	return Endpoint::Send(m_connection, packet);
}