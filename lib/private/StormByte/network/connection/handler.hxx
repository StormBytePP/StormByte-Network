#pragma once

#include <StormByte/network/typedefs.hxx>

#ifdef WINDOWS
#include <winsock2.h>
#endif

#include <string>

/**
 * @namespace Connection
 * @brief Connection helpers (handler, info, client wrapper).
 */
namespace StormByte::Network::Connection {
	/**
	 * @class Handler
	 * @brief Platform network bootstrap and last-error helpers (singleton).
	 *
	 * Performs WSAStartup on Windows. Non-copyable / non-movable.
	 */
	class STORMBYTE_NETWORK_PRIVATE Handler {
		public:
			/**
			 * Copy constructor (deleted).
			 */
			Handler(const Handler& other) = delete;

			/**
			 * Move constructor (deleted).
			 */
			Handler(Handler&& other) noexcept = delete;

			/**
			 * Destructor (WSACleanup on Windows).
			 */
			~Handler() noexcept;

			/**
			 * Copy assignment (deleted).
			 */
			Handler& operator=(const Handler& other) = delete;

			/**
			 * Move assignment (deleted).
			 */
			Handler& operator=(Handler&& other) noexcept = delete;

			/**
			 * @return Global Handler instance.
			 */
			static Handler& Instance() noexcept;

			/**
			 * @return Human-readable last network error (platform-specific).
			 */
			std::string LastError() const noexcept;

			/**
			 * @return Raw last error code (errno / WSAGetLastError).
			 */
			int LastErrorCode() const noexcept;

			/**
			 * Converts a platform error code to a string.
			 * @param errnum Error code.
			 * @return Description, or numeric string on failure.
			 */
			std::string ErrnoToString(int errnum) const noexcept;

		private:
			bool m_initialized = false;	///< Initialization flag
			#ifdef WINDOWS
			WSADATA m_wsaData;			///< Winsock data
			#endif

			/**
			 * Private constructor (singleton).
			 */
			Handler() noexcept;
	};
}
