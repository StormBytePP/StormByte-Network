#include <StormByte/network/server.hxx>
#include <StormByte/network/socket/client.hxx>
#include <StormByte/network/socket/server.hxx>

using namespace StormByte::Network;

Server::Server(Connection::Protocol protocol, std::shared_ptr<Connection::Handler> handler, Logger::ThreadedLog logger) noexcept:
EndPoint(protocol, handler, logger) {}

Server::~Server() noexcept {
	Disconnect();
}

ExpectedVoid Server::Connect(const std::string& host, const unsigned short& port) noexcept {
	if (Connection::IsConnected(m_status))
		return StormByte::Unexpected<ConnectionError>("Server is already connected");

	try {
		m_socket = new Socket::Server(m_protocol, m_handler, m_logger);
	}
	catch (const std::bad_alloc& e) {
		return StormByte::Unexpected<ConnectionError>("Can not create connection: {}", e.what());
	}

	m_status.store(Connection::Status::Connecting);
	auto expected_listen = static_cast<Socket::Server*>(m_socket)->Listen(host, port);
	if (!expected_listen) {
		m_status = Connection::Status::Disconnected;
		return StormByte::Unexpected(expected_listen.error());
	}
	m_status.store(Connection::Status::Connected);
	m_accept_thread = std::thread(&Server::AcceptClients, this);
	return {};
}

void Server::Disconnect() noexcept {
	if (!Connection::IsConnected(m_status.load()))
		return;
		
	m_status.store(Connection::Status::Disconnecting);

	// Disconnect our socket so all operations pending on it will fail
	m_socket->Disconnect();

	// Stop accepting new clients first
	if (m_accept_thread.joinable())
		m_accept_thread.join();

	// Disconnect all clients
	DisconnectAllClients();

	// Ensure all client threads are joined and cleared
	{
		std::lock_guard<std::mutex> lock(m_msg_threads_mutex);
		for (auto& kv : m_msg_threads) {
			if (kv.second.joinable()) kv.second.join();
		}
		m_msg_threads.clear();
	}

	// Clear the client vector
	m_clients.clear();

	EndPoint::Disconnect();
}

Socket::Client& Server::AddClient(Socket::Client&& client) noexcept {
	std::lock_guard<std::mutex> lock(m_clients_mutex);
	m_clients.push_back(std::move(client));
	return m_clients.back();
}

void Server::RemoveClient(Socket::Client& client) noexcept {
	// If already removed, skip
	{
		std::lock_guard<std::mutex> lock(m_clients_mutex);
		auto itc = std::find_if(m_clients.begin(), m_clients.end(), [&](const Socket::Client& c){ return &c == &client; });
		if (itc == m_clients.end()) return;
	}
	// Unified cleanup: capture handle before disconnect to avoid null-deref.
	std::unique_lock<std::mutex> threads_lock(m_msg_threads_mutex);
	bool has_thread = false;
	bool is_self = false;
	Connection::Handler::Type client_handler{};
	if (Connection::IsConnected(client.Status())) {
		client_handler = client.Handle();
		auto it = m_msg_threads.find(client_handler);
		has_thread = (it != m_msg_threads.end());
		is_self = (has_thread && it->second.get_id() == std::this_thread::get_id());
	}

	// Disconnect client
	client.Disconnect();

	// Join if applicable (not self)
	if (has_thread && !is_self) {
		auto& client_thread = m_msg_threads.find(client_handler)->second;
		threads_lock.unlock();
		client_thread.join();
		threads_lock.lock();
	}

	// Remove thread record
	if (has_thread) {
		m_msg_threads.erase(client_handler);
	}

	std::lock_guard<std::mutex> lock(m_clients_mutex);
	m_clients.erase(std::remove_if(m_clients.begin(), m_clients.end(), [&](const Socket::Client& c) {
		return &c == &client;
	}), m_clients.end());
}

void Server::RemoveClientAsync(Socket::Client& client) noexcept {
	// Run synchronously to ensure client lifetime is valid.
	RemoveClient(client);
}

