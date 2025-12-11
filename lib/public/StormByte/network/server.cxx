#include <StormByte/network/connection/client.hxx>
#include <StormByte/network/server.hxx>
#include <StormByte/network/socket/server.hxx>

using namespace StormByte::Network;

Server::Server(const DeserializePacketFunction& deserialize_packet_function, const Logger::ThreadedLog& logger) noexcept:
	Endpoint(deserialize_packet_function, logger),
	m_socket_server(nullptr),
	m_status(Connection::Status::Disconnected),
	m_accept_thread()
{}

Server::~Server() noexcept {
	Disconnect(); // Not really needed but explicit to make sure logger is not destructed before disconnection
}

bool Server::Connect(const Connection::Protocol& protocol, const std::string& address, const unsigned short& port) {
	if (m_socket_server) {
		m_logger << Logger::Level::Error << "Server is already running." << std::endl;
		return false;
	}

	try {
		m_socket_server = std::make_unique<Socket::Server>(protocol, m_logger);

		if (!m_socket_server->Listen(address, port)) {
			m_logger << Logger::Level::Error << "Failed to listen on " << address << ":" << port << " using protocol " << Connection::ProtocolString(protocol) << std::endl;
			m_socket_server.reset();
			return false;
		}

		m_status.store(Connection::Status::Connected);
		m_accept_thread = std::thread(&Server::AcceptClients, this);
		m_logger << Logger::Level::LowLevel << "Server is listening on " << address << ":" << port << " using protocol " << Connection::ProtocolString(protocol) << std::endl;
		return true;
	} catch (const std::bad_alloc& bd) {
		m_logger << Logger::Level::Error << "Failed to allocate memory for server socket: " << bd.what() << std::endl;
		return false;
	}
}

void Server::Disconnect() noexcept {
	if (m_socket_server) {
		m_logger << Logger::Level::LowLevel << "Stopping server and disconnecting all clients." << std::endl;

		// Update status
		m_status.store(Connection::Status::Disconnecting);

		// Disconnect early so no new clients are accepted
		m_socket_server->Disconnect();

		// Wait until accept thread finishes
		if (m_accept_thread.joinable()) {
			m_accept_thread.join();
		}

		// We snapshot the client UUIDs to avoid holding the mutex while disconnecting
		std::vector<std::string> client_uuids;
		{
			std::scoped_lock lock_guard(m_mutex);
			for (const auto& [uuid, _] : m_clients) {
				client_uuids.push_back(uuid);
			}
		}

		// Disconnect all clients
		for (const auto& uuid : client_uuids)
			DisconnectClient(uuid);

		// Close server socket
		m_socket_server.reset();

		// Update status
		m_status.store(Connection::Status::Disconnected);
	}
}

void Server::DisconnectClient(const std::string& uuid) noexcept {
	std::scoped_lock lock_guard(m_mutex);
	auto it = m_clients.find(uuid);
	if (it != m_clients.end()) {
		it->second->Socket()->Disconnect();
		m_logger << Logger::Level::LowLevel << "Disconnected client: " << uuid << std::endl;
		m_clients.erase(it);
	}

	auto thread_it = m_handle_msg_threads.find(uuid);
	if (thread_it != m_handle_msg_threads.end()) {
		if (thread_it->second.joinable()) {
			// Avoid attempting to join the current thread (would deadlock).
			if (thread_it->second.get_id() == std::this_thread::get_id()) {
				thread_it->second.detach();
			} else {
				thread_it->second.join();
			}
		}
		m_handle_msg_threads.erase(thread_it);
	}
}

void Server::AcceptClients() noexcept {
	constexpr const auto TIMEOUT = 1000000; // 1 second
	m_logger << Logger::Level::LowLevel << "Started accept clients thread" << std::endl;
	
	while (Connection::IsConnected(m_status.load())) {
		auto expected_wait = m_socket_server->WaitForData(TIMEOUT);
		if (!expected_wait) {
			m_logger << Logger::Level::Error << expected_wait.error()->what() << std::endl;
			return;
		}

		switch(expected_wait.value()) {
			case Connection::Read::Result::Success: {
				auto expected_client = m_socket_server->Accept();
				if (!expected_client) {
					m_logger << Logger::Level::Error << expected_client.error()->what() << std::endl;
					return;
				}

				const std::string client_uuid = expected_client.value()->UUID();
				{
					std::scoped_lock lock_guard(m_mutex);
					m_clients.emplace(client_uuid, CreateConnection(expected_client.value()));
					m_handle_msg_threads.emplace(client_uuid, std::thread(&Server::HandleClientCommunication, this, client_uuid));
				}
				m_logger << Logger::Level::LowLevel << "AcceptClients: accepted client uuid=" << client_uuid << std::endl;
				break;
			}

			case Connection::Read::Result::Timeout:
				// No incoming connection within timeout; continue waiting
				std::this_thread::yield();
				continue;
			case Connection::Read::Result::Closed:
				// Listening socket was closed or became invalid — stop accepting
				m_logger << Logger::Level::LowLevel << "Listening socket closed; stopping accept loop" << std::endl;
				return;
			default:
				continue;
		}
	}

	m_logger << Logger::Level::LowLevel << "Stopped accept clients thread" << std::endl;
}

