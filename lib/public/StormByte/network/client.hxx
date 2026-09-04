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

#include <memory>
#include <string>

/**
 * @brief Network module of the StormByte suite.
 */
namespace StormByte::Network {
	namespace Connection {
		class Client;	///< Forward declaration
	}

	/**
	 * @class Client
	 * @brief Abstract application client.
	 *
	 * Derive and implement InputPipeline() / OutputPipeline(). Use protected Send() for request/response.
	 *
	 * @note Inheritance-oriented. Not for direct generic use without a subclass.
	 */
	class STORMBYTE_NETWORK_PUBLIC Client: private Endpoint {
		public:
			/**
			 * @brief Construct with a packet factory and a logger.
			 * @param deserialize_packet_function Builds domain packets from wire data.
			 * @param logger Diagnostic logger.
			 */
			inline Client(const DeserializePacketFunction& deserialize_packet_function, std::shared_ptr<Logger::Log> logger) noexcept:
				Endpoint(deserialize_packet_function, logger),
				m_connection(nullptr) {}

			/**
			 * @brief Copy constructor (deleted).
			 */
			Client(const Client& other) = delete;

			/**
			 * @brief Move constructor.
			 */
			Client(Client&& other) noexcept = default;

			/**
			 * @brief Destructor (out-of-line in .cxx).
			 */
			virtual ~Client() noexcept;

			/**
			 * @brief Copy assignment (deleted).
			 */
			Client& operator=(const Client& other) = delete;

			/**
			 * @brief Move assignment.
			 */
			Client& operator=(Client&& other) noexcept = default;

			/**
			 * @brief Connect to a remote host.
			 * @param protocol Address family.
			 * @param address Hostname or IP.
			 * @param port Port number.
			 * @return true on success.
			 */
			bool Connect(const Connection::Protocol& protocol, const std::string& address, const unsigned short& port) override;

			/**
			 * @brief Disconnect if connected.
			 */
			void Disconnect() noexcept override;

			/**
			 * @brief Current connection status.
			 * @return Status.
			 */
			Connection::Status Status() const noexcept override;

		protected:
			/**
			 * @brief Send @p packet and return the response (or nullptr).
			 * @param packet Request packet.
			 * @return Response, or nullptr on error.
			 */
			PacketPointer Send(const Transport::Packet& packet) noexcept;

		private:
			std::shared_ptr<Connection::Client> m_connection;	///< Active connection
	};
}
