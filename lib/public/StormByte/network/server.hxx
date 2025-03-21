#pragma once

#include <StormByte/expected.hxx>
#include <StormByte/network/connection/handler.hxx>
#include <StormByte/network/connection/info.hxx>
#include <StormByte/network/connection/status.hxx>
#include <StormByte/network/exception.hxx>
#include <StormByte/network/socket/client.hxx>
#include <StormByte/network/socket/server.hxx>
#include <StormByte/util/buffer.hxx>

#include <atomic>
#include <list>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

/**
 * @namespace Network
 * @brief The namespace containing all the network related classes.
 */
namespace StormByte::Network {
	/**
	 * @class Server
	 * @brief The class representing a server.
	 */
	class STORMBYTE_NETWORK_PUBLIC Server {
		public:
			/**
			 * @brief The constructor of the Server class.
			 * @param handler The handler of the server.
			 */
			Server(std::shared_ptr<const Connection::Handler> handler);

			/**
			 * @brief The copy constructor of the Server class.
			 * @param other The other server to copy.
			 */
			Server(const Server& other)										= delete;

			/**
			 * @brief The move constructor of the Server class.
			 * @param other The other server to move.
			 */
			Server(Server&& other) noexcept									= default;

			/**
			 * @brief The destructor of the Server class.
			 */
			~Server() noexcept;

			/**
			 * @brief The assignment operator of the Server class.
			 * @param other The other server to assign.
			 * @return The reference to the assigned server.
			 */
			Server& operator=(const Server& other)							= delete;

			/**
			 * @brief The assignment operator of the Server class.
			 * @param other The other server to assign.
			 * @return The reference to the assigned server.
			 */
			Server& operator=(Server&& other) noexcept						= default;

			/**
			 * @brief The function to connect a server.
			 * @param hostname The hostname of the server.
			 * @param port The port of the server.
			 * @param protocol The protocol of the server.
			 * @return The expectedn void or error.
			 */
			StormByte::Expected<void, ConnectionError>						Connect(const std::string& hostname, const unsigned short& port, const Connection::Protocol& protocol) noexcept;

			/**
			 * @brief The function to disconnect the server.
			 */
			void 															Disconnect() noexcept;

		protected:
			/**
			 * @brief The function to negotiate a connection, the base class will accept the connection.
			 * @param client The client to negotiate with.
			 * @return The result of the negotiation: true will accept finally the client but false will drop its connection.
			 */
			virtual bool 													NegotiateConnection(Socket::Client& client) noexcept;

			/**
			 * @brief The function to preprocess a message from a client (for example, decrypt it).
			 * @param client The client to process the message for.
			 * @param message The message to process.
			 * @return True if the message is valid, false otherwise.
			 */
			virtual bool 													PreProcessMessage(Socket::Client& client, Util::Buffer& message) noexcept;

			/**
			 * @brief The function to process a message from a client.
			 * @param client The client to process the message for.
			 * @param message The message to process.
			 * @return The response to the message.
			 */
			virtual Util::Buffer 											ProcessMessage(Socket::Client& client, const Util::Buffer& message) noexcept;

		private:
			std::unique_ptr<Socket::Server> m_socket;						///< The socket of the server.
			std::atomic<Connection::Status> m_status;						///< The status of the server.
			std::thread m_acceptThread;										///< The thread to accept incoming connections.
			std::list<std::thread> m_clientThreads;							///< The threads to communicate with clients connections.
            std::vector<std::unique_ptr<Socket::Client>> m_clients;			///< The clients connected to the server.
            std::mutex m_clientsMutex;										///< The mutex to protect the clients vector.
			std::shared_ptr<const Connection::Handler> m_handler;			///< The handler of the server.

			/**
			 * @brief The function to accept clients.
			 */
			void 															AcceptClients();

			/**
			 * @brief Adds a client to the server.
			 * @param client The client to add.
			 */
			Socket::Client& 												AddClient(Socket::Client&& client);

			/**
			 * @brief Removes a client from the server.
			 * @param client The client to remove.
			 */
			void 															RemoveClient(Socket::Client& client) noexcept;

			/**
			 * @brief Removes all clients from the server.
			 */
			void 															RemoveAllClients() noexcept;
			
			/**
			 * @brief The function to handle client messages.
			 * @param client The client to handle messages for.
			 */
			void 															HandleClientMessages(Socket::Client& client) noexcept;

			/**
			 * @brief The function to handle a read message from a client.
			 * @param client The client to read the message from.
			 * @return Buffer
			 */
			StormByte::Expected<Util::Buffer, ConnectionClosed>				HandleClientMessage(Socket::Client& client) noexcept;

			/**
			 * @brief The function to start client communication
			 * @param client The client to communicate with
			 */
			void 															StartClientCommunication(Socket::Client& client) noexcept;
	};
}