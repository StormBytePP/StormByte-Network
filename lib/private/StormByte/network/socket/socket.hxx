#pragma once

#include <StormByte/expected.hxx>
#include <StormByte/logger/threaded_log.hxx>
#include <StormByte/network/connection/handler.hxx>
#include <StormByte/network/connection/info.hxx>
#include <StormByte/network/connection/protocol.hxx>
#include <StormByte/network/connection/status.hxx>
#include <StormByte/network/exception.hxx>
#include <StormByte/network/typedefs.hxx>

#include <atomic>

/**
 * @namespace Socket
 * @brief Low-level socket wrappers.
 */
namespace StormByte::Network::Socket {
	class Server;
	class Client;

	/**
	 * @class Socket
	 * @brief Platform socket: create, configure, wait, disconnect.
	 *
	 * Move-only. Owned by Client/Server (friends). `m_status` is atomic for
	 * concurrent Disconnect/Status.
	 */
	class STORMBYTE_NETWORK_PRIVATE Socket {
		friend class Server;
		friend class Client;
		public:
			/**
			 * Copy constructor (deleted).
			 */
			Socket(const Socket& other) = delete;

			/**
			 * Move constructor.
			 */
			Socket(Socket&& other) noexcept;

			/**
			 * Destructor (calls Disconnect).
			 */
			virtual ~Socket() noexcept;

			/**
			 * Copy assignment (deleted).
			 */
			Socket& operator=(const Socket& other) = delete;

			/**
			 * Move assignment.
			 */
			Socket& operator=(Socket&& other) noexcept;

			/**
			 * Graceful shutdown and close (idempotent, thread-safe first-caller wins).
			 */
			virtual void Disconnect() noexcept;

			/**
			 * @return Current connection status.
			 */
			Connection::Status Status() const noexcept {
				return m_status.load(std::memory_order_acquire);
			}

			/**
			 * @return Effective MTU.
			 */
			constexpr const unsigned long& MTU() const noexcept {
				return m_mtu;
			}

			/**
			 * @return Native handle.
			 */
			inline const Connection::HandlerType& Handle() const noexcept {
				return m_handle;
			}

			/**
			 * @return Socket UUID.
			 */
			inline const std::string& UUID() const noexcept {
				return m_UUID;
			}

			/**
			 * Waits for readable data (or peer close / timeout).
			 * @param usecs Timeout in microseconds (0 = non-blocking style wait policy as implemented).
			 * @return Read result or ConnectionClosed.
			 */
			ExpectedReadResult WaitForData(const long long& usecs = 0) noexcept;

		protected:
			Connection::Protocol m_protocol;					///< Protocol
			std::atomic<Connection::Status> m_status;			///< Status
			Connection::HandlerType m_handle;					///< Native handle
			std::unique_ptr<Connection::Info> m_conn_info;		///< Peer info
			unsigned long m_mtu;								///< MTU
			mutable std::shared_ptr<Logger::Log> m_logger;		///< Logger

			int m_effective_send_buf = 65536;	///< SO_SNDBUF effective
			int m_effective_recv_buf = 65536;	///< SO_RCVBUF effective

			/**
			 * @param protocol Address family.
			 * @param logger Logger.
			 */
			Socket(const Connection::Protocol& protocol, std::shared_ptr<Logger::Log> logger) noexcept;

			/**
			 * Creates the OS socket.
			 * @return Handle or ConnectionError.
			 */
			Expected<Connection::HandlerType, ConnectionError> CreateSocket() noexcept;

			/**
			 * Post-connect options: non-blocking, buffers, TCP_NODELAY, MTU.
			 */
			void InitializeAfterConnect() noexcept;

			/**
			 * Ensures the handle is closed (internal helper if used).
			 */
			void EnsureIsClosed() noexcept;

		private:
			constexpr static const unsigned short DEFAULT_MTU = 1500;
			std::string m_UUID;	///< Instance UUID

			/**
			 * @return Path MTU or DEFAULT_MTU.
			 */
			int GetMTU() const noexcept;

			/**
			 * Sets non-blocking mode.
			 */
			void SetNonBlocking() noexcept;
	};
}
