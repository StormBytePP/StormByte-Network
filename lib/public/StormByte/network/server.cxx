/*
 * Copyright (C) 2024-2026 David C. Manuelda (StormBytePP)
 *
 * This file is part of StormByte-Network.
 *
 * StormByte-Network is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3
 * or later, as published by the Free Software Foundation.
 *
 * StormByte-Network is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with StormByte-Network. If not, see
 * <https://www.gnu.org/licenses/lgpl-3.0.html>.
 */

#include <StormByte/network/connection/client.hxx>
#include <StormByte/network/event_loop.hxx>
#include <StormByte/network/server.hxx>
#include <StormByte/network/session.hxx>
#include <StormByte/network/socket/server.hxx>
#ifdef UNIX
#include <unistd.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
using namespace StormByte::Network;
Server::Server(const DeserializePacketFunction& deserialize_packet_function, std::shared_ptr<Logger::Log> logger) noexcept:
	Endpoint(deserialize_packet_function, logger),
	m_socket_server(nullptr),
	m_status(Connection::Status::Disconnected),
	m_accept_thread(),
#ifdef WINDOWS
	m_wakeup_read(INVALID_SOCKET),
	m_wakeup_write(INVALID_SOCKET)
#else
	m_wakeup_read(-1),
	m_wakeup_write(-1)
