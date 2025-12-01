#pragma once

#include <StormByte/network/typedefs.hxx>

#ifdef WINDOWS
#include <winsock2.h>
#endif

#include <string>

/**
 * @namespace Connection
 * @brief The namespace containing connection items
 */
namespace StormByte::Network::Connection {
	class STORMBYTE_NETWORK_PUBLIC Handler {
		public:
			#ifdef LINUX
			using Type = int;													///< The type of the socket.
			#else
			using Type = SOCKET;												///< The type of the socket.
			#endif

			/**
			 * @brief The constructor of the Handler class.
			 */
			Handler() noexcept;

			/**
			 * @brief The copy constructor of the Handler class.
			 * @param other The other Handler to copy.
			 */
			Handler(const Handler& other) 										= delete;

			/**
			 * @brief The move constructor of the Handler class.
			 * @param other The other Handler to move.
			 */
			Handler(Handler&& other) noexcept									= delete;

			/**
			 * @brief The destructor of the Handler class.
			 */
			~Handler() noexcept;

			/**
			 * @brief The assignment operator of the Handler class.
			 * @param other The other Handler to assign.
			 * @return The reference to the assigned Handler.
			 */
			Handler& operator=(const Handler& other) 							= delete;

			/**
			 * @brief The move assignment operator of the Handler class.
			 * @param other The other Handler to assign.
			 * @return The reference to the assigned Handler.
			 */
			Handler& operator=(Handler&& other) noexcept						= delete;

			/**
			 * @brief The function to get the last error.
			 * @return The last error.
			 */
			std::string 														LastError() const noexcept;

			/**
			 * @brief Convert an errno code to a human-readable string in a
			 * thread-safe, cross-platform manner.
			 * @param errnum The errno value to stringify.
			 * @return Human-readable error string (or numeric value on failure).
			 */
			std::string 														ErrnoToString(int errnum) const noexcept;

			/**
			 * @brief Get last error code
			 * @return The last error code.
			 */
			int 																LastErrorCode() const noexcept;

		private:
			bool m_initialized = false;											///< The initialization flag.
			#ifdef WINDOWS
			WSADATA m_wsaData;		///< The Handler data.
			#endif
	};
}
