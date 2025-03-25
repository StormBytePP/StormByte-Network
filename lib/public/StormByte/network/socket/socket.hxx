#pragma once

#include <StormByte/expected.hxx>
#include <StormByte/logger.hxx>
#include <StormByte/network/connection/handler.hxx>
#include <StormByte/network/connection/info.hxx>
#include <StormByte/network/connection/protocol.hxx>
#include <StormByte/network/connection/rw.hxx>
#include <StormByte/network/connection/status.hxx>
#include <StormByte/network/exception.hxx>
#include <StormByte/network/typedefs.hxx>

/**
 * @namespace Socket
 * @brief The namespace containing all the socket related classes.
 */
namespace StormByte::Network::Socket {
	class Server;
	/**
	 * @class Socket
	 * @brief The class representing a non blocking socket.
	 */
	class STORMBYTE_NETWORK_PUBLIC Socket {
		friend class Server;
		public:
			/**
			 * @brief The copy constructor of the Socket class.
			 * @param other The other socket to copy.
			 */
			Socket(const Socket& other) 										= delete;

			/**
			 * @brief The move constructor of the Socket class.
			 * @param other The other socket to move.
			 */
			Socket(Socket&& other) noexcept;

			/**
			 * @brief The destructor of the Socket class.
			 */
			virtual ~Socket() noexcept;

			/**
			 * @brief The assignment operator of the Socket class.
			 * @param other The other socket to assign.
			 * @return The reference to the assigned socket.
			 */
			Socket& operator=(const Socket& other) 								= delete;

			/**
			 * @brief The move assignment operator of the Socket class.
			 * @param other The other socket to assign.
			 * @return The reference to the assigned socket.
			 */
			Socket& operator=(Socket&& other) noexcept;

			/**
			 * @brief The function to disconnect from a server.
			 */
			void 																Disconnect() noexcept;

			/**
			 * @brief Gets the socket status
			 * @return The status of the socket.
			 */
			constexpr const Connection::Status& 								Status() const noexcept {
				return m_status;
			}

			/**
			 * @brief Gets the MTU of the socket.
			 * @return The MTU of the socket.
			 */
			constexpr const unsigned long&										MTU() const noexcept {
				return m_mtu;
			}

			/**
			 * @brief Gets the handle of the socket (don't check if it's valid!)
			 * @return The handle of the socket.
			 */
			inline const Connection::Handler::Type&								Handle() const noexcept {
				return *m_handle;
			}

			/**
			 * @brief The function to wait for data to be available.
			 * @param usecs The number of microseconds to wait for data. (0 for no timeout)
			 * @return The expected result of the operation.
			 */
			ExpectedReadResult 													WaitForData(const long long& usecs = 0) noexcept;

		protected:
			Connection::Protocol m_protocol;									///< The protocol of the socket.
			Connection::Status m_status;										///< The status of the socket.
			std::unique_ptr<const Connection::Handler::Type> m_handle;			///< The handle of the socket.
			std::shared_ptr<const Connection::Handler> m_conn_handler;			///< The handler of the socket.
			std::unique_ptr<Connection::Info> m_conn_info;						///< The socket address.
			unsigned long m_mtu;												///< The maximum transmission unit.
			std::shared_ptr<Logger> m_logger;								///< The logger.

			/**
			 * @brief The constructor of the Socket class.
			 * @param protocol The protocol of the socket.
			 * @param handler The handler of the socket.
			 * @param logger The logger to use.
			 */
			Socket(const Connection::Protocol& protocol, std::shared_ptr<const Connection::Handler> handler, std::shared_ptr<Logger> logger) noexcept;

			/**
			 * @brief The function to create a socket.
			 * @return The expected result of the operation.
			 */
			ExpectedHandlerType													CreateSocket() noexcept;

			/**
			 * @brief Function to initialize the socket after connecting (or accepting a connection).
			 */
			void																InitializeAfterConnect() noexcept;

		private:
			constexpr static const unsigned short DEFAULT_MTU = 1500;			///< The default maximum transmission unit.

			/**
			 * @brief The function to get the MTU of the socket.
			 * @return The MTU of the socket.
			 */
			int																	GetMTU() const noexcept;

			/**
			 * @brief The function to set the read mode of the socket.
			 */
			void																SetNonBlocking() noexcept;
	};
}