#pragma once

#include <StormByte/network/endpoint.hxx>

#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_map>

/**
 * @namespace StormByte::Network
 * @brief Network-related classes and utilities used by the StormByte networking
 *        subsystem.
 */
namespace StormByte::Network {
	namespace Connection {
		class Client;                                        ///< Forward declaration
	}

	namespace Socket {
		class Server;                                        ///< Forward declaration
	}

	/**
	 * @class Server
	 * @brief Abstract base for application-specific servers.
	 *
	 * @details
	 * Subclass `Server` to implement your server-side application logic. The
	 * constructor accepts a `DeserializePacketFunction` and a `Logger::ThreadedLog`
	 * instance; the deserializer converts opcode + payload bytes into concrete
	 * `Transport::Packet` instances used by the framework.
	 *
	 * Key extension points:
	 * - Implement `ProcessClientPacket()` to inspect incoming packets and return
	 *   a `PacketPointer` that will be sent back to the requesting client. If error
	 *   return `nullptr`.
	 * - Override `InputPipeline()` and `OutputPipeline()` to add buffer
	 *   preprocessing/postprocessing stages (compression, encryption, etc.).
	 * - Provide a concrete (out-of-line) destructor implementation in derived
	 *   classes (the base destructor is declared pure to force an out-of-line
	 *   override).
	 *
	 * Implementation notes:
	 * - The `Server` base manages listening, accepting, per-client threads and
	 *   the underlying socket server; application logic should reside in
	 *   `ProcessClientPacket()` or other overridable helpers.
	 */
	class STORMBYTE_NETWORK_PUBLIC Server: private Endpoint {
		public:
			/**
			 * @brief Constructs a `Server` instance.
			 *
			 * @param deserialize_packet_function Function used to deserialize incoming
			 *        transport packets into domain `Packet` objects.
			 * @param logger Logger instance used for diagnostic messages.
			 */
			Server(const DeserializePacketFunction& deserialize_packet_function, std::shared_ptr<Logger::Log> logger) noexcept;

			/**
			 * @brief Copy constructor (deleted).
			 */
			Server(const Server& other)														= delete;

			/**
			 * @brief Move constructor.
			 */
			Server(Server&& other) noexcept													= default;

			/**
			 * @brief Virtual destructor.
			 *
			 * Declared pure to make `Server` an abstract base. Concrete subclasses
			 * must provide an implementation for the destructor (even if defaulted)
			 * in their translation unit.
			 */
			virtual ~Server() noexcept;

			/**
			 * @brief Copy-assignment operator (deleted).
			 */
			Server& operator=(const Server& other)											= delete;

			/**
			 * @brief Move-assignment operator.
			 */
			Server& operator=(Server&& other) noexcept										= default;

			/**
			 * @brief Start the server and begin listening for connections.
			 *
			 * @param protocol The protocol to use for the server.
			 * @param address The address to bind to.
			 * @param port The port to listen on.
			 * @return True if the server started successfully, false otherwise.
			 */
			bool 																			Connect(const Connection::Protocol& protocol, const std::string& address, const unsigned short& port) override;

			/**
			 * @brief Stop the server and disconnect all clients.
			 */
			void																			Disconnect() noexcept override;

			/**
			 * @brief Gets the current server connection status.
			 *
			 * @return The current connection status.
			 */
			inline Connection::Status														Status() const noexcept override {
				return m_status.load();
			}

		protected:
			/**
			 * @brief Disconnects a client by UUID.
			 *
			 * @param client_uuid The UUID of the client to be disconnected.
			 */
			void 																			DisconnectClient(const std::string& uuid) noexcept;

		private:
			std::unique_ptr<Socket::Server>	m_socket_server;								///< The underlying socket server.
			std::atomic<Connection::Status> m_status;										///< The current server status.
			std::thread m_accept_thread;													///< The thread handling incoming connections.
			std::unordered_map<std::string, std::shared_ptr<Connection::Client>> m_clients;	///< The connected clients.
			std::unordered_map<std::string, std::thread> m_handle_msg_threads;				///< The threads handling client communication.
			std::mutex m_mutex;																///< Mutex for protecting client list.

			/**
			 * @brief Thread function to accept incoming client connections.
			 */
			void 																			AcceptClients() noexcept;

			/**
			 * @brief Thread function to handle communication with a connected client.
			 *
			 * @param client_uuid The UUID of the client to handle.
			 */
			void 																			HandleClientCommunication(const std::string& client_uuid) noexcept;
			
			/**
			 * @brief Process a received packet from a client.
			 *
			 * This is a pure virtual function that must be implemented by derived
			 * classes to handle application-specific packet processing logic.
			 *
			 * @param client_uuid The UUID of the client that sent the packet.
			 * @param packet The received packet to process.
			 * @return A `PacketPointer` to the response packet, or `nullptr` if no
			 *         response is needed.
			 */
			virtual PacketPointer															ProcessClientPacket(const std::string& client_uuid, PacketPointer packet) noexcept = 0;
	};
}