#pragma once

#include <StormByte/network/endpoint.hxx>

#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_map>

/**
 * @namespace StormByte::Network
 * @brief StormByte networking subsystem.
 */
namespace StormByte::Network {
	namespace Connection {
		class Client;	///< Forward declaration
	}

	namespace Socket {
		class Server;	///< Forward declaration
	}

	/**
	 * @class Server
	 * @brief Abstract application server endpoint.
	 *
	 * Manages listen socket, accept loop and per-client worker threads.
	 * Implement @ref ProcessClientPacket() for application logic; override
	 * pipelines as needed.
	 *
	 * @note **Inheritance-oriented.** Subclass required.
	 */
	class STORMBYTE_NETWORK_PUBLIC Server: private Endpoint {
		public:
			/**
			 * @param deserialize_packet_function Builds domain packets from wire data.
			 * @param logger Diagnostic logger.
			 */
			Server(const DeserializePacketFunction& deserialize_packet_function, std::shared_ptr<Logger::Log> logger) noexcept;

			/**
			 * Copy constructor (deleted).
			 */
			Server(const Server& other) = delete;

			/**
			 * Move constructor.
			 */
			Server(Server&& other) noexcept = default;

			/**
			 * Destructor (joins threads, disconnects clients).
			 */
			virtual ~Server() noexcept;

			/**
			 * Copy assignment (deleted).
			 */
			Server& operator=(const Server& other) = delete;

			/**
			 * Move assignment.
			 */
			Server& operator=(Server&& other) noexcept = default;

			/**
			 * Binds, listens and starts the accept thread.
			 * @param protocol Address family.
			 * @param address Bind address.
			 * @param port Port number.
			 * @return true on success.
			 */
			bool Connect(const Connection::Protocol& protocol, const std::string& address, const unsigned short& port) override;

			/**
			 * Stops accept loop, disconnects clients and closes the listener.
			 */
			void Disconnect() noexcept override;

			/**
			 * @return Listener / server status.
			 */
			inline Connection::Status Status() const noexcept override {
				return m_status.load();
			}

		protected:
			/**
			 * Disconnects a client by UUID.
			 * @param uuid Client UUID.
			 */
			void DisconnectClient(const std::string& uuid) noexcept;

		private:
			std::unique_ptr<Socket::Server> m_socket_server;											///< Listen socket
			std::atomic<Connection::Status> m_status;												///< Server status
			std::thread m_accept_thread;															///< Accept loop thread
			std::unordered_map<std::string, std::shared_ptr<Connection::Client>> m_clients;		///< Active clients
			std::unordered_map<std::string, std::thread> m_handle_msg_threads;						///< Per-client workers
			std::mutex m_mutex;																		///< Protects client maps

			/**
			 * Accept-loop thread body.
			 */
			void AcceptClients() noexcept;

			/**
			 * Per-client communication thread body.
			 * @param client_uuid Client UUID.
			 */
			void HandleClientCommunication(const std::string& client_uuid) noexcept;

			/**
			 * Application packet handler.
			 * @param client_uuid Sender UUID.
			 * @param packet Received packet.
			 * @return Response packet, or nullptr on error / no reply.
			 */
			virtual PacketPointer ProcessClientPacket(const std::string& client_uuid, PacketPointer packet) noexcept = 0;
	};
}
