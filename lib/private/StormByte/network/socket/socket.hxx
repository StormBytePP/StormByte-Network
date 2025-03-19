#pragma once

#include <StormByte/expected.hxx>
#include <StormByte/network/address.hxx>
#include <StormByte/network/exception.hxx>
#include <StormByte/network/handler.hxx>
#include <StormByte/network/status.hxx>

#ifdef WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

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
	class STORMBYTE_NETWORK_PRIVATE Socket {
		public:
			/**
			 * @brief The constructor of the Socket class.
			 * @param address The address to create the socket.
			 * @param handler The handler of the socket.
			 */
			Socket(const Address& address, std::shared_ptr<const Handler> handler);

			/**
			 * @brief The copy constructor of the Socket class.
			 * @param other The other socket to copy.
			 */
			Socket(const Socket& other) 						= delete;

			/**
			 * @brief The move constructor of the Socket class.
			 * @param other The other socket to move.
			 */
			Socket(Socket&& other) noexcept;

			/**
			 * @brief The destructor of the Socket class.
			 */
			virtual ~Socket() noexcept							= 0;

			/**
			 * @brief The assignment operator of the Socket class.
			 * @param other The other socket to assign.
			 * @return The reference to the assigned socket.
			 */
			Socket& operator=(const Socket& other) 				= delete;

			/**
			 * @brief The move assignment operator of the Socket class.
			 * @param other The other socket to assign.
			 * @return The reference to the assigned socket.
			 */
			Socket& operator=(Socket&& other) noexcept;

			/**
			 * @brief The function to listen for incoming connections.
			 * @return The expected result of the operation.
			 */
			StormByte::Expected<void, ConnectionError>			Listen() noexcept;

			/**
			 * @brief The function to disconnect from a server.
			 */
			void 												Disconnect() noexcept;

			/**
			 * @brief Gets the socket status
			 * @return The status of the socket.
			 */
			const Status& 										Status() const noexcept;

			/**
			 * @brief The function to get the handle of the socket.
			 * @return The handle of the socket.
			 */
			const std::unique_ptr<const Handler::Type>& 		Handle() const noexcept;										

		protected:
			Address m_address;									///< The address of the socket.
			Network::Status m_status;							///< The status of the socket.
			std::unique_ptr<const Handler::Type> m_handle;		///< The handle of the socket.
			#ifdef WINDOWS
			std::unique_ptr<WSADATA> m_wsaData;					///< The windows socket data.
			#endif
			std::shared_ptr<const Handler> m_handler;			///< The handler of the socket.
	};
}