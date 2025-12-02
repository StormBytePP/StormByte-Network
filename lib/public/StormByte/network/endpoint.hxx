#pragma once

#include <StormByte/logger/threaded_log.hxx>
#include <StormByte/network/codec.hxx>
#include <StormByte/network/packet.hxx>
#include <StormByte/network/typedefs.hxx>

#include <atomic>
#include <unordered_map>
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
			std::string m_self_uuid;											///< The UUID associated with this endpoint.
			enum Protocol m_protocol;											///< The protocol used by the endpoint (e.g., IPv4, IPv6).
			std::shared_ptr<Codec> m_codec;										///< The codec instance used for encoding/decoding packets.
			const unsigned short m_timeout;										///< The read timeout in seconds.
			Logger::ThreadedLog m_logger;										///< The logger instance.
			std::atomic<Connection::Status> m_status;							///< The current connection status of the endpoint.
			SocketUUIDPMap m_client_pmap; 										///< Map of all sockets including self.
			PipelineUUIDPMap m_in_pipeline_pmap;								///< Map of in pipelines associated with clients.
			PipelineUUIDPMap m_out_pipeline_pmap;								///< Map of out pipelines associated with clients.

			/**
			 * @brief Receives a packet from a client identified by UUID.
			 * @param client_uuid The UUID of the client to receive the packet from.
			 * @return An expected packet or error.
			 */
			ExpectedPacket														Receive(const std::string& client_uuid) noexcept;

			/**
			 * @brief Sends a packet to a client identified by UUID.
			 * @param client_uuid The UUID of the client to send the packet to.
			 * @param packet The packet to send.
			 * @return An expected void or error.
			 */
			ExpectedVoid 														Send(const std::string& client_uuid, const Packet& packet) noexcept;

			/**
			 * @brief Retrieves a socket by its UUID.
			 * @param uuid The UUID of the socket to retrieve.
			 * @return A pointer to the socket, or nullptr if not found.
			 */
			virtual Socket::Socket*												GetSocketByUUID(const std::string& uuid) noexcept;

			/**
			 * @brief Retrieves the input pipeline associated with a UUID.
			 * @param uuid The UUID of the pipeline to retrieve.
			 * @return A pointer to the input pipeline, or nullptr if not found.
			 */
			virtual Buffer::Pipeline*											GetInPipelineByUUID(const std::string& uuid) noexcept;

			/**
			 * @brief Retrieves the output pipeline associated with a UUID.
			 * @param uuid The UUID of the pipeline to retrieve.
			 * @return A pointer to the output pipeline, or nullptr if not found.
			 */
			virtual Buffer::Pipeline*											GetOutPipelineByUUID(const std::string& uuid) noexcept;
	};
}