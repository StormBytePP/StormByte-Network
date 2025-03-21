#include <StormByte/network/socket/client.hxx>
#include <StormByte/network/socket/server.hxx>

#ifdef LINUX
#include <netinet/in.h>
#include <unistd.h>
#else
#include <ws2tcpip.h>
#endif

#include <iostream>
#include <format>

using namespace StormByte::Network::Socket;

Server::Server(const Connection::Protocol& protocol, std::shared_ptr<const Connection::Handler> handler): Socket(protocol, handler) {}

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
        return StormByte::Unexpected<ConnectionError>(std::format("Failed to set socket options: {}", m_conn_handler->LastError()));
    }

    auto expected_connection_info = Connection::Info::FromHost(hostname, port, m_protocol, m_conn_handler);
    if (!expected_connection_info)
        return StormByte::Unexpected<ConnectionError>(std::format("Failed to resolve hostname: {}", m_conn_handler->LastError()));

    m_conn_info = std::make_unique<Connection::Info>(std::move(expected_connection_info.value()));
	InitializeMTU();

    auto bind_result = ::bind(*m_handle, m_conn_info->SockAddr().get(), sizeof(*m_conn_info->SockAddr()));
    if (bind_result == -1) {
        m_status = Connection::Status::Disconnected;
        m_handle.reset();
        return StormByte::Unexpected<ConnectionError>(std::format("Failed to bind socket: {}", m_conn_handler->LastError()));
    }

    auto listen_result = ::listen(*m_handle, SOMAXCONN);
    if (listen_result == -1) {
        m_status = Connection::Status::Disconnected;
        m_handle.reset();
        return StormByte::Unexpected<ConnectionError>(std::format("Failed to listen on socket: {}", m_conn_handler->LastError()));
    }

    m_status = Connection::Status::Connected;

    return {};
}

StormByte::Expected<Client, StormByte::Network::ConnectionError> Server::Accept() noexcept {
	if (!Connection::IsConnected(m_status))
		return StormByte::Unexpected<ConnectionError>("Socket is not connected");

	auto sock_addr = std::make_shared<sockaddr>();
	socklen_t sock_addr_len = sizeof(sockaddr_in);  // Use the appropriate size

	Connection::Handler::Type client_handle = ::accept(*m_handle, sock_addr.get(), &sock_addr_len);
	if (client_handle == -1)
		return StormByte::Unexpected<ConnectionError>(std::format("Failed to accept client: {}", m_conn_handler->LastError()));

    auto expected_conn_info = Connection::Info::FromSockAddr(sock_addr);
    if (!expected_conn_info)
        return StormByte::Unexpected<ConnectionError>(std::format("Failed to create connection info: {}", expected_conn_info.error()->what()));

	Client client_socket(m_protocol, m_conn_handler);
	client_socket.Handle() = std::make_unique<Connection::Handler::Type>(client_handle);
	client_socket.Status() = Connection::Status::Connected;
	client_socket.ConnInfo() = std::make_unique<Connection::Info>(std::move(expected_conn_info.value()));
	client_socket.ReadMode(Connection::ReadMode::NonBlocking);
	std::cout << "Accepted client!" << std::endl;
	return client_socket;
}
