#pragma once

#include <StormByte/expected.hxx>
#include <StormByte/network/exception.hxx>
#include <StormByte/network/connection/protocol.hxx>

#ifdef WINDOWS
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

#include <memory>
#include <string>

/**
 * @namespace Connection
 * @brief The namespace containing connection items
 */
namespace StormByte::Network::Connection {
	constexpr const unsigned short DEFAULT_MTU = 1500;									///< The default maximum transmission unit.

	/**
	 * @class Info
	 * @brief The class representing the connection information.
	 */
	class STORMBYTE_NETWORK_PRIVATE Info {
		public:
			/**
			 * @brief The copy constructor of the Info class.
			 * @param other The other Info to copy.
			 */
			Info(const Info& other) noexcept											= delete;

			/**
			 * @brief The move constructor of the Info class.
			 * @param other The other Info to move.
			 */
			Info(Info&& other) noexcept													= default;

			/**
			 * @brief The destructor of the Info class.
			 */
			~Info() noexcept															= default;

			/**
			 * @brief The assignment operator of the Info class.
			 * @param other The other Info to assign.
			 * @return The reference to the assigned Info.
			 */
			Info& operator=(const Info& other) noexcept									= delete;

			/**
			 * @brief The move assignment operator of the Info class.
			 * @param other The other Info to assign.
			 * @return The reference to the assigned Info.
			 */
			Info& operator=(Info&& other) noexcept										= default;

			/**
			 * @brief The function to create a connection info from a host.
			 * @param hostname The hostname of the connection.
			 * @param port The port of the connection.
			 * @param protocol The protocol of the connection.
			 * @param handler The connection handler.
			 * @return The expected connection info or error.
			 */
			static StormByte::Expected<Info, Exception>									FromHost(const std::string& hostname, const unsigned short& port, const Protocol& protocol) noexcept;
			
			/**
			 * @brief The function to create a connection info from a socket address.
			 * @param sockaddr The socket address.
			 * @return The expected connection info or error.
			 */
			static StormByte::Expected<Info, Exception>									FromSockAddr(std::shared_ptr<sockaddr> sockaddr) noexcept;

			/**
			 * @brief The function to get the IP address.
			 * @return The IP address.
			 */
			constexpr const std::string&												IP() const noexcept {
				return m_ip;
			}

			/**
			 * @brief The function to get the port.
			 * @return The port.
			 */
			constexpr const unsigned short&												Port() const noexcept {
				return m_port;
			}

			/**
			 * @brief The function to get the socket address
			 * @return The socket address.
			 */
			inline std::shared_ptr<const sockaddr>										SockAddr() const noexcept {
				return m_sock_addr;
			}

		private:
			std::shared_ptr<sockaddr> m_sock_addr;										///< The socket address.
			unsigned int m_mtu;															///< The maximum transmission unit.
			std::string m_ip;															///< The hostname.
			unsigned short m_port;														///< The port.

			/**
			 * @brief The constructor of the Info class.
			 * @param sock_addr The socket address.
			 */
			Info(std::shared_ptr<sockaddr> sock_addr) noexcept;

			/**
			 * @brief Resolves hostname and gets the connection info
			 * @param hostname The hostname to resolve.
			 * @param port The port to connect to.
			 * @param protocol The protocol to use.
			 * @param handler The connection handler.
			 * @return The expected connection info or error.
			 */
			static StormByte::Expected<std::shared_ptr<sockaddr>, Exception> 			ResolveHostname(const std::string& hostname, const unsigned short& port, const Protocol& protocol) noexcept;

			/**
			 * @brief Initializes from a socket address.
			 * @param sock_addr The socket address.
			 * @param handler The connection handler.
			 */
			void 																		Initialize(std::shared_ptr<sockaddr> sock_addr) noexcept;
	};
}