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
	/**
	 * @class Handler
	 * @brief Platform-specific network initializer and error helper.
	 *
	 * This class performs any platform-specific networking initialization
	 * (for example WSAStartup on Windows) and exposes helpers for retrieving
	 * the last error and converting errno codes to human-readable strings.
	 *
	 * The class is intentionally non-copyable and non-movable and is
	 * intended to be used as a singleton via the `Instance()` accessor.
	 *
	 * Thread-safety: calling `Instance()` is noexcept; initialization is
	 * performed in the constructor and the implementation attempts to be
	 * idempotent. Callers that use the instance across threads should ensure
	 * proper ordering during program startup/shutdown if they require
	 * stronger synchronization guarantees.
	 */
	class STORMBYTE_NETWORK_PRIVATE Handler {
		public:
			/**
			 * @brief Deleted copy constructor.
			 *
			 * The Handler is a singleton and therefore cannot be copied.
			 */
			Handler(const Handler& other) = delete;

			/**
			 * @brief Deleted move constructor.
			 */
			Handler(Handler&& other) noexcept = delete;

			/**
			 * @brief Destructor.
			 *
			 * Performs any necessary cleanup of platform networking resources.
			 */
			~Handler() noexcept;

			/**
			 * @brief Deleted copy-assignment.
			 */
			Handler& operator=(const Handler& other) = delete;

			/**
			 * @brief Deleted move-assignment.
			 */
			Handler& operator=(Handler&& other) noexcept = delete;

			/**
			 * @brief Access the global Handler singleton.
			 *
			 * Returns a reference to the single global `Handler` instance. The
			 * first call ensures platform initialization has been performed.
			 *
			 * @return Reference to the global `Handler`.
			 */
			static Handler& Instance() noexcept;

			/**
			 * @brief Get the last error message recorded by the network layer.
			 *
			 * The returned string contains a human-readable description of the
			 * last error that occurred during networking operations. The exact
			 * content is platform-specific.
			 *
			 * @return Human-readable last error message.
			 */
			std::string LastError() const noexcept;

			/**
			 * @brief Get the raw last error code (platform-specific).
			 *
			 * On POSIX this is typically an errno value; on Windows this will be
			 * the last Winsock error code. Use `ErrnoToString()` to convert
			 * numeric codes to readable text.
			 *
			 * @return Integer error code for the last network error.
			 */
			int LastErrorCode() const noexcept;

			/**
			 * @brief Convert a platform error code to a human-readable string.
			 *
			 * This method accepts a platform-specific error value (errno on POSIX
			 * or a Winsock error on Windows) and returns a thread-safe,
			 * human-readable description. If the conversion fails, the numeric
			 * value is returned as a string.
			 *
			 * @param errnum Platform-specific error code to convert.
			 * @return Human-readable description of `errnum`.
			 */
			std::string ErrnoToString(int errnum) const noexcept;

		private:
			bool m_initialized = false; ///< The initialization flag.
			#ifdef WINDOWS
			WSADATA m_wsaData; ///< The Handler data.
			#endif

			/**
			 * @brief Private constructor performing platform initialization.
			 *
			 * The constructor performs any necessary platform-specific setup
			 * (for example `WSAStartup` on Windows). It is private to enforce
			 * the singleton pattern; use `Instance()` to obtain the global
			 * reference.
			 */
			Handler() noexcept;
	};
}
