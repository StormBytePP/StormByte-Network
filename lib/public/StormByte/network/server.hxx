#pragma once

#include <StormByte/network/endpoint.hxx>
#include <StormByte/network/typedefs.hxx>

#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace StormByte::Network {
	/**
	 * @class Server
	 * @brief A server class.
	 */
	class STORMBYTE_NETWORK_PUBLIC Server: public EndPoint {
		public:
			/**
			 * @brief Constructor.
			 * @param protocol The protocol to use.
			 * @param handler The handler to use.
			 */
			Server(Connection::Protocol protocol, std::shared_ptr<Connection::Handler> handler, Logger::ThreadedLog logger) noexcept;

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
			virtual ~Server() noexcept override;

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
			virtual ExpectedVoid 															Connect(const std::string& host, const unsigned short& port) noexcept override;

			/**
			 * @brief Stops a running server
			 */
			virtual void 																	Disconnect() noexcept override;

		protected:
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
			 * @brief The function to process a client packet and returns a reply.
			 * 
			 * This function is called when a client sends a packet to the server
			 * so the derived class can process the packet and optionally send a reply.
			 * If the function returns an unexpected it will be logged and the client will be disconnected.
			 * @param client The client to process the message for.
			 * @param message The message to process.
			 * @return The expected result of the operation.
			 */
			virtual Expected<void, PacketError>												ProcessClientPacket(Socket::Client& client, const Packet& packet) noexcept = 0;

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
	};
}