#include <StormByte/network/server.hxx>
#include <StormByte/network/socket/client.hxx>
#include <StormByte/network/socket/server.hxx>

using namespace StormByte::Network;

Server::Server(const enum Protocol& protocol, const Codec& codec, const unsigned short& timeout, const Logger::ThreadedLog& logger) noexcept:
EndPoint(protocol, codec, timeout, logger) {}

Server::~Server() noexcept {
	Disconnect();
}

ExpectedVoid Server::Connect(const std::string& host, const unsigned short& port) noexcept {
	if (Connection::IsConnected(m_status))
		return StormByte::Unexpected<ConnectionError>("Server is already connected");

	try {
		auto self_socket = std::make_shared<Socket::Server>(m_protocol, m_logger);
		m_self_uuid = self_socket->UUID();
		m_client_pmap.emplace(m_self_uuid,  std::move(self_socket));
	}
	catch (const std::bad_alloc& e) {
		return StormByte::Unexpected<ConnectionError>("Can not create connection: {}", e.what());
	}

	m_status.store(Connection::Status::Connecting);
	auto listen_sock = std::static_pointer_cast<Socket::Server>(GetSocketByUUID(m_self_uuid));
	if (!listen_sock) {
		m_status = Connection::Status::Disconnected;
		return StormByte::Unexpected<ConnectionError>("Internal error: listening socket missing");
	}
	auto expected_listen = listen_sock->Listen(host, port);
	if (!expected_listen) {
		m_status = Connection::Status::Disconnected;
		return StormByte::Unexpected(expected_listen.error());
	}
	m_accept_thread = std::thread(&Server::AcceptClients, this);
	m_logger << Logger::Level::LowLevel << "Server started and listening on " << host << ":" << port << std::endl;
	m_status.store(Connection::Status::Connected);
	return {};
}

void Server::Disconnect() noexcept {
	if (!Connection::IsConnected(m_status.load()))
		return;
		
	m_status.store(Connection::Status::Disconnecting);

	// Disconnect our listening socket so pending operations fail. Snapshot under lock
	std::shared_ptr<Socket::Socket> listen_sock;
	{
		std::lock_guard<std::mutex> lock(m_clients_mutex);
		auto it = m_client_pmap.find(m_self_uuid);
		if (it != m_client_pmap.end()) listen_sock = it->second;
	}
	if (listen_sock) listen_sock->Disconnect();

	// Stop accepting new clients first
	if (m_accept_thread.joinable()) m_accept_thread.join();

	// Snapshot current clients under lock and disconnect them outside the lock
	std::vector<std::shared_ptr<Socket::Socket>> clients_snapshot;
	{
		std::lock_guard<std::mutex> lock(m_clients_mutex);
		for (auto& kv : m_client_pmap) {
			if (kv.first != m_self_uuid) clients_snapshot.push_back(kv.second);
		}
	}
	for (auto& s : clients_snapshot) if (s && s->UUID() != m_self_uuid) s->Disconnect();

	// Ensure all client threads are joined and cleared
	{
		std::lock_guard<std::mutex> lock(m_msg_threads_mutex);
		for (auto& kv : m_handle_msg_threads) {
			if (kv.second.joinable()) kv.second.join();
		}
		m_handle_msg_threads.clear();
	}

	// Remove all clients (under lock)
	{
		std::lock_guard<std::mutex> lock(m_clients_mutex);
		m_client_pmap.clear();
		m_in_pipeline_pmap.clear();
		m_out_pipeline_pmap.clear();
	}

	m_logger << Logger::Level::LowLevel << "Server stopped and disconnected." << std::endl;
	m_status.store(Connection::Status::Disconnected);
}

