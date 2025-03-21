#include <StormByte/network/server.hxx>

#ifdef LINUX
#include <sys/select.h>
#include <unistd.h>
#include <cstring> // For strerror
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <iostream>
#include <format>

using namespace StormByte::Network;

Server::Server(std::shared_ptr<const Connection::Handler> handler): m_socket(nullptr), m_status(Connection::Status::Disconnected), m_handler(handler) {}

Server::~Server() noexcept {
	Disconnect();
}

StormByte::Expected<void, ConnectionError> Server::Connect(const std::string& hostname, const unsigned short& port, const Connection::Protocol& protocol) noexcept {
	if (Connection::IsConnected(m_status.load()))
		return StormByte::Unexpected<ConnectionError>("Server is already connected");

	m_socket = std::make_unique<Socket::Server>(protocol, m_handler);
	auto expected_listen = m_socket->Listen(hostname, port);
	if (!expected_listen)
		return StormByte::Unexpected(expected_listen.error());
	
	// Start a thread to accept client connections
	std::cout << "Starting accept thread" << std::endl;
	m_status.store(Connection::Status::Connected);
    m_acceptThread = std::thread(&Server::AcceptClients, this);
	std::cout << "Accept thread end" << std::endl;

	return {};
}

void Server::Disconnect() noexcept {
	if (m_status.load() == Connection::Status::Disconnected)
		return;

	m_status.store(Connection::Status::Disconnecting);

	if (m_acceptThread.joinable()) {
		m_acceptThread.join();
	}
	else std::cerr << "Accept thread is not joinable" << std::endl;
	for (auto& clientThread: m_clientThreads) {
		if (clientThread.joinable()) {
			clientThread.join();
		}
		else std::cerr << "Client thread is not joinable" << std::endl;
	}
	RemoveAllClients();
	m_socket.reset();

	m_status.store(Connection::Status::Disconnected);
}

void Server::AcceptClients() {
	std::cout << "Starting to accept clients" << std::endl;
	while (m_status.load() == Connection::Status::Connected) {
		auto expected_client_socket = m_socket->Accept();
		if (expected_client_socket) {
			expected_client_socket.value().Status() = Connection::Status::Negotiating;
			Socket::Client& client = AddClient(std::move(expected_client_socket.value()));
			m_clientThreads.push_back(std::thread(&Server::StartClientCommunication, this, std::ref(client)));
		}
	}
	std::cout << "Ending to accept clients" << std::endl;
}

Socket::Client& Server::AddClient(Socket::Client&& client) {
	std::lock_guard<std::mutex> lock(m_clientsMutex);
	m_clients.push_back(std::make_unique<Socket::Client>(std::move(client)));
	return *m_clients.back();
}

void Server::RemoveClient(Socket::Client& client) noexcept {
	std::lock_guard<std::mutex> lock(m_clientsMutex);
	m_clients.erase(std::remove_if(m_clients.begin(), m_clients.end(), [&client](const std::unique_ptr<Socket::Client>& c) {
		return c.get() == &client;
	}), m_clients.end());
}

void Server::RemoveAllClients() noexcept {
	std::lock_guard<std::mutex> lock(m_clientsMutex);
	m_clients.clear();
}

void Server::HandleClientMessages(Socket::Client& client) noexcept {
	fd_set readfds;
	FD_ZERO(&readfds);

	Connection::Handler::Type client_fd = *client.Handle();

	while (Connection::IsConnected(client.Status())) { // Keep processing as long as the client is connected
		FD_ZERO(&readfds);         // Reset fd_set for each iteration
		FD_SET(client_fd, &readfds);

		struct timeval timeout;
		timeout.tv_sec = 5;        // 5 seconds timeout
		timeout.tv_usec = 0;

		int activity = select(client_fd + 1, &readfds, nullptr, nullptr, &timeout);

		if (activity < 0) {
			// Handle select error (log or take action as needed)
			break;
		}
		else if (activity == 0) {
			// Timeout occurred, no activity on the socket
			continue; // Continue to the next iteration
		}
		else if (FD_ISSET(client_fd, &readfds)) {
			// Activity detected on the client's socket
			auto expected_message = HandleClientMessage(client);
			if (!expected_message) {
				client.Status() = Connection::Status::PeerClosed;
			}
			else {
				Util::Buffer message = std::move(expected_message.value());
				if (!this->PreProcessMessage(client, message)) {
					client.Status() = Connection::Status::Error;
				}
				else {
					Util::Buffer response = this->ProcessMessage(client, message);
					// Send the response to the client
				}
			}
		}
	}
}

StormByte::Expected<StormByte::Util::Buffer, ConnectionClosed> Server::HandleClientMessage(Socket::Client&) noexcept {
	// Read the message from the client
	return {};
}

bool Server::NegotiateConnection(Socket::Client& client) noexcept {
	client.Status() = Connection::Status::Connected;
	return true;
}

bool Server::PreProcessMessage(Socket::Client&, Util::Buffer&) noexcept {
	return true;
}

StormByte::Util::Buffer Server::ProcessMessage(Socket::Client&, const Util::Buffer&) noexcept {
	return {};
}

void Server::StartClientCommunication(Socket::Client& client) noexcept {
	std::thread client_thread([&client, this]() {
		while (Connection::IsConnected(m_status.load()) && Connection::IsConnected(client.Status())) {
			// Handle client messages
			HandleClientMessages(client);
		}
	});
	if (!this->NegotiateConnection(client)) {
		client.Status() = Connection::Status::Rejected;
		if (client_thread.joinable()) {
			client_thread.join();
		}
		RemoveClient(client);
		return;
	}
	else if (client_thread.joinable()) {
		client_thread.join();
	}
	std::cout << "Estoy aqui?" << std::endl;
	// This communication ended, remove the client
	RemoveClient(client);
}
