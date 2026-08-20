#pragma once

#include <StormByte/expected.hxx>
#include <StormByte/network/connection/protocol.hxx>
#include <StormByte/network/exception.hxx>

#ifdef WINDOWS
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

#include <memory>
#include <string>

/**
 * @namespace Connection
 * @brief Connection helpers (handler, info, client wrapper).
 */
namespace StormByte::Network::Connection {
	constexpr const unsigned short DEFAULT_MTU = 1500;	///< Default MTU

	/**
	 * @class Info
	 * @brief Resolved peer address (IP, port, sockaddr).
	 */
	class STORMBYTE_NETWORK_PRIVATE Info {
		public:
			/**
			 * Copy constructor (deleted).
			 */
			Info(const Info& other) noexcept = delete;

			/**
			 * Move constructor.
			 */
			Info(Info&& other) noexcept = default;

			/**
			 * Destructor.
			 */
			~Info() noexcept = default;

			/**
			 * Copy assignment (deleted).
			 */
			Info& operator=(const Info& other) noexcept = delete;

			/**
			 * Move assignment.
			 */
			Info& operator=(Info&& other) noexcept = default;

			/**
			 * Resolves hostname and builds Info.
			 * @param hostname Host name.
			 * @param port Port.
			 * @param protocol Address family.
			 * @return Info or error.
			 */
			static StormByte::Expected<Info, Exception> FromHost(const std::string& hostname, const unsigned short& port, const Protocol& protocol) noexcept;

			/**
			 * Builds Info from an existing sockaddr.
			 * @param sockaddr Socket address.
			 * @return Info or error.
			 */
			static StormByte::Expected<Info, Exception> FromSockAddr(std::shared_ptr<sockaddr> sockaddr) noexcept;

			/**
			 * @return Resolved IP string.
			 */
			constexpr const std::string& IP() const noexcept {
				return m_ip;
			}

			/**
			 * @return Port number.
			 */
			constexpr const unsigned short& Port() const noexcept {
				return m_port;
			}

			/**
			 * @return Shared sockaddr.
			 */
			inline std::shared_ptr<const sockaddr> SockAddr() const noexcept {
				return m_sock_addr;
			}

		private:
			std::shared_ptr<sockaddr> m_sock_addr;	///< Socket address
			unsigned int m_mtu;						///< MTU (reserved)
			std::string m_ip;						///< IP string
			unsigned short m_port;					///< Port

			/**
			 * @param sock_addr Socket address.
			 */
			Info(std::shared_ptr<sockaddr> sock_addr) noexcept;

			/**
			 * Hostname resolution helper.
			 * @param hostname Host name.
			 * @param port Port.
			 * @param protocol Address family.
			 * @return Shared sockaddr or error.
			 */
			static StormByte::Expected<std::shared_ptr<sockaddr>, Exception> ResolveHostname(const std::string& hostname, const unsigned short& port, const Protocol& protocol) noexcept;

			/**
			 * Fills IP/port from sockaddr.
			 * @param sock_addr Socket address.
			 */
			void Initialize(std::shared_ptr<sockaddr> sock_addr) noexcept;
	};
}
