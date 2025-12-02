#pragma once

#include <StormByte/network/endpoint.hxx>
#include <StormByte/network/packet.hxx>

/**
 * @namespace Network
 * @brief The namespace containing all the network related classes.
 */
namespace StormByte::Network {
	/**
	 * @class Client
	 * @brief The class representing a client.
	 */
	class STORMBYTE_NETWORK_PUBLIC Client: private EndPoint {
		public:
			/**
			 * @brief The constructor of the Client class.
			 * @param protocol The protocol to use.
			 * @param codec The codec to use.
			 * @param timeout The timeout duration.
			 * @param logger The logger instance.
			 */
			Client(const enum Protocol& protocol, const Codec& codec, const unsigned short& timeout, const Logger::ThreadedLog& logger) noexcept;

			/**
			 * @brief The copy constructor of the Client class.
			 * @param other The other server to copy.
			 */
			Client(const Client& other)										= delete;

			/**
			 * @brief The move constructor of the Client class.
			 * @param other The other server to move.
			 */
			Client(Client&& other) noexcept									= default;

			/**
			 * @brief The destructor of the Client class.
			 */
			virtual ~Client() noexcept override								= default;

			/**
			 * @brief The assignment operator of the Client class.
			 * @param other The other server to assign.
			 * @return The reference to the assigned server.
			 */
			Client& operator=(const Client& other)							= delete;

			/**
			 * @brief The assignment operator of the Client class.
			 * @param other The other server to assign.
			 * @return The reference to the assigned server.
			 */
			Client& operator=(Client&& other) noexcept						= default;

			/**
			 * @brief The function to connect a server.
			 * @param hostname The hostname of the server.
			 * @param port The port of the server.
			 * @param protocol The protocol of the server.
			 * @return The expected void or error.
			 */
			ExpectedVoid													Connect(const std::string& hostname, const unsigned short& port) noexcept override;

			/**
			 * @brief The function to disconnect the client.
			 */
			void															Disconnect() noexcept override;

		protected:
			Buffer::Pipeline m_in_pipeline, m_out_pipeline;					///< Input and output pipelines for processing data.

			/**
			 * @brief The function to receive data from the server.
			 * @return The expected buffer or error.
			 */
			ExpectedPacket													Receive() noexcept;

			/**
			 * @brief The function to send data to the server.
			 * @param packet The packet to send.
			 * @return The expected void or error.
			 */
			ExpectedVoid													Send(const Packet& packet) noexcept;
	};
}