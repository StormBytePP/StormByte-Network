#pragma once

#include <StormByte/expected.hxx>
#include <StormByte/network/address.hxx>
#include <StormByte/network/exception.hxx>
#include <StormByte/network/handler.hxx>
#include <StormByte/network/status.hxx>

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

/**
 * @namespace Network
 * @brief The namespace containing all the network related classes.
 */
namespace StormByte::Network {
	// Forward declarations
	class Address;
	namespace Socket {
		class Server;
		class Client;
	}

	/**
	 * @class Server
	 * @brief The class representing a server.
	 */
	class STORMBYTE_NETWORK_PUBLIC Server {
		public:
			/**
			 * @brief The constructor of the Server class.
			 * @param address The address of the server.
			 * @param handler The handler of the server.
			 */
			Server(const Address& address, std::shared_ptr<const Handler> handler);

			/**
			 * @brief The constructor of the Server class.
			 * @param address The address of the server.
			 * @param handler The handler of the server.
			 */
			Server(Address&& address, std::shared_ptr<const Handler> handler);

			/**
			 * @brief The copy constructor of the Server class.
			 * @param address The address of the server.
			 */
			Server(const Server& other)								= delete;

			/**
			 * @brief The move constructor of the Server class.
			 * @param address The address of the server.
			 */
			Server(Server&& other) noexcept							= delete;

			/**
			 * @brief The destructor of the Server class.
			 */
			~Server() noexcept;

			/**
			 * @brief The assignment operator of the Server class.
			 * @param other The other server to assign.
			 * @return The reference to the assigned server.
			 */
			Server& operator=(const Server& other)					= delete;

			/**
			 * @brief The assignment operator of the Server class.
			 * @param other The other server to assign.
			 * @return The reference to the assigned server.
			 */
			Server& operator=(Server&& other) noexcept;

			/**
			 * @brief The function to connect a server.
			 * @return The the server object.
			 */
			StormByte::Expected<void, ConnectionError>				Connect() noexcept;

			/**
			 * @brief The function to disconnect the server.
			 */
			void 													Disconnect() noexcept;

		private:
			std::unique_ptr<Address> m_address;						///< The address of the server.
			std::unique_ptr<Socket::Server> m_socket;				///< The address of the server.
			std::atomic<Status> m_status;							///< The status of the server.
			std::thread m_acceptThread;								///< The thread to accept incoming connections.
            std::vector<std::unique_ptr<Socket::Client>> m_clients;	///< The clients connected to the server.
            std::mutex m_clientsMutex;								///< The mutex to protect the clients vector.
			std::shared_ptr<const Handler> m_handler;				///< The handler of the server.

			/**
			 * @brief The function to accept clients.
			 */
			void 													AcceptClients();
			
			/**
			 * @brief The function to handle client messages.
			 */
			void 													HandleClientMessages();
	};
}