#endif
{}
Server::Server(Server&& other) noexcept:
	Endpoint(std::move(other)),
	m_socket_server(std::move(other.m_socket_server)),
	m_status(other.m_status.load(std::memory_order_relaxed)),
	m_accept_thread(std::move(other.m_accept_thread)),
	m_wakeup_read(other.m_wakeup_read),
	m_wakeup_write(other.m_wakeup_write),
	m_clients(std::move(other.m_clients)),
	m_handle_msg_threads(std::move(other.m_handle_msg_threads)) {
#ifdef WINDOWS
	other.m_wakeup_read = INVALID_SOCKET;
	other.m_wakeup_write = INVALID_SOCKET;
#else
	other.m_wakeup_read = -1;
	other.m_wakeup_write = -1;
#endif
	other.m_status.store(Connection::Status::Disconnected, std::memory_order_relaxed);
}
Server::~Server() noexcept {
	Disconnect();
}
Server& Server::operator=(Server&& other) noexcept {
	if (this != &other) {
		Disconnect();
		Endpoint::operator=(std::move(other));
		m_socket_server = std::move(other.m_socket_server);
		m_status.store(other.m_status.load(std::memory_order_relaxed), std::memory_order_relaxed);
		m_accept_thread = std::move(other.m_accept_thread);
		m_wakeup_read = other.m_wakeup_read;
		m_wakeup_write = other.m_wakeup_write;
		m_clients = std::move(other.m_clients);
		m_handle_msg_threads = std::move(other.m_handle_msg_threads);
#ifdef WINDOWS
		other.m_wakeup_read = INVALID_SOCKET;
		other.m_wakeup_write = INVALID_SOCKET;
#else
		other.m_wakeup_read = -1;
		other.m_wakeup_write = -1;
#endif
		other.m_status.store(Connection::Status::Disconnected, std::memory_order_relaxed);
	}
	return *this;
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
		if (!CreateWakeup()) {
			m_logger << Logger::Level::Error << "Failed to create server wakeup channel" << std::endl;
			m_socket_server->Disconnect();
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
	// 2) Wake AcceptClients without tearing down the listener from another thread.
	SignalWakeup();
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
	CloseWakeup();
	m_status.store(Connection::Status::Disconnected, std::memory_order_release);
}

bool Server::CreateWakeup() noexcept {
#ifdef UNIX
	int handles[2];
	if (::pipe(handles) != 0) {
		return false;
	}
	m_wakeup_read = handles[0];
	m_wakeup_write = handles[1];
	return true;
#else
	m_wakeup_read = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	m_wakeup_write = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (m_wakeup_read == INVALID_SOCKET || m_wakeup_write == INVALID_SOCKET) {
		CloseWakeup();
		return false;
	}
	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	address.sin_port = 0;
	if (::bind(m_wakeup_read, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) {
		CloseWakeup();
		return false;
	}
	int address_size = sizeof(address);
	if (::getsockname(m_wakeup_read, reinterpret_cast<sockaddr*>(&address), &address_size) == SOCKET_ERROR ||
		::connect(m_wakeup_write, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) {
		CloseWakeup();
		return false;
	}
	return true;
#endif
}

void Server::SignalWakeup() noexcept {
#ifdef WINDOWS
	if (m_wakeup_write == INVALID_SOCKET) {
		return;
	}
#else
	if (m_wakeup_write < 0) {
		return;
	}
#endif
	const char signal = 1;
#ifdef UNIX
	[[maybe_unused]] const ssize_t written = ::write(m_wakeup_write, &signal, sizeof(signal));
#else
	(void)::send(m_wakeup_write, &signal, sizeof(signal), 0);
#endif
}

void Server::CloseWakeup() noexcept {
#ifdef WINDOWS
	if (m_wakeup_read != INVALID_SOCKET) {
		closesocket(m_wakeup_read);
		m_wakeup_read = INVALID_SOCKET;
	}
	if (m_wakeup_write != INVALID_SOCKET) {
		closesocket(m_wakeup_write);
		m_wakeup_write = INVALID_SOCKET;
	}
#else
	if (m_wakeup_read >= 0) {
		close(m_wakeup_read);
		m_wakeup_read = -1;
	}
	if (m_wakeup_write >= 0) {
		close(m_wakeup_write);
		m_wakeup_write = -1;
	}
#endif
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
void Server::AcceptOneClient() noexcept {
	auto expected_client = m_socket_server->Accept();
	if (!expected_client) {
		if (Connection::IsConnected(m_status.load())) {
			m_logger << Logger::Level::LowLevel << expected_client.error()->what() << std::endl;
		}
		return;
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
}

void Server::AcceptClients() noexcept {
	m_logger << Logger::Level::LowLevel << "Started accept event loop" << std::endl;
	Detail::EventLoop event_loop(*m_socket_server, m_wakeup_read, m_status, m_logger);
	event_loop.Run([this]() noexcept {
		AcceptOneClient();
	});
	m_logger << Logger::Level::LowLevel << "Stopped accept event loop" << std::endl;
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
	Detail::Session session(client_uuid, client);
	while (Connection::IsConnected(m_status.load()) && Connection::IsConnected(session.Client()->Status())) {
		auto expected_wait = client->Socket()->WaitForData();
		if (!expected_wait) {
			m_logger << Logger::Level::Error << expected_wait.error()->what() << std::endl;
			break;
		}
		switch (expected_wait.value()) {
			case Connection::Read::Result::Success: {
				auto expected_frames = session.AppendAndTakeFrames(client->InputPipeline(), m_logger);
				if (!expected_frames) {
					m_logger << Logger::Level::Error << "Failed to process frame from client="
							<< client_uuid << ": " << expected_frames.error()->what() << std::endl;
					break;
				}
				bool stop_client = false;
				for (auto& frame : expected_frames.value()) {
					PacketPointer packet = frame.ProcessPacket(m_deserialize_packet_function, m_logger);
					if (!packet) {
						m_logger << Logger::Level::Error << "Failed to process packet from client="
								<< client_uuid << std::endl;
						stop_client = true;
						break;
					}
					if (!Connection::IsConnected(m_status.load())) {
						stop_client = true;
						break;
					}
					PacketPointer response_packet = ProcessClientPacket(client_uuid, packet);
					if (!response_packet) {
						m_logger << Logger::Level::Error
								<< "HandleClientCommunication: response packet was null" << std::endl;
						stop_client = true;
						break;
					}
					if (session.Client()->Socket()->HasShutdownRequest() || !Connection::IsConnected(m_status.load())) {
						stop_client = true;
						break;
					}
					Reply(session.Client(), *response_packet);
				}
				if (stop_client) {
					break;
				}
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