void Server::AcceptClients() noexcept {
	constexpr const auto TIMEOUT = 1000000; // 1 second
	m_logger << Logger::Level::LowLevel << "Starting accept clients thread" << std::endl;
	std::shared_ptr<Socket::Server> server_socket = std::static_pointer_cast<Socket::Server>(GetSocketByUUID(m_self_uuid));
	if (!server_socket) {
		m_logger << Logger::Level::Error << "Accept thread: listening socket missing" << std::endl;
		return;
	}
	while (Connection::IsConnected(m_status)) {
		auto expected_wait = server_socket->WaitForData(TIMEOUT);
		if (!expected_wait) {
			m_logger << Logger::Level::Error << expected_wait.error()->what() << std::endl;
			return;
		}

		switch(expected_wait.value()) {
			case Connection::Read::Result::Success: {
				auto expected_client = server_socket->Accept();
				if (!expected_client) {
					m_logger << Logger::Level::Error << expected_client.error()->what() << std::endl;
					return;
				}

				{
					const std::string client_uuid = expected_client->UUID();

					// Create client and pipelines outside the clients mutex to avoid long holds
					auto client_shared = std::make_shared<Socket::Client>(std::move(expected_client.value()));
					auto in_pipeline = std::make_shared<Buffer::Pipeline>(CreateClientInputPipeline(client_uuid));
					auto out_pipeline = std::make_shared<Buffer::Pipeline>(CreateClientOutputPipeline(client_uuid));

					std::lock_guard<std::mutex> lock(m_clients_mutex);
					m_client_pmap.emplace(client_uuid, std::static_pointer_cast<Socket::Socket>(client_shared));
					m_handle_msg_threads.insert({client_uuid, std::thread(&Server::HandleClientCommunication, this, client_uuid)});
					m_in_pipeline_pmap.emplace(client_uuid, in_pipeline);
					m_out_pipeline_pmap.emplace(client_uuid, out_pipeline);
				}
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

void Server::HandleClientCommunication(const std::string& client_uuid) noexcept {
	m_logger << Logger::Level::LowLevel << "Starting handle client " << client_uuid << " messages thread" << std::endl;
	std::shared_ptr<Socket::Client> client = std::static_pointer_cast<Socket::Client>(GetSocketByUUID(client_uuid));
	if (!client) {
		m_logger << Logger::Level::Error << "HandleClientCommunication: client not found " << client_uuid << std::endl;
		return;
	}
	while (Connection::IsConnected(m_status) && Connection::IsConnected(client->Status())) {
		auto expected_wait = client->WaitForData();
		if (!expected_wait) {
			m_logger << Logger::Level::Error << expected_wait.error()->what() << std::endl;
			client->Disconnect();
			break;
		}

		switch(expected_wait.value()) {
			case Connection::Read::Result::Success: {
				if (client->HasShutdownRequest()) {
					m_logger << Logger::Level::LowLevel << "Client has requested shutdown, disconnecting..." << std::endl;
					client->Disconnect();
					break;
				}
				else {
					auto expected_packet = Receive(client_uuid); // Uses codec to transform data to a packet
					if (!expected_packet) {
						m_logger << Logger::Level::Error << expected_packet.error()->what() << std::endl;
						client->Disconnect();
						break;
					}

					auto expected_processed_packet = ProcessClientPacket(client_uuid, *expected_packet.value());
					if (!expected_processed_packet) {
						m_logger << Logger::Level::Error << expected_processed_packet.error()->what() << std::endl;
						client->Disconnect();
						break;
					}
				}
			}

			case Connection::Read::Result::Timeout:
			default:
				continue;
		}
	}
	{
		std::lock_guard<std::mutex> lock(m_clients_mutex);
		m_client_pmap.erase(client_uuid);
		m_in_pipeline_pmap.erase(client_uuid);
		m_out_pipeline_pmap.erase(client_uuid);
	}
	m_logger << Logger::Level::LowLevel << "Stopping handle client messages thread" << std::endl;
}

StormByte::Buffer::Pipeline Server::CreateClientInputPipeline(const std::string& client_uuid) noexcept {
	return Buffer::Pipeline();
}

StormByte::Buffer::Pipeline Server::CreateClientOutputPipeline(const std::string& client_uuid) noexcept {
	return Buffer::Pipeline();
}

std::shared_ptr<Socket::Socket> Server::GetSocketByUUID(const std::string& uuid) noexcept {
	std::lock_guard<std::mutex> lock(m_clients_mutex);
	auto it = m_client_pmap.find(uuid);
	if (it == m_client_pmap.end()) return nullptr;
	return it->second;
}

std::shared_ptr<StormByte::Buffer::Pipeline> Server::GetInPipelineByUUID(const std::string& uuid) noexcept {
	std::lock_guard<std::mutex> lock(m_clients_mutex);
	auto it = m_in_pipeline_pmap.find(uuid);
	if (it == m_in_pipeline_pmap.end()) return nullptr;
	return it->second;
}

std::shared_ptr<StormByte::Buffer::Pipeline> Server::GetOutPipelineByUUID(const std::string& uuid) noexcept {
	std::lock_guard<std::mutex> lock(m_clients_mutex);
	auto it = m_out_pipeline_pmap.find(uuid);
	if (it == m_out_pipeline_pmap.end()) return nullptr;
	return it->second;
}