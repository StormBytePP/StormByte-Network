/*
 * Copyright (C) 2024-2026 David C. Manuelda (StormBytePP)
 *
 * This file is part of StormByte.
 *
 * StormByte is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * StormByte is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with StormByte. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <StormByte/buffer/pipeline.hxx>
#include <StormByte/logger/threaded_log.hxx>
#include <StormByte/network/typedefs.hxx>

/**
 * @namespace StormByte::Network
 * @brief StormByte networking subsystem.
 */
namespace StormByte::Network {
	namespace Connection {
		class Client;	///< Forward declaration
	}

	/**
	 * @class Endpoint
	 * @brief Shared base for Client and Server endpoints.
	 *
	 * Not instantiated directly. Override @ref InputPipeline() /
	 * @ref OutputPipeline() for buffer stages; use @ref Send() / @ref Reply()
	 * for framed request/response.
	 *
	 * @note **Inheritance-oriented.** Derive application clients/servers from
	 * @ref Client / @ref Server, not from Endpoint alone.
	 */
	class STORMBYTE_NETWORK_PUBLIC Endpoint {
		public:
			/**
			 * @param deserialize_packet_function Builds domain packets from wire data.
			 * @param logger Diagnostic logger.
			 */
			Endpoint(const DeserializePacketFunction& deserialize_packet_function, std::shared_ptr<Logger::Log> logger) noexcept;

			/**
			 * Copy constructor (deleted).
			 */
			Endpoint(const Endpoint& other) = delete;

			/**
			 * Move constructor.
			 */
			Endpoint(Endpoint&& other) noexcept = default;

			/**
			 * Destructor.
			 */
			virtual ~Endpoint() noexcept = default;

			/**
			 * Copy assignment (deleted).
			 */
			Endpoint& operator=(const Endpoint& other) = delete;

			/**
			 * Move assignment.
			 */
			Endpoint& operator=(Endpoint&& other) noexcept = default;

			/**
			 * Connects or listens (meaning depends on derived class).
			 * @param protocol Address family.
			 * @param address Host or bind address.
			 * @param port Port number.
			 * @return true on success.
			 */
			virtual bool Connect(const Connection::Protocol& protocol, const std::string& address, const unsigned short& port) = 0;

			/**
			 * Tears down the endpoint.
			 */
			virtual void Disconnect() noexcept = 0;

			/**
			 * @return Current connection/listen status.
			 */
			virtual Connection::Status Status() const noexcept = 0;

		protected:
			DeserializePacketFunction m_deserialize_packet_function;	///< Packet factory
			std::shared_ptr<Logger::Log> m_logger;						///< Logger

			/**
			 * Wraps a socket client with input/output pipelines.
			 * @param socket Underlying socket client.
			 * @return Connection::Client shared pointer.
			 */
			std::shared_ptr<Connection::Client> CreateConnection(std::shared_ptr<Socket::Client> socket) noexcept;

			/**
			 * @return Pipeline applied to inbound frame payloads.
			 */
			virtual Buffer::Pipeline InputPipeline() const noexcept = 0;

			/**
			 * @return Pipeline applied to outbound frame payloads.
			 */
			virtual Buffer::Pipeline OutputPipeline() const noexcept = 0;

			/**
			 * Sends @p packet and waits for a response frame.
			 * @param client_connection Active connection.
			 * @param packet Packet to send.
			 * @return Response packet, or nullptr on failure.
			 */
			PacketPointer Send(std::shared_ptr<Connection::Client> client_connection, const Transport::Packet& packet) noexcept;

			/**
			 * Sends @p packet without waiting for a reply.
			 * @param client_connection Active connection.
			 * @param packet Packet to send.
			 * @return true on success.
			 */
			bool Reply(std::shared_ptr<Connection::Client> client_connection, const Transport::Packet& packet) noexcept;

		private:
			/**
			 * Internal send helper (no receive).
			 * @param client_connection Active connection.
			 * @param packet Packet to send.
			 * @return true on success.
			 */
			bool SendPacket(std::shared_ptr<Connection::Client> client_connection, const Transport::Packet& packet) noexcept;
	};
}
