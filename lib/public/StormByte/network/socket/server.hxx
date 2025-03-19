#pragma once

#include <StormByte/network/socket/client.hxx>

/**
 * @namespace Socket
 * @brief The namespace containing all the socket related classes.
 */
namespace StormByte::Network::Socket {
	/**
	 * @class Server
	 * @brief The class representing a server socket.
	 */
	class STORMBYTE_NETWORK_PUBLIC Server final: public Socket {
		public:
			/**
			 * @brief The constructor of the Server class.
			 * @param address The address to create the socket.
			 * @param handler The handler of the socket.
			 */
			Server(const Address& address, std::shared_ptr<const Handler> handler);

			/**
			 * @brief The copy constructor of the Server class.
			 * @param other The other socket to copy.
			 */
			Server(const Server& other) 						= delete;

			/**
			 * @brief The move constructor of the Server class.
			 * @param other The other socket to move.
			 */
			Server(Server&& other) noexcept						= default;

			/**
			 * @brief The destructor of the Server class.
			 */
			~Server() noexcept override							= default;

			/**
			 * @brief The assignment operator of the Server class.
			 * @param other The other socket to assign.
			 * @return The reference to the assigned socket.
			 */
			Server& operator=(const Server& other) 				= delete;

			/**
			 * @brief The move assignment operator of the Server class.
			 * @param other The other socket to assign.
			 * @return The reference to the assigned socket.
			 */
			Server& operator=(Server&& other) noexcept			= default;

			/**
			 * @brief The function to listen for incoming connections.
			 * @return The expected result of the operation.
			 */
			StormByte::Expected<void, ConnectionError>			Listen() noexcept;

			/**
			 * @brief The function to accept a client connection.
			 * @return The expected result of the operation.
			 */
			StormByte::Expected<Client, ConnectionError>		Accept() noexcept;
	};
}