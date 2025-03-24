#pragma once

#include <StormByte/network/exception.hxx>
#include <StormByte/network/socket/server.hxx>
#include <StormByte/network/socket/client.hxx>
#include <StormByte/logger/log.hxx>

#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace StormByte::Network {
	/**
	 * @class Server
	 * @brief A server class.
	 */
	class STORMBYTE_NETWORK_PUBLIC Server {
		public:
			/**
			 * @brief Constructor.
			 * @param protocol The protocol to use.
			 * @param handler The handler to use.
			 */
			Server(Connection::Protocol protocol, std::shared_ptr<Connection::Handler> handler, std::shared_ptr<Logger::Log> logger) noexcept;

			/**
			 * @brief Copy constructor (deleted).
			 * @param other The other instance.
			 */
			Server(const Server& other) 													= delete;

			/**
			 * @brief Move constructor.
			 * @param other The other instance.
			 */
			Server(Server&& other) noexcept													= default;

			/**
			 * @brief Destructor.
			 */
			virtual ~Server() noexcept;

			/**
			 * @brief Copy assignment operator (deleted).
			 * @param other The other instance.
			 * @return The new instance.
			 */
			Server& operator=(const Server& other) 											= delete;

			/**
			 * @brief Move assignment operator.
			 * @param other The other instance.
			 * @return The new instance.
			 */
			Server& operator=(Server&& other) noexcept 										= default;

			/**
			 * @brief Starts the server.
			 * @param host The host to use.
			 * @param port The port to use.
			 * @return True if the server started successfully, false otherwise.
			 */
			virtual StormByte::Expected<void, ConnectionError> 								Start(const std::string& host, const unsigned short& port) noexcept;

			/**
			 * @brief Stops a running server
			 */
			virtual void 																	Stop() noexcept;

		protected:
			Connection::Protocol m_protocol;												///< The protocol to use.
			Socket::Server m_socket;														///< The server socket.
			std::shared_ptr<Connection::Handler> m_conn_handler;							///< The handler to use.
			std::shared_ptr<Logger::Log> m_logger;											///< The logger.
			std::atomic<Connection::Status> m_status;										///< The connection status of the server.

			/**
			 * @brief The function to add a client to client's vector for store and ownership handling.
			 * @param client The client to add.
			 * @return The reference to the added client.
			 */
			Socket::Client& 																AddClient(Socket::Client&& client) noexcept;

			/**
			 * @brief The function to remove a client from client's vector.
			 * @param client The client to remove.
			 */
			void 																			RemoveClient(Socket::Client& client) noexcept;

			/**
			 * @brief The function to remove a client from client's vector in async way
			 * @param client The client to remove.
			 */
			void 																			RemoveClientAsync(Socket::Client& client) noexcept;

			/**
			 * @brief The function to disconnect a client (but not remove from vector).
			 * @param client The client to disconnect.
			 */
			void 																			DisconnectClient(Socket::Client& client) noexcept;

			/**
			 * @brief The function to authenticate a client
			 * @param client The client to authenticate.
			 */
			virtual bool 																	ClientAuthentication(Socket::Client& client) noexcept;

			/**
			 * @brief The function to preprocess a message.
			 * @param client The client to preprocess the message for.
			 * @param message The message to preprocess.
			 * @return The future of the preprocessed message.
			 */
			virtual StormByte::Expected<std::future<Util::Buffer>, ConnectionError>			PreProcessClientMessage(Socket::Client& client, std::future<Util::Buffer>& message) noexcept;

			/**
			 * @brief The function to preprocess a reply for example to encrypt it.
			 * @param client The client to preprocess the reply for.
			 * @param message The message to preprocess.
			 * @return The future of the preprocessed reply.
			 */
			virtual StormByte::Expected<std::future<Util::Buffer>, ConnectionError>			PreProcessClientReply(Socket::Client& client, std::future<Util::Buffer>& message) noexcept;

			/**
			 * @brief The function to process a client message and returns a reply.
			 * @param client The client to process the message for.
			 * @param message The message to process.
			 * @return The expected result of the operation.
			 */
			virtual StormByte::Expected<std::future<Util::Buffer>, ConnectionError>			ProcessClientMessage(Socket::Client& client, std::future<Util::Buffer>& message) noexcept = 0;

		private:
			std::thread m_accept_thread;													///< The thread for accepting clients.
			std::mutex m_clients_mutex;														///< The mutex to protect clients vector.
			std::vector<Socket::Client> m_clients;											///< The clients.
			std::mutex m_msg_threads_mutex;													///< The mutex to protect message threads.
			std::unordered_map<Connection::Handler::Type, std::thread> m_msg_threads;		///< The threads to handle client messages.

			/**
			 * @brief The function to disconnect all clients.
			 */
			void 																			DisconnectAllClients() noexcept;

			/**
			 * @brief The function to accept clients.
			 */
			void 																			AcceptClients() noexcept;

			/**
			 * @brief The function to start client communication.
			 * @param client The client to start communication for.
			 */
			void 																			StartClientCommunication(Socket::Client& client) noexcept;

			/**
			 * @brief The function to handle client messages.
			 * @param client The client to handle messages for.
			 */
			void 																			HandleClientMessages(Socket::Client& client) noexcept;

			/**
			 * @brief The function to send a reply to client.
			 * @param client The client to reply to.
			 * @param message The reply.
			 * @return The expected result of the operation.
			 */
			StormByte::Expected<void, ConnectionError> 										SendClientReply(Socket::Client& client, std::future<Util::Buffer>& message) noexcept;
	};
}