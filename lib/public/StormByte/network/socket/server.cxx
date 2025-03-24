#include <StormByte/network/socket/server.hxx>

#ifdef LINUX
#include <netinet/in.h>
#include <unistd.h>
#else
#include <ws2tcpip.h>
#endif

#include <format>

using namespace StormByte::Network::Socket;

Server::Server(const Connection::Protocol& protocol, std::shared_ptr<const Connection::Handler> handler, std::shared_ptr<Logger::Log> logger) noexcept:
Socket(protocol, handler, logger) {}

StormByte::Expected<void, StormByte::Network::ConnectionError> Server::Listen(const std::string& hostname, const unsigned short& port) noexcept {
	if (Connection::IsConnected(m_status))
		return StormByte::Unexpected<ConnectionError>("Server is already connected");
	
	m_status = Connection::Status::Connecting;

	auto expected_socket = CreateSocket();
	if (!expected_socket)
		return StormByte::Unexpected(expected_socket.error());

	m_handle = std::make_unique<Connection::Handler::Type>(expected_socket.value());

	// Set SO_REUSEADDR to allow reuse of the address
	int opt = 1;
	if (setsockopt(*m_handle, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt)) < 0) {
		m_status = Connection::Status::Disconnected;
		m_handle.reset();
		return StormByte::Unexpected<ConnectionError>(std::format("Failed to set socket options: {} (error code: {})",
													m_conn_handler->LastError(),
													m_conn_handler->LastErrorCode()));
	}

	auto expected_connection_info = Connection::Info::FromHost(hostname, port, m_protocol, m_conn_handler);
	if (!expected_connection_info)
		return StormByte::Unexpected<ConnectionError>(std::format("Failed to resolve hostname: {} (error code: {})",
													m_conn_handler->LastError(),
													m_conn_handler->LastErrorCode()));

	m_conn_info = std::make_unique<Connection::Info>(std::move(expected_connection_info.value()));

	auto bind_result = ::bind(*m_handle, m_conn_info->SockAddr().get(), sizeof(*m_conn_info->SockAddr()));
	if (bind_result == -1) {
		m_status = Connection::Status::Disconnected;
		m_handle.reset();
		return StormByte::Unexpected<ConnectionError>(std::format("Failed to bind socket: {} (error code: {})",
													m_conn_handler->LastError(),
													m_conn_handler->LastErrorCode()));
	}

	auto listen_result = ::listen(*m_handle, SOMAXCONN);
	if (listen_result == -1) {
		m_status = Connection::Status::Disconnected;
		m_handle.reset();
		return StormByte::Unexpected<ConnectionError>(std::format("Failed to listen on socket: {} (error code: {})",
													m_conn_handler->LastError(),
													m_conn_handler->LastErrorCode()));
	}

	InitializeAfterConnect();

	m_logger << Logger::Level::LowLevel << "Server listening on " << hostname << ":" << port << std::endl;

	return {};
}

StormByte::Expected<Client, StormByte::Network::ConnectionError> Server::Accept() noexcept {
	if (!Connection::IsConnected(m_status))
		return StormByte::Unexpected<ConnectionError>("Socket is not connected");

	fd_set read_fds;
	FD_ZERO(&read_fds);
	FD_SET(*m_handle, &read_fds);

	struct timeval timeout = {5, 0}; // 5 seconds timeout
	int select_result = select(*m_handle + 1, &read_fds, nullptr, nullptr, &timeout);
	if (select_result == 0) {
		return StormByte::Unexpected<ConnectionError>("Timeout occurred while waiting to accept connection.");
	} else if (select_result < 0) {
		return StormByte::Unexpected<ConnectionError>("Error during select.");
	}

	Connection::Handler::Type client_handle = ::accept(*m_handle, nullptr, nullptr);
	if (client_handle == -1) {
		return StormByte::Unexpected<ConnectionError>("Failed to accept client connection.");
	}

	Client client_socket(m_protocol, m_conn_handler, m_logger);
	client_socket.m_handle = std::make_unique<Connection::Handler::Type>(client_handle);
	client_socket.InitializeAfterConnect();

	return client_socket;
}
