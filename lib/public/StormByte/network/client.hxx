#pragma once

#include <StormByte/network/endpoint.hxx>
#include <string>
#include <memory>

/**
 * @namespace StormByte::Network
 * @brief Network-related classes used by the StormByte networking subsystem.
 */
namespace StormByte::Network {
	namespace Connection {
		class Client;                                        ///< Forward declaration
	}

	/**
	 * @class Client
	 * @brief Abstract base class for application-specific clients.
	 *
	 * @details
	 * Derive from `Client` to implement your application's client-side logic.
	 * The modern API expects a deserializer function (see `DeserializePacketFunction`)
	 * and a `Logger::ThreadedLog` instance to be provided to the `Client`
	 * constructor. Subclasses typically implement lightweight, domain-specific
	 * helper methods that call the protected `Send()` helper to perform
	 * synchronous request/response flows.
	 *
	 * Key extension points:
	 * - Override `InputPipeline()` and `OutputPipeline()` to return any
	 *   `Buffer::Pipeline` stages required by your application (compression,
	 *   encryption, framing, etc.). Return an empty pipeline when no processing
	 *   is required (the common case in tests/examples).
	 * - Implement a concrete (non-pure) destructor in a translation unit. The
	 *   base destructor is declared virtual/pure to enforce an out-of-line
	 *   definition in derived classes.
	 *
	 * The protected `Send()` helper sends a `Transport::Packet` to the server
	 * and returns a `PacketPointer` referencing the reply (or `nullptr` on
	 * failure). Use `Connect()` / `Disconnect()` to manage the connection.
	 */
	class STORMBYTE_NETWORK_PUBLIC Client: private Endpoint {
		public:
			/**
			 * @brief Constructs a `Client` instance.
			 *
			 * @param deserialize_packet_function Function used to deserialize incoming
			 *        transport packets into domain `Packet` objects.
			 * @param logger Logger instance used for diagnostic messages.
			 */
			inline Client(const DeserializePacketFunction& deserialize_packet_function, std::shared_ptr<Logger::Log> logger) noexcept:
				Endpoint(deserialize_packet_function, logger),
				m_connection(nullptr) {}

			/**
			 * @brief Copy constructor (deleted).
			 */
			Client(const Client& other)									= delete;

			/**
			 * @brief Move constructor.
			 */
			Client(Client&& other) noexcept								= default;

			/**
			 * @brief Virtual destructor (abstract).
			 *
			 * Kept pure to make `Client` abstract; derived classes must provide an
			 * implementation (even if defaulted) in a translation unit.
			 */
			virtual ~Client() noexcept;

			/**
			 * @brief Copy-assignment operator (deleted).
			 */
			Client& operator=(const Client& other)						= delete;

			/**
			 * @brief Move-assignment operator.
			 */
			Client& operator=(Client&& other) noexcept					= default;

			/**
			 * @brief Establish a connection to a server.
			 *
			 * @param protocol Protocol parameters to use for the connection.
			 * @param address Server address (hostname or IP string).
			 * @param port Server port number.
			 * @return `true` if the connection was established, `false` otherwise.
			 */
			bool														Connect(const Connection::Protocol& protocol, const std::string& address, const unsigned short& port) override;

			/**
			 * @brief Disconnect from the server, if connected.
			 */
			void														Disconnect() noexcept override;

			/**
			 * @brief Query the current connection status.
			 *
			 * @return Current `Connection::Status` representing the connection state.
			 */
			Connection::Status											Status() const noexcept override;

		protected:
			/**
			 * @brief Send a `Transport::Packet` to the connected server and receive a
			 *        reply.
			 *
			 * Subclasses should use this protected helper to perform request/response
			 * interactions with the server. The function returns a `PacketPointer`
			 * referencing the response packet, or `nullptr` if the send or receive
			 * operation failed.
			 *
			 * @param packet The transport-level packet to send.
			 * @return `PacketPointer` to the response packet, or `nullptr` on error.
			 */
			PacketPointer												Send(const Transport::Packet& packet) noexcept;

		private:
			std::shared_ptr<Connection::Client> m_connection;			///< The underlying connection.
	};
}