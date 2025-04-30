#include <StormByte/network/server.hxx>
#include <StormByte/network/socket/client.hxx>
#include <StormByte/network/socket/server.hxx>

using namespace StormByte::Network;

Server::Server(Connection::Protocol protocol, std::shared_ptr<Connection::Handler> handler, std::shared_ptr<Logger> logger) noexcept:
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
	DisconnectClient(client);
	std::lock_guard<std::mutex> lock(m_clients_mutex);
	m_clients.erase(std::remove_if(m_clients.begin(), m_clients.end(), [&](const Socket::Client& c) {
		return &c == &client;
	}), m_clients.end());
}

void Server::RemoveClientAsync(Socket::Client& client) noexcept {
	std::thread([this](Socket::Client& client) {
		RemoveClient(client);
	}, std::ref(client)).detach();
}

void Server::DisconnectClient(Socket::Client& client) noexcept {
	std::lock_guard<std::mutex> lock(m_msg_threads_mutex);

	// It is important to copy the client handle here as when disconnect it will be freed so it will be safe to use for the map
	const auto client_handler = client.Handle();
	auto& client_thread = m_msg_threads.at(client_handler);

	// Disconnect client
	client.Disconnect();

	// Stop client message thread
	client_thread.join();

	// Remove thread from map
	m_msg_threads.erase(client_handler);
}

void Server::DisconnectAllClients() noexcept {
	std::lock_guard<std::mutex> lock(m_clients_mutex);
	for (auto& client : m_clients)
		DisconnectClient(client);
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
			case Connection::Read::Result::Closed:
				m_logger << Logger::Level::LowLevel << "Socket closed or timeout occurred" << std::endl;
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
			RemoveClientAsync(client);
			goto end;
		}

		switch(expected_wait.value()) {
			case Connection::Read::Result::Success: {
				if (client.HasShutdownRequest()) {
					// Can't remove client here because it will cause a deadlock
					// as RemoveClient waits for this thread to finish
					// Instead, we start a thread which will remove it with a delay
					// Allowing time for this thread to finish
					m_logger << Logger::Level::LowLevel << "Client has requested shutdown, disconnecting..." << std::endl;
					RemoveClientAsync(client);
					goto end;
				}
				else {
					auto expected_msg = client.Receive();
					if (!expected_msg) {
						m_logger << Logger::Level::Error << expected_msg.error()->what() << std::endl;
						RemoveClientAsync(client);
						goto end;
					}

					auto expected_preprocessed_msg = ProcessInputMessagePipeline(client, expected_msg.value());
					if (!expected_preprocessed_msg) {
						m_logger << Logger::Level::Error << expected_preprocessed_msg.error()->what() << std::endl;
						RemoveClientAsync(client);
						goto end;
					}
					auto process_expected = ProcessClientMessage(client, expected_preprocessed_msg.value());
					if (!process_expected) {
						m_logger << Logger::Level::Error << process_expected.error()->what() << std::endl;
						RemoveClientAsync(client);
						goto end;
					}
					auto send_expected = SendClientReply(client, process_expected.value());
					if (!send_expected) {
						m_logger << Logger::Level::Error << send_expected.error()->what() << std::endl;
						RemoveClientAsync(client);
						goto end;
					}
				}
			}

			case Connection::Read::Result::Timeout:
			default:
				continue;
		}
	}
	
	end:
	m_logger << Logger::Level::LowLevel << "Stopping handle client messages thread" << std::endl;
}

ExpectedVoid Server::SendClientReply(Socket::Client& client, FutureBuffer& message) noexcept {
	auto preprocessed_reply = ProcessOutputMessagePipeline(client, message);
	if (!preprocessed_reply) {
		m_logger << Logger::Level::Error << preprocessed_reply.error()->what() << std::endl;
		return StormByte::Unexpected<ConnectionError>(preprocessed_reply.error()->what());
	}
	auto expected_send = client.Send(preprocessed_reply.value().get().Data());
	if (!expected_send) {
		m_logger << Logger::Level::Error << expected_send.error()->what() << std::endl;
		return StormByte::Unexpected<ConnectionError>(expected_send.error()->what());
	}
	return {};
}

ExpectedFutureBuffer Server::ProcessInputMessagePipeline(Socket::Client& client, FutureBuffer& message) noexcept {
	FutureBuffer processed_message = std::move(message);

	for (auto& processor: m_input_pipeline) {
		auto expected_processed = processor(client, processed_message);
		if (!expected_processed) {
			m_logger << Logger::Level::Error << expected_processed.error()->what() << std::endl;
			return StormByte::Unexpected<ConnectionError>(expected_processed.error()->what());
		}
		processed_message = std::move(expected_processed.value());
	}

	return processed_message;
}

ExpectedFutureBuffer Server::ProcessOutputMessagePipeline(Socket::Client& client, FutureBuffer& message) noexcept {
	FutureBuffer processed_message = std::move(message);

	for (auto& processor: m_output_pipeline) {
		auto expected_processed = processor(client, processed_message);
		if (!expected_processed) {
			m_logger << Logger::Level::Error << expected_processed.error()->what() << std::endl;
			return StormByte::Unexpected<ConnectionError>(expected_processed.error()->what());
		}
		processed_message = std::move(expected_processed.value());
	}

	return processed_message;
}