#pragma once

#include <StormByte/network/socket/socket.hxx>

/**
 * @namespace Socket
 * @brief The namespace containing all the socket related classes.
 */
namespace StormByte::Network::Socket {
	/**
	 * @class Client
	 * @brief The class representing a client socket.
	 */
	class STORMBYTE_NETWORK_PRIVATE Client: public Socket {
		friend class Server;
		public:
			/**
			 * @brief The constructor of the Client class.
			 * @param address The address to create the socket.
			 * @param handler The handler of the socket.
			 */
			Client(const Address& address, std::shared_ptr<const Handler> handler);

			/**
			 * @brief The copy constructor of the Client class.
			 * @param other The other socket to copy.
			 */
			Client(const Client& other) 					= delete;

			/**
			 * @brief The move constructor of the Client class.
			 * @param other The other socket to move.
			 */
			Client(Client&& other) noexcept					= default;

			/**
			 * @brief The destructor of the Client class.
			 */
			~Client() noexcept override							= default;

			/**
			 * @brief The assignment operator of the Client class.
			 * @param other The other socket to assign.
			 * @return The reference to the assigned socket.
			 */
			Client& operator=(const Client& other) 			= delete;

			/**
			 * @brief The move assignment operator of the Client class.
			 * @param other The other socket to assign.
			 * @return The reference to the assigned socket.
			 */
			Client& operator=(Client&& other) noexcept		= default;

			/**
			 * @brief The function to connect to a server.
			 * @return The expected result of the operation.
			 */
			StormByte::Expected<void, ConnectionError>		Connect() noexcept;
	};
}