#pragma once

#include <StormByte/expected.hxx>
#include <StormByte/logger/threaded_log.hxx>
#include <StormByte/network/connection/handler.hxx>
#include <StormByte/network/connection/info.hxx>
#include <StormByte/network/protocol.hxx>
#include <StormByte/network/connection/status.hxx>
#include <StormByte/network/exception.hxx>
#include <StormByte/network/typedefs.hxx>

/**
 * @namespace Socket
 * @brief The namespace containing all the socket related classes.
 */
namespace StormByte::Network::Socket {
	class Server;
	class Client;

	/**
	 * @class Socket
	 * @brief Low-level socket wrapper used by the connection `Server`.
	 *
	 * Although this class lives in the private API surface, it encapsulates
	 * platform-specific socket creation/configuration and provides a small,
	 * well-documented interface used by higher-level connection code. The
	 * `Server` class is declared a friend and is the intended owner of
	 * `Socket` instances.
	 *
	 * Responsibilities:
	 * - Create and configure the underlying OS socket according to the
	 *   specified `Protocol`.
	 * - Expose status and runtime properties (MTU, effective buffer sizes).
	 * - Provide a `WaitForData` helper used by the event loop to poll for
	 *   incoming data with an optional timeout.
	 * - Perform proper teardown via `Disconnect()` and destructor.
	 *
	 * Thread-safety and ownership:
	 * - `Socket` instances are moveable but not copyable. Move operations
	 *   transfer ownership of the platform handle and associated state.
	 * - The class uses `std::unique_ptr` for owned resources to make
	 *   ownership semantics explicit.
	 */
	class STORMBYTE_NETWORK_PRIVATE Socket {
		friend class Server;
		friend class Client;
		public:
			// Non-copyable: sockets represent unique OS handles.
			Socket(const Socket& other) = delete;

			/**
			 * @brief Move constructor.
			 *
			 * Transfers ownership of the underlying socket handle and
			 * associated connection state. The moved-from instance is left in a
			 * valid but unspecified state where `Disconnect()` is a no-op.
			 */
			Socket(Socket&& other) noexcept;

			/**
			 * @brief Destructor.
			 *
			 * Ensures the socket handle is closed and resources released. The
			 * destructor never throws (`noexcept`).
			 */
			virtual ~Socket() noexcept;

			Socket& operator=(const Socket& other) = delete;

			/**
			 * @brief Move assignment.
			 *
			 * Move-assigns the socket, transferring ownership semantics as with
			 * the move constructor.
			 */
			Socket& operator=(Socket&& other) noexcept;

			/**
			 * @brief Gracefully disconnect the socket.
			 *
			 * Perform protocol-level and OS-level shutdown/close operations. This
			 * is safe to call multiple times and will not throw.
			 */
			virtual void Disconnect() noexcept;

			/**
			 * @brief Current connection status.
			 *
			 * Returns a reference to the `Connection::Status` describing the
			 * current state (Connected/Disconnected/etc.). The reference remains
			 * valid for the lifetime of the `Socket` object.
			 */
			constexpr const Connection::Status& Status() const noexcept { return m_status; }

			/**
			 * @brief Report the current MTU in bytes.
			 *
			 * Returns the effective MTU used for packet size decisions. The
			 * returned reference aliases an internal member — the value itself is
			 * a small integer (unsigned long) and safe to read.
			 */
			constexpr const unsigned long& MTU() const noexcept { return m_mtu; }

			/**
			 * @brief Access the underlying connection handler.
			 *
			 * Returns a reference to the connection handler type owned by this
			 * instance. The returned reference is valid while the `Socket` is
			 * alive and holds the handler.
			 */
			inline const Connection::HandlerType& Handle() const noexcept { return m_handle; }

			/**
			 * @brief Access the UUID associated with the socket.
			 *
			 * Returns a constant reference to the UUID string assigned to this
			 * socket instance.
			 */
			inline const std::string& UUID() const noexcept { return m_UUID; }

			/**
			 * @brief Poll the socket for incoming data.
			 *
			 * Waits up to `usecs` microseconds for data to become available and
			 * returns an `ExpectedReadResult` describing readiness or an error.
			 * Passing `0` performs a non-blocking poll; negative values indicate
			 * an indefinite wait depending on platform support.
			 *
			 * @param usecs Timeout in microseconds (default 0).
			 * @return `ExpectedReadResult` with readiness or error.
			 */
			ExpectedReadResult WaitForData(const long long& usecs = 0) noexcept;

		protected:
			Protocol m_protocol;							///< Protocol family/config
			Connection::Status m_status;					///< Current connection status
			Connection::HandlerType m_handle;				///< Owned connection handler
			std::unique_ptr<Connection::Info> m_conn_info;	///< Optional connection metadata
			unsigned long m_mtu;							///< Active MTU value
			Logger::ThreadedLog m_logger;					///< Logger used for socket diagnostics

			// Effective socket buffer sizes as reported by the OS (bytes).
			// Initialized to a sensible default; updated in InitializeAfterConnect().
			int m_effective_send_buf = 65536;
			int m_effective_recv_buf = 65536;

			/**
			 * @brief Protected constructor.
			 *
			 * Create a socket instance configured for `protocol`. A logger can
			 * be supplied to receive diagnostic messages. The constructor does
			 * not throw and leaves the object ready for `CreateSocket()`.
			 */
			Socket(const Protocol& protocol, Logger::ThreadedLog logger) noexcept;

			/**
			 * @brief Create and configure the underlying OS socket.
			 *
			 * Returns either a `Connection::HandlerType` on success or a
			 * `ConnectionError` describing failure. This function encapsulates
			 * platform-dependent socket calls and returns an `Expected` to avoid
			 * exceptions in the networking layer.
			 */
			Expected<Connection::HandlerType, ConnectionError> 	CreateSocket() noexcept;

			/**
			 * @brief Perform initialization that requires a connected socket.
			 *
			 * Called after a successful connect to query OS-level properties
			 * (effective buffer sizes, MTU, etc.) and to apply runtime socket
			 * options. This method is noexcept.
			 */
			void 												InitializeAfterConnect() noexcept;

			/**
			 * @brief Ensures the socket is closed.
			 *
			 * Internal helper to close and release the underlying socket
			 * handle. This enabled handle to be closed as late as possible
			 */
			void 												EnsureIsClosed() noexcept;

		private:
			constexpr static const unsigned short DEFAULT_MTU = 1500;
			std::string m_UUID;									///< UUID associated with the socket

			/**
			 * @brief Query the path MTU for the connected peer.
			 *
			 * Returns the MTU used for decision-making about packet sizes. The
			 * function is `noexcept` and falls back to `DEFAULT_MTU` on error.
			 */
			int GetMTU() const noexcept;

			/**
			 * @brief Set the socket to non-blocking mode.
			 *
			 * Applies platform-specific calls to mark the underlying socket as
			 * non-blocking. This method is used during initialization and is
			 * `noexcept`.
			 */
			void SetNonBlocking() noexcept;
		};
}