#pragma once

#include <StormByte/expected.hxx>
#include <StormByte/network/connection/handler.hxx>
#include <StormByte/network/connection/status.hxx>
#include <StormByte/network/exception.hxx>
#include <StormByte/network/socket/client.hxx>

/**
 * @namespace Network
 * @brief The namespace containing all the network related classes.
 */
namespace StormByte::Network {
	/**
	 * @class Client
	 * @brief The class representing a client.
	 */
	class STORMBYTE_NETWORK_PUBLIC Client {
		public:
			/**
			 * @brief The constructor of the Client class.
			 * @param address The address of the server.
			 * @param handler The handler of the server.
			 */
			Client(std::shared_ptr<const Connection::Handler> handler, std::shared_ptr<Logger> logger) noexcept;

			/**
			 * @brief The copy constructor of the Client class.
			 * @param other The other server to copy.
			 */
			Client(const Client& other)										= delete;

			/**
			 * @brief The move constructor of the Client class.
			 * @param other The other server to move.
			 */
			Client(Client&& other) noexcept									= default;

			/**
			 * @brief The destructor of the Client class.
			 */
			~Client() noexcept;

			/**
			 * @brief The assignment operator of the Client class.
			 * @param other The other server to assign.
			 * @return The reference to the assigned server.
			 */
			Client& operator=(const Client& other)							= delete;

			/**
			 * @brief The assignment operator of the Client class.
			 * @param other The other server to assign.
			 * @return The reference to the assigned server.
			 */
			Client& operator=(Client&& other) noexcept						= default;

			/**
			 * @brief The function to connect a server.
			 * @param hostname The hostname of the server.
			 * @param port The port of the server.
			 * @param protocol The protocol of the server.
			 * @return The expected void or error.
			 */
			StormByte::Expected<void, ConnectionError>						Connect(const std::string& hostname, const unsigned short& port, const Connection::Protocol& protocol) noexcept;

			/**
			 * @brief The function to disconnect the server.
			 */
			void 															Disconnect() noexcept;

		private:
			std::unique_ptr<Socket::Client> m_socket;						///< The socket of the client.
			Connection::Status m_status;									///< The status of the client.
			std::shared_ptr<const Connection::Handler> m_handler;			///< The handler of the client.
			std::shared_ptr<Logger> m_logger;								///< The logger of the client.
	};
}