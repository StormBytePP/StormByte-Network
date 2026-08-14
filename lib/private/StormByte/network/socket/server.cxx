#include <StormByte/network/connection/handler.hxx>
#include <StormByte/network/socket/server.hxx>
#include <StormByte/system.hxx>

#ifdef LINUX
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>
#else
#include <ws2tcpip.h>
#endif

#include <format>
#include <vector>
#include <algorithm>

using namespace StormByte::Network;

Socket::Server::Server(const Connection::Protocol& protocol, std::shared_ptr<Logger::Log> logger) noexcept:
Socket(protocol, logger) {
	m_logger << Logger::Level::LowLevel << "Created server socket with UUID: " << m_UUID << std::endl;
}

ExpectedVoid Socket::Server::Listen(const std::string& hostname, const unsigned short& port) noexcept {
	if (Connection::IsConnected(m_status.load(std::memory_order_acquire)))
		return Unexpected<ConnectionError>("Server is already connected");

	m_status.store(Connection::Status::Connecting, std::memory_order_release);

	auto expected_socket = CreateSocket();
	if (!expected_socket)
		return Unexpected(expected_socket.error());

	m_handle = expected_socket.value();

	int opt = 1;
#ifdef WINDOWS
	{
		BOOL exclusive = TRUE;
		if (setsockopt(m_handle, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
				reinterpret_cast<const char*>(&exclusive), sizeof(exclusive)) == SOCKET_ERROR) {
			m_status.store(Connection::Status::Disconnected, std::memory_order_release);
			m_handle = INVALID_SOCKET;
			return Unexpected<ConnectionError>("Failed to set SO_EXCLUSIVEADDRUSE: {} (error code: {})",
				Connection::Handler::Instance().LastError(),
				Connection::Handler::Instance().LastErrorCode());
		}
	}
#else
	if (setsockopt(m_handle, SOL_SOCKET, SO_REUSEADDR,
			reinterpret_cast<const char*>(&opt), sizeof(opt)) < 0) {
		m_status.store(Connection::Status::Disconnected, std::memory_order_release);
		m_handle = -1;
		return Unexpected<ConnectionError>("Failed to set socket options: {} (error code: {})",
			Connection::Handler::Instance().LastError(),
			Connection::Handler::Instance().LastErrorCode());
	}
#endif

	auto expected_connection_info = Connection::Info::FromHost(hostname, port, m_protocol);
	if (!expected_connection_info)
		return Unexpected<ConnectionError>(expected_connection_info.error()->what());

	m_conn_info = std::make_unique<Connection::Info>(std::move(expected_connection_info.value()));

	auto bind_result = ::bind(m_handle, m_conn_info->SockAddr().get(), sizeof(*m_conn_info->SockAddr()));
	if (bind_result == -1) {
		m_status.store(Connection::Status::Disconnected, std::memory_order_release);
#ifdef WINDOWS
		m_handle = INVALID_SOCKET;
#else
		m_handle = -1;
#endif
		return Unexpected<ConnectionError>("Failed to bind socket: {} (error code: {})",
			Connection::Handler::Instance().LastError(),
			Connection::Handler::Instance().LastErrorCode());
	}

	auto listen_result = ::listen(m_handle, SOMAXCONN);
	if (listen_result == -1) {
		m_status.store(Connection::Status::Disconnected, std::memory_order_release);
#ifdef WINDOWS
		m_handle = INVALID_SOCKET;
#else
		m_handle = -1;
#endif
		return Unexpected<ConnectionError>("Failed to listen on socket: {} (error code: {})",
			Connection::Handler::Instance().LastError(),
			Connection::Handler::Instance().LastErrorCode());
	}

	InitializeAfterConnect();

	m_logger << Logger::Level::LowLevel << "Server listening on " << hostname << ":" << port << std::endl;

	return {};
}

ExpectedClient Socket::Server::Accept() noexcept {
	if (!Connection::IsConnected(m_status.load(std::memory_order_acquire)))
		return Unexpected<ConnectionError>("Socket is not connected");

#ifdef LINUX
	// poll avoids FD_SETSIZE issues and is cheaper than select for one fd
	struct pollfd pfd;
	pfd.fd = m_handle;
	pfd.events = POLLIN;
	int pr = poll(&pfd, 1, 200); // 200ms — same responsiveness as before
	if (pr == 0) {
		return Unexpected<ConnectionError>("Timeout occurred while waiting to accept connection.");
	} else if (pr < 0) {
		return Unexpected<ConnectionError>("Error during poll.");
	}
#else
	fd_set read_fds;
	FD_ZERO(&read_fds);
	FD_SET(m_handle, &read_fds);
	struct timeval timeout = {0, 200000}; // 200ms
	// On Windows the nfds argument is ignored
	int select_result = select(0, &read_fds, nullptr, nullptr, &timeout);
	if (select_result == 0) {
		return Unexpected<ConnectionError>("Timeout occurred while waiting to accept connection.");
	} else if (select_result < 0) {
		return Unexpected<ConnectionError>("Error during select.");
	}
#endif

	Connection::HandlerType client_handle = ::accept(m_handle, nullptr, nullptr);
#ifdef WINDOWS
	if (client_handle == INVALID_SOCKET) {
#else
	if (client_handle == -1) {
#endif
		return Unexpected<ConnectionError>("Failed to accept client connection.");
	}

	Client client_socket(m_protocol, m_logger);
	client_socket.m_handle = client_handle;
	client_socket.InitializeAfterConnect();

	m_active_clients.push_back(std::make_shared<Client>(std::move(client_socket)));
	return m_active_clients.back();
}

void Socket::Server::Disconnect() noexcept {
	for (auto& client : m_active_clients) {
		if (!client) continue;
		client->Disconnect();
	}
	m_active_clients.clear();

	Socket::Disconnect();
}

void Socket::Server::DisconnectClient(const std::string& client_uuid) noexcept {
	auto it = std::find_if(m_active_clients.begin(), m_active_clients.end(),
		[&client_uuid](const std::shared_ptr<Client>& client) {
			return client && client->UUID() == client_uuid;
		});
	if (it != m_active_clients.end()) {
		(*it)->Disconnect();
		m_active_clients.erase(it);
	}
}