void Server::DisconnectClient(Socket::Client& client) noexcept {
	RemoveClient(client);
}

void Server::DisconnectAllClients() noexcept {
	std::lock_guard<std::mutex> lock(m_clients_mutex);
	for (auto& client : m_clients)
		RemoveClient(client);
}

void Server::AcceptClients() noexcept {
	constexpr const auto TIMEOUT = 2000000; // 2 seconds
	m_logger << Logger::Level::LowLevel << "Starting accept clients thread" << std::endl;
	while (Connection::IsConnected(m_status)) {
		auto expected_wait = static_cast<Socket::Server*>(m_socket)->WaitForData(TIMEOUT);
		if (!expected_wait) {
			m_logger << Logger::Level::Error << expected_wait.error()->what() << std::endl;
			return;
		}

		switch(expected_wait.value()) {
			case Connection::Read::Result::Success: {
				auto expected_client = static_cast<Socket::Server*>(m_socket)->Accept();
				if (!expected_client) {
					m_logger << Logger::Level::Error << expected_client.error()->what() << std::endl;
					return;
				}

				auto& client = AddClient(std::move(expected_client.value()));
				m_msg_threads[client.Handle()] = std::thread(&Server::StartClientCommunication, this, std::ref(client));
				break;
			}

			case Connection::Read::Result::Timeout:
				// No incoming connection within timeout; continue waiting
				m_logger << Logger::Level::LowLevel << "Accept wait timed out, continuing" << std::endl;
				continue;
			case Connection::Read::Result::Closed:
				// Listening socket was closed or became invalid — stop accepting
				m_logger << Logger::Level::LowLevel << "Listening socket closed; stopping accept loop" << std::endl;
				return;
			default:
				continue;
		}
	}
	m_logger << Logger::Level::LowLevel << "Stopping accept clients thread" << std::endl;
}

void Server::StartClientCommunication(Socket::Client& client) noexcept {
	auto handshake_expected = Handshake(client);
	if (!handshake_expected) {
		m_logger << Logger::Level::LowLevel << "Failed handshake with client: " << handshake_expected.error()->what() << std::endl;
		RemoveClientAsync(client);
		return;
	}

	HandleClientMessages(client);
}

void Server::HandleClientMessages(Socket::Client& client) noexcept {
	m_logger << Logger::Level::LowLevel << "Starting handle client messages thread" << std::endl;
	while (Connection::IsConnected(m_status) && Connection::IsConnected(client.Status())) {
		auto expected_wait = client.WaitForData();
		if (!expected_wait) {
			m_logger << Logger::Level::Error << expected_wait.error()->what() << std::endl;
			client.Disconnect();
			break;
		}

		switch(expected_wait.value()) {
			case Connection::Read::Result::Success: {
				if (client.HasShutdownRequest()) {
					// Can't remove client here because it will cause a deadlock
					// as RemoveClient waits for this thread to finish
					// Instead, we start a thread which will remove it with a delay
					// Allowing time for this thread to finish
					m_logger << Logger::Level::LowLevel << "Client has requested shutdown, disconnecting..." << std::endl;
					client.Disconnect();
					break;
				}
				else {
					auto expected_packet = Receive(client);
					if (!expected_packet) {
						m_logger << Logger::Level::Error << expected_packet.error()->what() << std::endl;
						client.Disconnect();
						break;
					}

					auto expected_processed_packet = ProcessClientPacket(client, *expected_packet.value());
					if (!expected_processed_packet) {
						m_logger << Logger::Level::Error << expected_processed_packet.error()->what() << std::endl;
						client.Disconnect();
						break;
					}
				}
			}

			case Connection::Read::Result::Timeout:
			default:
				continue;
		}
	}
	// Final cleanup after loop exits
	RemoveClient(client);
	m_logger << Logger::Level::LowLevel << "Stopping handle client messages thread" << std::endl;
}
