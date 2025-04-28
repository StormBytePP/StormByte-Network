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
	class STORMBYTE_NETWORK_PUBLIC Client: public EndPoint {
		public:
			/**
			 * @brief The constructor of the Client class.
			 * @param address The address of the server.
			 * @param handler The handler of the server.
			 * @param pf The function to create a packet instance from opcode
			 */
			Client(const Connection::Protocol& protocol, std::shared_ptr<Connection::Handler> handler, std::shared_ptr<Logger> logger, const PacketInstanceFunction& pf) noexcept;

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
			virtual ~Client() noexcept override;

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
			StormByte::Expected<void, ConnectionError>						Connect(const std::string& hostname, const unsigned short& port, const Connection::Protocol& protocol) noexcept;

			/**
			 * @brief The function to disconnect the server.
			 */
			void 															Disconnect() noexcept;

		protected:
			/**
			 * @brief The function to send data to the server.
			 * @param packet The packet to send.
			 * @return The expected void or error.
			 */
			StormByte::Expected<void, ConnectionError>						Send(const Packet& packet) noexcept;

			/**
			 * @brief The function to receive data from the server.
			 * @return The expected buffer or error.
			 */
			ExpectedPacket													Receive() noexcept;

		private:
			PacketInstanceFunction m_packet_instance_function;				///< The function to create a packet instance from opcode.
	};
}