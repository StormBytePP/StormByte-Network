#pragma once

#include <StormByte/network/socket/client.hxx>
#include <StormByte/network/typedefs.hxx>

/**
 * @namespace Socket
 * @brief The namespace containing all the socket related classes.
 */
namespace StormByte::Network::Socket {
	/**
	 * @class Server
	 * @brief The class representing a server socket.
	 */
	class STORMBYTE_NETWORK_PRIVATE Server final: public Socket {
		public:
			/**
			 * @brief The constructor of the Server class.
			 * @param protocol The protocol of the socket.
			 * @param handler The handler of the socket.
			 */
			Server(const Connection::Protocol& protocol, std::shared_ptr<const Connection::Handler> handler, std::shared_ptr<Logger> logger) noexcept;

			/**
			 * @brief The copy constructor of the Server class.
			 * @param other The other socket to copy.
			 */
			Server(const Server& other) 									= delete;

			/**
			 * @brief The move constructor of the Server class.
			 * @param other The other socket to move.
			 */
			Server(Server&& other) noexcept									= default;

			/**
			 * @brief The destructor of the Server class.
			 */
			~Server() noexcept override										= default;

			/**
			 * @brief The assignment operator of the Server class.
			 * @param other The other socket to assign.
			 * @return The reference to the assigned socket.
			 */
			Server& operator=(const Server& other) 							= delete;

			/**
			 * @brief The move assignment operator of the Server class.
			 * @param other The other socket to assign.
			 * @return The reference to the assigned socket.
			 */
			Server& operator=(Server&& other) noexcept						= default;

			/**
			 * @brief The function to listen for incoming connections.
			 * @return The expected result of the operation.
			 */
			ExpectedVoid													Listen(const std::string& hostname, const unsigned short& port) noexcept;

			/**
			 * @brief The function to accept a client connection.
			 * @return The expected result of the operation.
			 */
			ExpectedClient													Accept() noexcept;
	};
}