#pragma once

#include <StormByte/network/packet.hxx>
#include <StormByte/network/socket/socket.hxx>

namespace StormByte::Network { class Server; } // Forward declaration

/**
 * @namespace Socket
 * @brief The namespace containing all the socket related classes.
 */
namespace StormByte::Network::Socket {
	class Server; // Forward declaration
	/**
	 * @class Client
	 * @brief The class representing a client socket.
	 */
	class STORMBYTE_NETWORK_PUBLIC Client final: public Socket {
		friend class Server;
		friend class Network::Server;
		public:
			/**
			 * @brief The constructor of the Client class.
			 * @param protocol The protocol of the socket.
			 * @param handler The handler of the socket.
			 */
			Client(const Connection::Protocol& protocol, std::shared_ptr<const Connection::Handler> handler);

			/**
			 * @brief The copy constructor of the Client class.
			 * @param other The other socket to copy.
			 */
			Client(const Client& other) 						= delete;

			/**
			 * @brief The move constructor of the Client class.
			 * @param other The other socket to move.
			 */
			Client(Client&& other) noexcept						= default;

			/**
			 * @brief The destructor of the Client class.
			 */
			~Client() noexcept override							= default;

			/**
			 * @brief The assignment operator of the Client class.
			 * @param other The other socket to assign.
			 * @return The reference to the assigned socket.
			 */
			Client& operator=(const Client& other) 				= delete;

			/**
			 * @brief The move assignment operator of the Client class.
			 * @param other The other socket to assign.
			 * @return The reference to the assigned socket.
			 */
			Client& operator=(Client&& other) noexcept			= default;

			/**
			 * @brief The function to connect to a server.
			 * @param hostname The hostname of the server.
			 * @param port The port of the server.
			 * @return The expected result of the operation.
			 */
			StormByte::Expected<void, ConnectionError>			Connect(const std::string& hostname, const unsigned short& port) noexcept;

			/**
			 * @brief Function to receive data from the socket.
			 * @return The expected result of the operation.
			 */
			StormByte::Expected<Packet, ConnectionClosed>		Receive() noexcept;

			/**
			 * @brief Function to send data to the socket.
			 * @param buffer The buffer to send.
			 * @return The expected result of the operation.
			 */
			StormByte::Expected<void, ConnectionError>			Send(const Packet& packet) noexcept;

			/**
			 * @brief The function to set the socket to non blocking reading mode.
			 * @param mode The read mode to set.
			 */
			void												ReadMode(const Connection::ReadMode& mode) noexcept;

		private:
			// Those two functions are needed because MSVC does not inherit friendship
			constexpr std::unique_ptr<const Connection::Handler::Type>&		Handle() noexcept {
				return m_handle;
			}
			constexpr Connection::Status&									Status() noexcept {
				return m_status;
			}
			constexpr std::unique_ptr<Connection::Info>&					ConnInfo() noexcept {
				return m_conn_info;
			}
	};
}