void Server::HandleClientCommunication(const std::string& client_uuid) noexcept {
	m_logger << Logger::Level::LowLevel << "Started communication thread for client uuid=" << client_uuid << std::endl;
	std::shared_ptr<Connection::Client> client;
	{
		std::scoped_lock lock_guard(m_mutex);
		auto it = m_clients.find(client_uuid);
		if (it == m_clients.end()) {
			m_logger << Logger::Level::LowLevel << "Client uuid=" << client_uuid << " not found; ending communication thread" << std::endl;
			return;
		}
		client = it->second;
	}

	while (Connection::IsConnected(m_status.load()) && Connection::IsConnected(client->Status())) {
		auto expected_wait = client->Socket()->WaitForData();
		if (!expected_wait) {
			m_logger << Logger::Level::Error << expected_wait.error()->what() << std::endl;
			goto end;
		}

		switch(expected_wait.value()) {
			case Connection::Read::Result::Success: {
				m_logger << Logger::Level::LowLevel << "HandleClientCommunication: data ready for client=" << client_uuid << std::endl;
				PacketPointer packet;
				{
					Transport::Frame frame = client->Receive(m_logger);
					packet = frame.ProcessPacket(m_deserialize_packet_function, m_logger);
				}
				if (!packet) {
					m_logger << Logger::Level::Error << "Failed to process packet from client=" << client_uuid << std::endl;
					goto end;
				}
				m_logger << Logger::Level::LowLevel << "HandleClientCommunication: received packet opcode=" << packet->Opcode() << " from client=" << client_uuid << std::endl;

				// Guard against server teardown races: if disconnecting, skip calling virtual handler
				if (!Connection::IsConnected(m_status.load())) {
					m_logger << Logger::Level::LowLevel << "HandleClientCommunication: server is disconnecting; skipping packet processing for client=" << client_uuid << std::endl;
					goto end;
				}

				PacketPointer response_packet = ProcessClientPacket(client_uuid, packet);
				if (!response_packet) {
					m_logger << Logger::Level::Error << "HandleClientCommunication: resonse packet was null" << std::endl;
					goto end;
					break;
				}
				m_logger << Logger::Level::LowLevel << "HandleClientCommunication: processed packet successfully for client=" << client_uuid << std::endl;

				// After processing, honour shutdown if requested
				if (client->Socket()->HasShutdownRequest() || !Connection::IsConnected(m_status.load())) {
					m_logger << Logger::Level::LowLevel << "Client has requested shutdown, disconnecting..." << std::endl;
					goto end;
				}
				else {
					// Reply response packet
					Reply(client, *response_packet);
					m_logger << Logger::Level::LowLevel << "HandleClientCommunication: sent response packet opcode=" << response_packet->Opcode() << " to client=" << client_uuid << std::endl;
				}
				break;
			}

			case Connection::Read::Result::Closed: {
				m_logger << Logger::Level::LowLevel << "HandleClientCommunication: client=" << client_uuid << " has closed the connection" << std::endl;
				goto end;
			}

			case Connection::Read::Result::ShutdownRequest: {
				m_logger << Logger::Level::LowLevel << "HandleClientCommunication: client=" << client_uuid << " has requested shutdown" << std::endl;
				goto end;
			}

			case Connection::Read::Result::Timeout:
				m_logger << Logger::Level::LowLevel << "HandleClientCommunication: timeout waiting for data from client=" << client_uuid << std::endl;
				std::this_thread::yield();
				continue;
			default:
				m_logger << Logger::Level::LowLevel << "HandleClientCommunication: unexpected wait result for client=" << client_uuid << std::endl;
				continue;
		}
	}

	end:
	DisconnectClient(client_uuid);
	m_logger << Logger::Level::LowLevel << "Stopped communication thread for client uuid=" << client_uuid << std::endl;
}