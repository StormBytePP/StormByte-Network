#include <StormByte/network/socket/server.hxx>

#ifdef LINUX
#include <unistd.h>
#include <cstring> // For strerror
#endif

#include <format>

using namespace StormByte::Network::Socket;

Server::Server(const Address& address, std::shared_ptr<const Handler> handler): Socket(address, handler) {}

StormByte::Expected<void, StormByte::Network::ConnectionError> Server::Listen() noexcept {
	if (m_status != Status::Disconnected)
		return StormByte::Unexpected<ConnectionError>("Server is already connected");
	
	m_status = Status::Connecting;

	Handler::Type server_handle = socket(AF_INET, SOCK_STREAM, 0);
	std::string error_string = m_handler->LastError();
	if (!error_string.empty()) {
		m_status = Status::Disconnected;
		return StormByte::Unexpected<ConnectionError>(error_string);
	}
	m_handle = std::make_unique<Handler::Type>(server_handle);

	bind(*m_handle, m_address.SockAddr(), sizeof(m_address.SockAddr()));
	error_string = m_handler->LastError();
	if (!error_string.empty()) {
		m_status = Status::Disconnected;
		m_handle.reset();
		return StormByte::Unexpected<ConnectionError>(std::format("Failed to bind socket: {}", error_string));
	}

	m_status = Status::Connected;

	return {};
}

StormByte::Expected<Client, StormByte::Network::ConnectionError> Server::Accept() noexcept {
	if (m_status != Status::Connected)
		return StormByte::Unexpected<ConnectionError>("Socket is not connected");

	Handler::Type client_handle = accept(*m_handle, nullptr, nullptr);
	const std::string error_string = m_handler->LastError();
	if (!error_string.empty())
		return StormByte::Unexpected<ConnectionError>(error_string);

	Client client_socket(m_address, m_handler);
	client_socket.m_handle = std::make_unique<Handler::Type>(client_handle);
	client_socket.m_status = Status::Connected;

	return client_socket;
}
