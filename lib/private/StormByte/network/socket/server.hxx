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

#include <StormByte/network/socket/client.hxx>
#include <StormByte/network/typedefs.hxx>
#include <vector>

/**
 * @namespace Socket
 * @brief Low-level socket wrappers.
 */
namespace StormByte::Network::Socket {
	/**
	 * @class Server
	 * @brief Listening socket: bind, listen, accept.
	 */
	class STORMBYTE_NETWORK_PRIVATE Server final: public Socket {
		public:
			/**
			 * @param protocol Address family.
			 * @param logger Logger.
			 */
			Server(const Connection::Protocol& protocol, std::shared_ptr<Logger::Log> logger) noexcept;

			/**
			 * Copy constructor (deleted).
			 */
			Server(const Server& other) = delete;

			/**
			 * Move constructor.
			 */
			Server(Server&& other) noexcept = default;

			/**
			 * Destructor.
			 */
			~Server() noexcept override = default;

			/**
			 * Copy assignment (deleted).
			 */
			Server& operator=(const Server& other) = delete;

			/**
			 * Move assignment.
			 */
			Server& operator=(Server&& other) noexcept = default;

			/**
			 * Binds and listens on host:port.
			 * @param hostname Bind address.
			 * @param port Port.
			 * @return Empty Expected on success.
			 */
			ExpectedVoid Listen(const std::string& hostname, const unsigned short& port) noexcept;

			/**
			 * Accepts one client (with short poll/select wait).
			 * @return Shared Client or error.
			 */
			ExpectedClient Accept() noexcept;

			/**
			 * Disconnects all accepted clients then the listener.
			 */
			void Disconnect() noexcept override;

			/**
			 * Disconnects one accepted client by UUID.
			 * @param client_uuid Client UUID.
			 */
			void DisconnectClient(const std::string& client_uuid) noexcept;

		private:
			std::vector<std::shared_ptr<Client>> m_active_clients;	///< Accepted clients
	};
}
