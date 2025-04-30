#pragma once

#include <StormByte/buffer/pipeline.hxx>
#include <StormByte/network/connection/handler.hxx>
#include <StormByte/network/packet.hxx>
#include <StormByte/network/typedefs.hxx>
#include <StormByte/logger.hxx>

#include <atomic>

/**
 * @namespace StormByte::Network
 * @brief Contains all the network-related classes and utilities.
 */
namespace StormByte::Network {
	/**
	 * @class EndPoint
	 * @brief Represents a network endpoint, serving as a base class for both clients and servers.
	 */
	class STORMBYTE_NETWORK_PUBLIC EndPoint {
		public:
			/**
			 * @brief Constructs an EndPoint with the specified protocol, handler, and logger.
			 * @param protocol The protocol to use (e.g., IPv4, IPv6).
			 * @param handler A shared pointer to the connection handler.
			 * @param logger A shared pointer to the logger instance.
			 */
			EndPoint(const Connection::Protocol& protocol, std::shared_ptr<Connection::Handler> handler, std::shared_ptr<Logger> logger) noexcept;

			/**
			 * @brief Deleted copy constructor to prevent copying.
			 */
			EndPoint(const EndPoint& other) 									= delete;

			/**
			 * @brief Defaulted move constructor.
			 */
			EndPoint(EndPoint&& other) noexcept 								= default;

			/**
			 * @brief Virtual destructor for proper cleanup in derived classes.
			 */
			virtual ~EndPoint() noexcept;

			/**
			 * @brief Deleted copy assignment operator to prevent copying.
			 */
			EndPoint& operator=(const EndPoint& other) 							= delete;

			/**
			 * @brief Defaulted move assignment operator.
			 */
			EndPoint& operator=(EndPoint&& other) noexcept 						= default;

			/**
			 * @brief Starts the endpoint (client or server) and connects/binds to the specified host and port.
			 * @param host The hostname or IP address to connect to.
			 * @param port The port number to connect to.
			 * @return An expected void or error.
			 */
			virtual ExpectedVoid 												Connect(const std::string& host, const unsigned short& port) noexcept = 0;

			/**
			 * @brief Stops the endpoint (client or server) and disconnects.
			 */
			virtual void 														Disconnect() noexcept;

			/**
			 * @brief Retrieves the protocol used by the endpoint.
			 * @return A constant reference to the protocol.
			 */
			const Connection::Protocol& 										Protocol() const noexcept;

			/**
			 * @brief Retrieves the current connection status of the endpoint.
			 * @return The current connection status.
			 */
			Connection::Status 													Status() const noexcept;

		protected:
			Connection::Protocol m_protocol;									///< The protocol used by the endpoint (e.g., IPv4, IPv6).
			std::shared_ptr<Connection::Handler> m_handler;						///< Shared pointer to the connection handler.
			std::shared_ptr<Logger> m_logger;									///< Shared pointer to the logger instance.
			std::atomic<Connection::Status> m_status;							///< The current connection status of the endpoint.
			Socket::Socket* m_socket;											///< Unique pointer to the socket instance.
			Buffer::Pipeline m_input_pipeline, m_output_pipeline;				///< The input/output pipeline for processing data.

			/**
			 * @brief Sets up the connection handler for the endpoint.
			 * @param client A reference to the client socket.
			 * @return An expected void or error.
			 */
			virtual ExpectedVoid 												Handshake(Socket::Client& client) noexcept;

			/**
			 * @brief Authenticates the client connection.
			 * @param client A reference to the client socket.
			 * @return An expected void or error.
			 */
			virtual ExpectedVoid 												Authenticate(Socket::Client& client) noexcept;

			/**
			 * @brief Receives packet from the client.
			 * @param client A reference to the client socket.
			 * @param pif The packet instance function to create a packet.
			 * @return An expected packet or error.
			 */
			ExpectedPacket 														Receive(Socket::Client& client) noexcept;

			/**
			 * @brief Sends a packet to the client.
			 * @param client A reference to the client socket.
			 * @param packet The packet to send.
			 * @return An expected void or error.
			 */
			ExpectedVoid 														Send(Socket::Client& client, const Packet& packet) noexcept;
	};
}