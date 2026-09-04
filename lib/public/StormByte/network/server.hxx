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

#pragma once

#include <StormByte/network/endpoint.hxx>

#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_map>

/**
 * @brief Network module of the StormByte suite.
 */
namespace StormByte::Network {
	namespace Connection {
		class Client;	///< Forward declaration
	}

	namespace Socket {
		class Server;	///< Forward declaration
	}

	/**
	 * @class Server
	 * @brief Abstract application server.
	 *
	 * Listen socket, accept loop and per-client workers. Implement ProcessClientPacket(); override pipelines as needed.
	 *
	 * @note Inheritance-oriented. Subclass required.
	 */
	class STORMBYTE_NETWORK_PUBLIC Server: private Endpoint {
		public:
			/**
			 * @brief Construct with a packet factory and a logger.
			 * @param deserialize_packet_function Builds domain packets from wire data.
			 * @param logger Diagnostic logger.
			 */
			Server(const DeserializePacketFunction& deserialize_packet_function, std::shared_ptr<Logger::Log> logger) noexcept;

			/**
			 * @brief Copy constructor (deleted).
			 */
			Server(const Server& other) = delete;

			/**
			 * @brief Move constructor.
			 */
			Server(Server&& other) noexcept = default;

			/**
			 * @brief Destructor (joins threads, disconnects clients).
			 */
			virtual ~Server() noexcept;

			/**
			 * @brief Copy assignment (deleted).
			 */
			Server& operator=(const Server& other) = delete;

			/**
			 * @brief Move assignment.
			 */
			Server& operator=(Server&& other) noexcept = default;

			/**
			 * @brief Bind, listen and start the accept thread.
			 * @param protocol Address family.
			 * @param address Bind address.
			 * @param port Port number.
			 * @return true on success.
			 */
			bool Connect(const Connection::Protocol& protocol, const std::string& address, const unsigned short& port) override;

			/**
			 * @brief Stop accept, disconnect clients and close the listener.
			 */
			void Disconnect() noexcept override;

			/**
			 * @brief Listener / server status.
			 * @return Status.
			 */
			inline Connection::Status Status() const noexcept override {
				return m_status.load();
			}

		protected:
			/**
			 * @brief Disconnect a client by UUID.
			 * @param uuid Client UUID.
			 */
			void DisconnectClient(const std::string& uuid) noexcept;

		private:
			std::unique_ptr<Socket::Server> m_socket_server;											///< Listen socket
			std::atomic<Connection::Status> m_status;												///< Server status
			std::thread m_accept_thread;															///< Accept loop thread
			std::unordered_map<std::string, std::shared_ptr<Connection::Client>> m_clients;		///< Active clients
			std::unordered_map<std::string, std::thread> m_handle_msg_threads;						///< Per-client workers
			std::mutex m_mutex;																		///< Protects client maps

			/**
			 * @brief Accept-loop thread body.
			 */
			void AcceptClients() noexcept;

			/**
			 * @brief Per-client communication thread body.
			 * @param client_uuid Client UUID.
			 */
			void HandleClientCommunication(const std::string& client_uuid) noexcept;

			/**
			 * @brief Application packet handler.
			 * @param client_uuid Sender UUID.
			 * @param packet Received packet.
			 * @return Response packet, or nullptr on error / no reply.
			 */
			virtual PacketPointer ProcessClientPacket(const std::string& client_uuid, PacketPointer packet) noexcept = 0;
	};
}
