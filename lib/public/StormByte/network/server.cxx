#include <StormByte/network/connection/client.hxx>
#include <StormByte/network/server.hxx>
#include <StormByte/network/socket/server.hxx>

using namespace StormByte::Network;

Server::Server(const DeserializePacketFunction& deserialize_packet_function, std::shared_ptr<Logger::Log> logger) noexcept:
	Endpoint(deserialize_packet_function, logger),
	m_socket_server(nullptr),
	m_status(Connection::Status::Disconnected),
	m_accept_thread()
{}

Server::~Server() noexcept {
	Disconnect();
}

bool Server::Connect(const Connection::Protocol& protocol, const std::string& address, const unsigned short& port) {
	if (m_socket_server) {
		m_logger << Logger::Level::Error << "Server is already running." << std::endl;
		return false;
	}

	try {
		m_socket_server = std::make_unique<Socket::Server>(protocol, m_logger);

		if (!m_socket_server->Listen(address, port)) {
			m_logger << Logger::Level::Error << "Failed to listen on " << address << ":" << port
					<< " using protocol " << Connection::ProtocolString(protocol) << std::endl;
			m_socket_server.reset();
			return false;
		}

		m_status.store(Connection::Status::Connected);
		m_accept_thread = std::thread(&Server::AcceptClients, this);
		m_logger << Logger::Level::LowLevel << "Server is listening on " << address << ":" << port
				<< " using protocol " << Connection::ProtocolString(protocol) << std::endl;
		return true;
	} catch (const std::bad_alloc& bd) {
		m_logger << Logger::Level::Error << "Failed to allocate memory for server socket: " << bd.what() << std::endl;
		return false;
	}
}

void Server::Disconnect() noexcept {
	if (!m_socket_server) {
		return;
	}

	m_logger << Logger::Level::LowLevel
			<< "Stopping server and disconnecting all clients." << std::endl;

	// 1) Signal stop so AcceptClients' while (IsConnected(...)) exits
	m_status.store(Connection::Status::Disconnecting, std::memory_order_release);

	// 2) Wake AcceptClients if blocked in WaitForData / Accept (without full teardown yet)
	{
		const auto& h = m_socket_server->Handle();
#ifdef LINUX
		if (h > 0)
			::shutdown(h, SHUT_RDWR);
#else
		if (h != INVALID_SOCKET)
			::shutdown(h, SD_BOTH);
#endif
	}

	// 3) Wait until accept thread has left WaitForData / the loop
	if (m_accept_thread.joinable()) {
		m_accept_thread.join();
	}

	// 4) Snapshot client UUIDs (no join under mutex)
	std::vector<std::string> client_uuids;
	{
		std::scoped_lock lock_guard(m_mutex);
		client_uuids.reserve(m_clients.size());
		for (const auto& [uuid, _] : m_clients) {
			client_uuids.push_back(uuid);
		}
	}

	for (const auto& uuid : client_uuids) {
		DisconnectClient(uuid);
	}

	// 5) Now safe: no accept thread using the listen fd
	m_socket_server->Disconnect();
	m_socket_server.reset();
	m_status.store(Connection::Status::Disconnected, std::memory_order_release);
}

void Server::DisconnectClient(const std::string& uuid) noexcept {
	std::thread thread_to_join;
	std::shared_ptr<Connection::Client> client;

	{
		std::scoped_lock lock_guard(m_mutex);

		auto it = m_clients.find(uuid);
		if (it != m_clients.end()) {
			client = it->second;
			m_clients.erase(it);
		}

		auto thread_it = m_handle_msg_threads.find(uuid);
		if (thread_it != m_handle_msg_threads.end()) {
			if (thread_it->second.get_id() == std::this_thread::get_id()) {
				// Called from the worker itself: detach so we never self-join
				if (thread_it->second.joinable()) {
					thread_it->second.detach();
				}
				m_handle_msg_threads.erase(thread_it);
			} else {
				thread_to_join = std::move(thread_it->second);
				m_handle_msg_threads.erase(thread_it);
			}
		}
	}

	// Socket I/O outside the map lock
	if (client && client->Socket()) {
		client->Socket()->Disconnect();
		m_logger << Logger::Level::LowLevel << "Disconnected client: " << uuid << std::endl;
	}

	if (thread_to_join.joinable()) {
		thread_to_join.join();
	}
}

void Server::AcceptClients() noexcept {
	constexpr auto TIMEOUT = 1000000; // 1 second
	m_logger << Logger::Level::LowLevel << "Started accept clients thread" << std::endl;

	while (Connection::IsConnected(m_status.load())) {
		auto expected_wait = m_socket_server->WaitForData(TIMEOUT);
		if (!expected_wait) {
			m_logger << Logger::Level::Error << expected_wait.error()->what() << std::endl;
			return;
		}

		switch (expected_wait.value()) {
			case Connection::Read::Result::Success: {
				auto expected_client = m_socket_server->Accept();
				if (!expected_client) {
					// Transient accept failure (e.g. raced with disconnect): keep listening
					if (!Connection::IsConnected(m_status.load())) {
						return;
					}
					m_logger << Logger::Level::LowLevel << expected_client.error()->what() << std::endl;
					break;
				}

				const std::string client_uuid = expected_client.value()->UUID();
				{
					std::scoped_lock lock_guard(m_mutex);
					m_clients.emplace(client_uuid, CreateConnection(expected_client.value()));
					m_handle_msg_threads.emplace(
						client_uuid,
						std::thread(&Server::HandleClientCommunication, this, client_uuid));
				}
				m_logger << Logger::Level::LowLevel << "AcceptClients: accepted client uuid=" << client_uuid << std::endl;
				break;
			}

			case Connection::Read::Result::Timeout:
				// Idle listen socket — loop again without yield spin
				continue;

			case Connection::Read::Result::Closed:
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
			m_logger << Logger::Level::LowLevel << "Client uuid=" << client_uuid
					<< " not found; ending communication thread" << std::endl;
			return;
		}
		client = it->second;
	}

	while (Connection::IsConnected(m_status.load()) && Connection::IsConnected(client->Status())) {
		auto expected_wait = client->Socket()->WaitForData();
		if (!expected_wait) {
			m_logger << Logger::Level::Error << expected_wait.error()->what() << std::endl;
			break;
		}

		switch (expected_wait.value()) {
			case Connection::Read::Result::Success: {
				PacketPointer packet;
				{
					Transport::Frame frame = client->Receive(m_logger);
					packet = frame.ProcessPacket(m_deserialize_packet_function, m_logger);
				}
				if (!packet) {
					m_logger << Logger::Level::Error << "Failed to process packet from client="
							<< client_uuid << std::endl;
					break;
				}

				if (!Connection::IsConnected(m_status.load())) {
					break;
				}

				PacketPointer response_packet = ProcessClientPacket(client_uuid, packet);
				if (!response_packet) {
					m_logger << Logger::Level::Error
							<< "HandleClientCommunication: response packet was null" << std::endl;
					break;
				}

				if (client->Socket()->HasShutdownRequest() || !Connection::IsConnected(m_status.load())) {
					break;
				}

				Reply(client, *response_packet);
				continue; // success path: wait for next message
			}

			case Connection::Read::Result::Closed:
			case Connection::Read::Result::ShutdownRequest:
				break;

			case Connection::Read::Result::Timeout:
				// No data this slice — keep waiting (no yield, no log spam)
				continue;

			default:
				continue;
		}

		break; // Closed / error / null packet
	}

	DisconnectClient(client_uuid);
	m_logger << Logger::Level::LowLevel << "Stopped communication thread for client uuid="
			<< client_uuid << std::endl;
}
