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
	class STORMBYTE_NETWORK_PUBLIC Server: private EndPoint {
		public:
			/**
			 * @brief Constructor.
			 * @param protocol The protocol to use.
			 * @param codec The codec to use.
			 * @param timeout The timeout to use.
			 * @param logger The logger to use.
			 */
			Server(const enum Protocol& protocol, const Codec& codec, const unsigned short& timeout, const Logger::ThreadedLog& logger) noexcept;

			/**
			 * @brief Copy constructor (deleted).
			 * @param other The other instance.
			 */
			Server(const Server& other) 											= delete;

			/**
			 * @brief Move constructor.
			 * @param other The other instance.
			 */
			Server(Server&& other) noexcept											= default;

			/**
			 * @brief Destructor.
			 */
			virtual ~Server() noexcept override;

			/**
			 * @brief Copy assignment operator (deleted).
			 * @param other The other instance.
			 * @return The new instance.
			 */
			Server& operator=(const Server& other) 									= delete;

			/**
			 * @brief Move assignment operator.
			 * @param other The other instance.
			 * @return The new instance.
			 */
			Server& operator=(Server&& other) noexcept 								= default;

			/**
			 * @brief Starts the server.
			 * @param host The host to use.
			 * @param port The port to use.
			 * @return True if the server started successfully, false otherwise.
			 */
			ExpectedVoid 															Connect(const std::string& host, const unsigned short& port) noexcept override;

			/**
			 * @brief Stops a running server disconnecting all clients
			 */
			void 																	Disconnect() noexcept override;

		protected:
			/**
			 * @brief Access the server logger for diagnostics in derived classes.
			 */
			inline Logger::ThreadedLog& Log() noexcept { return m_logger; }
			/**
			 * @brief The function to process a client packet and returns a reply.
			 * 
			 * This function is called when a client sends a packet to the server
			 * so the derived class can process the packet and optionally send a reply.
			 * If the function returns an unexpected it will be logged and the client will be disconnected.
			 * @param client_uuid The UUID of the client.
			 * @param packet The packet to process.
			 * @return The expected result of the operation.
			 */
			virtual Expected<void, PacketError>										ProcessClientPacket(const std::string& client_uuid, const Packet& packet) noexcept = 0;

			/**
			 * @brief Create a new input pipeline for a connected client.
			 * @param client_uuid The UUID of the connected client.
			 * @return The created input pipeline.
			 */
			virtual Buffer::Pipeline												CreateClientInputPipeline(const std::string& client_uuid) noexcept;

			/**
			 * @brief Create a new output pipeline for a connected client.
			 * @param client_uuid The UUID of the connected client.
			 * @return The created output pipeline.
			 */
			virtual Buffer::Pipeline												CreateClientOutputPipeline(const std::string& client_uuid) noexcept;

			/**
			 * @brief Sends a packet to a client identified by UUID.
			 * @param client_uuid The UUID of the client to send the packet to.
			 * @param packet The packet to send.
			 * @return The expected result of the operation.
			 */
			inline ExpectedVoid 													Send(const std::string& client_uuid, const Packet& packet) noexcept {
				return EndPoint::Send(client_uuid, packet);
			}

			/**
			 * @brief Disconnects a client identified by UUID.
			 * @param client_uuid The UUID of the client to disconnect.
			 */
			void 																	DisconnectClient(const std::string& client_uuid) noexcept;

		private:
			std::thread m_accept_thread;											///< The thread for accepting clients.
			std::mutex m_clients_mutex;												///< The mutex to protect clients vector.
			std::mutex m_msg_threads_mutex;											///< The mutex to protect message threads.
			std::unordered_map<std::string, std::thread> m_handle_msg_threads;		///< The threads to handle client messages.

			/**
			 * @brief Gets the socket by UUID thread safely.
			 * @param uuid The UUID of the socket.
			 * @return The socket.
			 */
			std::shared_ptr<Socket::Socket> 										GetSocketByUUID(const std::string& uuid) noexcept override;

			/**
			 * @brief Gets the in pipeline by UUID thread safely.
			 * @param uuid The UUID of the pipeline.
			 * @return The in pipeline.
			 */
			std::shared_ptr<Buffer::Pipeline> 										GetInPipelineByUUID(const std::string& uuid) noexcept override;

			/**
			 * @brief Gets the out pipeline by UUID thread safely.
			 * @param uuid The UUID of the pipeline.
			 * @return The out pipeline.
			 */
			std::shared_ptr<Buffer::Pipeline> 										GetOutPipelineByUUID(const std::string& uuid) noexcept override;

			/**
			 * @brief The function to accept clients.
			 */
			void 																	AcceptClients() noexcept;

			/**
			 * @brief The function to handle client communication.
			 * @param client_uuid The UUID of the client.
			 */
			void 																	HandleClientCommunication(const std::string& client_uuid) noexcept;
	};
}