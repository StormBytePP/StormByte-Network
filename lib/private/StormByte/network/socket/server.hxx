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
