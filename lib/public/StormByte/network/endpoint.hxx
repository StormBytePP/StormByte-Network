#pragma once

#include <StormByte/buffer/pipeline.hxx>
#include <StormByte/logger/threaded_log.hxx>
#include <StormByte/network/typedefs.hxx>

/**
 * @namespace StormByte::Network
 * @brief Network-related classes and utilities used by the StormByte networking
 *        subsystem.
 */
namespace StormByte::Network {
	namespace Connection {
		class Client;                                        ///< Forward declaration
	}

	/**
	 * @class Endpoint
	 * @brief Abstract base for network endpoints (clients/servers).
	 *
	 * Provides common functionality shared by concrete endpoint implementations
	 * such as `Client` and `Server`. This class is exported from the library but
	 * is not intended to be instantiated directly; implementors should derive
	 * from `Endpoint` and override the protected hooks as required.
	 */
	class STORMBYTE_NETWORK_PUBLIC Endpoint {
		public:
			/**
			 * @brief Constructs an Endpoint.
			 *
			 * @param deserialize_packet_function Function used to deserialize incoming
			 *        transport packets into domain `Packet` objects.
			 * @param logger Logger instance used by the endpoint for diagnostic
			 *        messages (copied into the endpoint).
			 */
			Endpoint(const DeserializePacketFunction& deserialize_packet_function, std::shared_ptr<Logger::Log> logger) noexcept;

			/**
			 * @brief Copy constructor (deleted).
			 *
			 * Copying is disabled to avoid accidental sharing of connection state.
			 */
			Endpoint(const Endpoint& other)								= delete;

			/**
			 * @brief Move constructor.
			 *
			 * Move semantics are supported to allow transfer of ownership of
			 * non-copyable members.
			 */
			Endpoint(Endpoint&& other) noexcept							= default;

			/**
			 * @brief Virtual destructor.
			 *
			 * Declared pure to make `Endpoint` an abstract base. Concrete subclasses
			 * must provide an implementation for the destructor (even if defaulted)
			 * in their translation unit.
			 */
			virtual ~Endpoint() noexcept								= default;

			/**
			 * @brief Copy-assignment operator (deleted).
			 *
			 * Copy assignment is disabled for the same reasons as copy construction.
			 */
			Endpoint& operator=(const Endpoint& other)					= delete;

			/**
			 * @brief Move-assignment operator.
			 *
			 * Provides move-assignment for transfer of ownership of internal
			 * resources.
			 */
			Endpoint& operator=(Endpoint&& other) noexcept				= default;

			/**
			 * @brief Establish a connection to a server.
			 *
			 * @param protocol The protocol to use for the connection.
			 * @param port The port to connect to.
			 * @return True if the connection was successful, false otherwise.
			 */
			virtual bool												Connect(const Connection::Protocol& protocol, const std::string& address, const unsigned short& port) = 0;

			/**
			 * @brief Disconnect from the server, if connected.
			 */
			virtual void 												Disconnect() noexcept = 0;

			/**
			 * @brief Gets the current connection status.
			 *
			 * @return The current connection status.
			 */
			virtual Connection::Status									Status() const noexcept = 0;

		protected:
			DeserializePacketFunction m_deserialize_packet_function;	///< The packet deserialization function.
			std::shared_ptr<Logger::Log> m_logger;						///< The logger instance.

			/**
			 * @brief Create a `Connection::Client` wrapper for an accepted socket.
			 *
			 * @param socket Shared pointer to the underlying client socket.
			 * @return Shared pointer to the created `Connection::Client` instance.
			 */
			std::shared_ptr<Connection::Client>							CreateConnection(std::shared_ptr<Socket::Client> socket) noexcept;

			/**
			 * @brief Provide the input `Buffer::Pipeline` for incoming data.
			 *
			 * Override this hook in derived classes to return a custom pipeline that
			 * will be applied to inbound buffers.
			 *
			 * @return A `Buffer::Pipeline` instance for input processing.
			 */
			virtual Buffer::Pipeline									InputPipeline() const noexcept = 0;

			/**
			 * @brief Provide the output `Buffer::Pipeline` for outgoing data.
			 *
			 * Override this hook to return a pipeline that will be applied to buffers
			 * before they are transmitted.
			 *
			 * @return A `Buffer::Pipeline` instance for output processing.
			 */
			virtual Buffer::Pipeline									OutputPipeline() const noexcept = 0;

			/**
			 * @brief Send a transport packet over the specified connection and
			 * obtains the response.
			 *
			 * The packet is prepared (e.g. serialized/enqueued) and handed to the
			 * transport for transmission. Implementations should return a pointer to
			 * the packet representation used by the transport, or `nullptr` on
			 * failure.
			 *
			 * @param client_connection Connection to use for sending the packet.
			 * @param packet The transport-level packet to send.
			 * @return A `PacketPointer` to the enqueued/sent packet, or `nullptr`
			 *         if sending failed.
			 */
			PacketPointer												Send(std::shared_ptr<Connection::Client> client_connection, const Transport::Packet& packet) noexcept;

			/**
			 * @brief Send a transport packet as a reply over the specified connection.
			 *
			 * The packet is prepared (e.g. serialized/enqueued) and handed to the
			 * transport for transmission. This function does not expect a response
			 * packet.
			 *
			 * @param client_connection Connection to use for sending the packet.
			 * @param packet The transport-level packet to send.
			 * @return True if the packet was sent successfully, false otherwise.
			 */
			bool 														Reply(std::shared_ptr<Connection::Client> client_connection, const Transport::Packet& packet) noexcept;

		private:
			/**
			 * @brief Internal helper to send a packet over a connection.
			 *
			 * @param client_connection Connection to use for sending the packet.
			 * @param packet The transport-level packet to send.
			 * @return True if the packet was sent successfully, false otherwise.
			 */
			bool 														SendPacket(std::shared_ptr<Connection::Client> client_connection, const Transport::Packet& packet) noexcept;
	};
}