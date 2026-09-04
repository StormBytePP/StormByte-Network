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

#include <StormByte/network/socket/client.hxx>
#include <StormByte/network/typedefs.hxx>

#include <vector>

/**
 * @brief Socket wrappers of the Network module.
 */
namespace StormByte::Network::Socket {
	/**
	 * @class Server
	 * @brief Listening socket: bind, listen, accept.
	 */
	class STORMBYTE_NETWORK_PRIVATE Server final: public Socket {
		public:
			/**
			 * @brief Construct with protocol and logger.
			 * @param protocol Address family.
			 * @param logger Logger.
			 */
			Server(const Connection::Protocol& protocol, std::shared_ptr<Logger::Log> logger) noexcept;

			/**
			 * @brief Copy constructor (deleted).
			 */
			Server(const Server& other) = delete;

			/**
			 * @brief Move constructor.
			 */
			Server(Server&& other) noexcept = default;

			/**
			 * @brief Destructor.
			 */
			~Server() noexcept override = default;

			/**
			 * @brief Copy assignment (deleted).
			 */
			Server& operator=(const Server& other) = delete;

			/**
			 * @brief Move assignment.
			 */
			Server& operator=(Server&& other) noexcept = default;

			/**
			 * @brief Bind and listen on host:port.
			 * @param hostname Bind address.
			 * @param port Port.
			 * @return Empty Expected on success.
			 */
			ExpectedVoid Listen(const std::string& hostname, const unsigned short& port) noexcept;

			/**
			 * @brief Accept one client.
			 * @return Shared Client or error.
			 */
			ExpectedClient Accept() noexcept;

			/**
			 * @brief Disconnect all accepted clients then the listener.
			 */
			void Disconnect() noexcept override;

			/**
			 * @brief Disconnect one accepted client by UUID.
			 * @param client_uuid Client UUID.
			 */
			void DisconnectClient(const std::string& client_uuid) noexcept;

		private:
			std::vector<std::shared_ptr<Client>> m_active_clients;	///< Accepted clients
	};
}
