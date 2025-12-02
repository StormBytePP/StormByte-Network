#pragma once

#include <StormByte/logger/threaded_log.hxx>
#include <StormByte/network/codec.hxx>
#include <StormByte/network/packet.hxx>
#include <StormByte/network/typedefs.hxx>

#include <atomic>
#include <map>
#include <memory>

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
			 * @brief Constructs an EndPoint with the specified protocol and logger.
			 * @param protocol The protocol to use (e.g., IPv4, IPv6).
			 * @param codec The codec to use for encoding/decoding packets.
			 * @param timeout The read timeout in seconds.
			 * @param logger A shared pointer to the logger instance.
			 */
			EndPoint(const enum Protocol& protocol, const Codec& codec, const unsigned short& timeout, const Logger::ThreadedLog& logger) noexcept;

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
			const enum Protocol& 												Protocol() const noexcept;

			/**
			 * @brief Retrieves the current connection status of the endpoint.
			 * @return The current connection status.
			 */
			Connection::Status 													Status() const noexcept;

		protected:
			Socket::Socket* m_self_socket;										///< Pointer to the socket instance of this endpoint (can't use std::unique_ptr as we don't include the header as its private).
			enum Protocol m_protocol;											///< The protocol used by the endpoint (e.g., IPv4, IPv6).
			std::shared_ptr<Codec> m_codec;										///< The codec instance used for encoding/decoding packets.
			const unsigned short m_timeout;										///< The read timeout in seconds.
			Logger::ThreadedLog m_logger;										///< The logger instance.
			std::atomic<Connection::Status> m_status;							///< The current connection status of the endpoint.

			/**
			 * @brief Receives a packet from the specified socket.
			 * @param socket The socket to receive the packet from.
			 * @return A shared pointer to the received packet (nullptr if error).
			 */
			ExpectedPacket														Receive(Socket::Client& socket) noexcept;

			/**
			 * @brief Sends a packet through the specified socket.
			 * @param socket The socket to send the packet through.
			 * @param packet The packet to send.
			 * @return An expected void or error.
			 */
			ExpectedVoid 														Send(Socket::Client& socket, const Packet& packet) noexcept;
	};
}