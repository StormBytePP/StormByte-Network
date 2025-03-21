#pragma once

#include <StormByte/expected.hxx>
#include <StormByte/network/connection/handler.hxx>
#include <StormByte/network/connection/info.hxx>
#include <StormByte/network/connection/status.hxx>
#include <StormByte/network/exception.hxx>

#include <memory>

/**
 * @namespace Socket
 * @brief The namespace containing all the socket related classes.
 */
namespace StormByte::Network::Socket {
	/**
	 * @class Socket
	 * @brief The class representing a socket.
	 */
	class STORMBYTE_NETWORK_PUBLIC Socket {
		public:
			/**
			 * @brief The constructor of the Socket class.
			 * @param protocol The protocol of the socket.
			 * @param handler The handler of the socket.
			 */
			Socket(const Connection::Protocol& protocol, std::shared_ptr<const Connection::Handler> handler);

			/**
			 * @brief The copy constructor of the Socket class.
			 * @param other The other socket to copy.
			 */
			Socket(const Socket& other) 									= delete;

			/**
			 * @brief The move constructor of the Socket class.
			 * @param other The other socket to move.
			 */
			Socket(Socket&& other) noexcept;

			/**
			 * @brief The destructor of the Socket class.
			 */
			virtual ~Socket() noexcept										= 0;

			/**
			 * @brief The assignment operator of the Socket class.
			 * @param other The other socket to assign.
			 * @return The reference to the assigned socket.
			 */
			Socket& operator=(const Socket& other) 							= delete;

			/**
			 * @brief The move assignment operator of the Socket class.
			 * @param other The other socket to assign.
			 * @return The reference to the assigned socket.
			 */
			Socket& operator=(Socket&& other) noexcept;

			/**
			 * @brief The function to disconnect from a server.
			 */
			void 															Disconnect() noexcept;

			/**
			 * @brief Gets the socket status
			 * @return The status of the socket.
			 */
			constexpr const Connection::Status& 							Status() const noexcept {
				return m_status;
			}

			/**
			 * @brief Gets the MTU of the socket.
			 * @return The MTU of the socket.
			 */
			constexpr const unsigned long&									MTU() const noexcept {
				return m_mtu;
			}

		protected:
			Connection::Protocol m_protocol;								///< The protocol of the socket.
			Connection::Status m_status;									///< The status of the socket.
			std::unique_ptr<const Connection::Handler::Type> m_handle;		///< The handle of the socket.
			std::shared_ptr<const Connection::Handler> m_conn_handler;		///< The handler of the socket.
			std::unique_ptr<Connection::Info> m_conn_info;					///< The socket address.
			unsigned long m_mtu;											///< The maximum transmission unit.

			/**
			 * @brief The function to create a socket.
			 * @return The expected result of the operation.
			 */
			StormByte::Expected<Connection::Handler::Type, ConnectionError>	CreateSocket() noexcept;

			/**
			 * @brief The function to get the handle of the socket.
			 * @return The handle of the socket.
			 */
			void															InitializeMTU() noexcept;

		private:
			constexpr static const unsigned short DEFAULT_MTU = 1500;		///< The default maximum transmission unit.
	};
}