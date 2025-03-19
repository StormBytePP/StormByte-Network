#include <StormByte/network/address.hxx>
#include <StormByte/network/server.hxx>

#ifdef LINUX
#include <sys/select.h>
#include <unistd.h>
#include <cstring> // For strerror
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <format>

using namespace StormByte::Network;

Server::Server(const Address& address, std::shared_ptr<const Handler> handler): m_address(std::make_unique<Address>(address)), m_socket(nullptr), m_status(Status::Disconnected), m_handler(handler) {}

Server::Server(Address&& address, std::shared_ptr<const Handler> handler): m_address(std::make_unique<Address>(std::move(address))), m_socket(nullptr), m_status(Status::Disconnected), m_handler(handler) {}

Server::~Server() noexcept {
	Disconnect();
}

StormByte::Expected<void, ConnectionError> Server::Connect() noexcept {
	if (m_status.load() != Status::Disconnected)
		return StormByte::Unexpected<ConnectionError>("Server is already connected");

	m_socket = std::make_unique<Socket::Server>(*m_address, m_handler);
	auto expected_listen = m_socket->Listen();
	if (!expected_listen)
		return StormByte::Unexpected(expected_listen.error());
	
	// Start a thread to accept client connections
    m_acceptThread = std::thread(&Server::AcceptClients, this);

	// Start a thread to handle client messages
    m_messageThread = std::thread([this]() {
        while (m_status.load() == Status::Connected) {
            HandleClientMessages();
        }
    });

	return {};
}

void Server::Disconnect() noexcept {
	if (m_status.load() != Status::Connected)
		return;

	m_status.store(Status::Disconnecting);

	if (m_acceptThread.joinable()) {
		m_acceptThread.join();
	}
	if (m_messageThread.joinable()) {
        m_messageThread.join();
    }
	RemoveAllClients();
	m_socket.reset();

	m_status.store(Status::Disconnected);
}

void Server::AcceptClients() {
	while (m_status.load() == Status::Connected) {
		auto expected_client_socket = m_socket->Accept();
		if (expected_client_socket) {
			AddClient(std::move(expected_client_socket.value()));
		}
	}
}

void Server::AddClient(Socket::Client&& client) {
	std::lock_guard<std::mutex> lock(m_clientsMutex);
	m_clients.push_back(std::make_unique<Socket::Client>(std::move(client)));
}

void Server::RemoveClient(std::unique_ptr<Socket::Client>& client) noexcept {
	std::lock_guard<std::mutex> lock(m_clientsMutex);
	m_clients.erase(std::remove_if(m_clients.begin(), m_clients.end(), [&client](const std::unique_ptr<Socket::Client>& c) {
		return c.get() == client.get();
	}), m_clients.end());
}

void Server::RemoveAllClients() noexcept {
	std::lock_guard<std::mutex> lock(m_clientsMutex);
	m_clients.clear();
}

void Server::HandleClientMessages() noexcept {
	fd_set readfds;
	FD_ZERO(&readfds);

	int max_fd = 0;

	{
		std::lock_guard<std::mutex> lock(m_clientsMutex);
		for (const auto& client : m_clients) {
			Handler::Type client_fd = *client->Handle();
			FD_SET(client_fd, &readfds);
			if (client_fd > max_fd) {
				max_fd = client_fd;
			}
		}
	}

	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	int activity = select(max_fd + 1, &readfds, nullptr, nullptr, &timeout);

	if (activity < 0) {
		// m_handler->LastError();
		return;
	}

	{
		std::lock_guard<std::mutex> lock(m_clientsMutex);
		for (auto& client : m_clients) {
			if (!this->HandleRead(client)) {
				RemoveClient(client);
			}
		}
	}
}