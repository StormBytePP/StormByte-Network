#pragma once

#include <StormByte/network/endpoint.hxx>
#include <string>
#include <memory>

/**
 * @namespace StormByte::Network
 * @brief StormByte networking subsystem.
 */
namespace StormByte::Network {
	namespace Connection {
		class Client;	///< Forward declaration
	}

	/**
	 * @class Client
	 * @brief Abstract application client endpoint.
	 *
	 * Derive and implement @ref InputPipeline() / @ref OutputPipeline() (and a
	 * concrete destructor in a .cxx). Use protected @ref Send() for
	 * request/response.
	 *
	 * @note **Inheritance-oriented.** Not for direct “generic” use without a subclass.
	 */
	class STORMBYTE_NETWORK_PUBLIC Client: private Endpoint {
		public:
			/**
			 * @param deserialize_packet_function Builds domain packets from wire data.
			 * @param logger Diagnostic logger.
			 */
			inline Client(const DeserializePacketFunction& deserialize_packet_function, std::shared_ptr<Logger::Log> logger) noexcept:
				Endpoint(deserialize_packet_function, logger),
				m_connection(nullptr) {}

			/**
			 * Copy constructor (deleted).
			 */
			Client(const Client& other) = delete;

			/**
			 * Move constructor.
			 */
			Client(Client&& other) noexcept = default;

			/**
			 * Destructor (out-of-line in .cxx).
			 */
			virtual ~Client() noexcept;

			/**
			 * Copy assignment (deleted).
			 */
			Client& operator=(const Client& other) = delete;

			/**
			 * Move assignment.
			 */
			Client& operator=(Client&& other) noexcept = default;

			/**
			 * Connects to a remote host.
			 * @param protocol Address family.
			 * @param address Hostname or IP.
			 * @param port Port number.
			 * @return true on success.
			 */
			bool Connect(const Connection::Protocol& protocol, const std::string& address, const unsigned short& port) override;

			/**
			 * Disconnects if connected.
			 */
			void Disconnect() noexcept override;

			/**
			 * @return Current connection status.
			 */
			Connection::Status Status() const noexcept override;

		protected:
			/**
			 * Sends @p packet and returns the response packet (or nullptr).
			 * @param packet Request packet.
			 * @return Response, or nullptr on error.
			 */
			PacketPointer Send(const Transport::Packet& packet) noexcept;

		private:
			std::shared_ptr<Connection::Client> m_connection;	///< Active connection
	};
}
