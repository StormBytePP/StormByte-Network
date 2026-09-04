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

#include <StormByte/buffer/pipeline.hxx>
#include <StormByte/logger/threaded_log.hxx>
#include <StormByte/network/typedefs.hxx>

/**
 * @brief Network module of the StormByte suite.
 */
namespace StormByte::Network {
	namespace Connection {
		class Client;	///< Forward declaration
	}

	/**
	 * @class Endpoint
	 * @brief Shared base for Client and Server.
	 *
	 * Not instantiated directly. Override InputPipeline() / OutputPipeline(). Use Send() / Reply() for framed request/response.
	 *
	 * @note Inheritance-oriented. Derive from Client / Server, not from Endpoint alone.
	 */
	class STORMBYTE_NETWORK_PUBLIC Endpoint {
		public:
			/**
			 * @brief Construct with a packet factory and a logger.
			 * @param deserialize_packet_function Builds domain packets from wire data.
			 * @param logger Diagnostic logger.
			 */
			Endpoint(const DeserializePacketFunction& deserialize_packet_function, std::shared_ptr<Logger::Log> logger) noexcept;

			/**
			 * @brief Copy constructor (deleted).
			 */
			Endpoint(const Endpoint& other) = delete;

			/**
			 * @brief Move constructor.
			 */
			Endpoint(Endpoint&& other) noexcept = default;

			/**
			 * @brief Destructor.
			 */
			virtual ~Endpoint() noexcept = default;

			/**
			 * @brief Copy assignment (deleted).
			 */
			Endpoint& operator=(const Endpoint& other) = delete;

			/**
			 * @brief Move assignment.
			 */
			Endpoint& operator=(Endpoint&& other) noexcept = default;

			/**
			 * @brief Connect or listen (meaning depends on the derived class).
			 * @param protocol Address family.
			 * @param address Host or bind address.
			 * @param port Port number.
			 * @return true on success.
			 */
			virtual bool Connect(const Connection::Protocol& protocol, const std::string& address, const unsigned short& port) = 0;

			/**
			 * @brief Tear down the endpoint.
			 */
			virtual void Disconnect() noexcept = 0;

			/**
			 * @brief Current connection/listen status.
			 * @return Status.
			 */
			virtual Connection::Status Status() const noexcept = 0;

		protected:
			DeserializePacketFunction m_deserialize_packet_function;	///< Packet factory
			std::shared_ptr<Logger::Log> m_logger;						///< Logger

			/**
			 * @brief Wrap a socket client with input/output pipelines.
			 * @param socket Underlying socket client.
			 * @return Connection::Client.
			 */
			std::shared_ptr<Connection::Client> CreateConnection(std::shared_ptr<Socket::Client> socket) noexcept;

			/**
			 * @brief Pipeline applied to inbound frame payloads.
			 * @return Pipeline.
			 */
			virtual Buffer::Pipeline InputPipeline() const noexcept = 0;

			/**
			 * @brief Pipeline applied to outbound frame payloads.
			 * @return Pipeline.
			 */
			virtual Buffer::Pipeline OutputPipeline() const noexcept = 0;

			/**
			 * @brief Send @p packet and wait for a response frame.
			 * @param client_connection Active connection.
			 * @param packet Packet to send.
			 * @return Response packet, or nullptr on failure.
			 */
			PacketPointer Send(std::shared_ptr<Connection::Client> client_connection, const Transport::Packet& packet) noexcept;

			/**
			 * @brief Send @p packet without waiting for a reply.
			 * @param client_connection Active connection.
			 * @param packet Packet to send.
			 * @return true on success.
			 */
			bool Reply(std::shared_ptr<Connection::Client> client_connection, const Transport::Packet& packet) noexcept;

		private:
			/**
			 * @brief Internal send (no receive).
			 * @param client_connection Active connection.
			 * @param packet Packet to send.
			 * @return true on success.
			 */
			bool SendPacket(std::shared_ptr<Connection::Client> client_connection, const Transport::Packet& packet) noexcept;
	};
}
