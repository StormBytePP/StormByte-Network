#pragma once

#include <StormByte/network/visibility.h>

#include <string>

/**
 * @namespace Connection
 * @brief The namespace containing connection items
 */
namespace StormByte::Network::Connection {
	/**
	 * @namespace Read
	 * @brief The namespace containing read items.
	 */
	namespace Read {
		/**
		 * @enum class Result
		 * @brief The enum class representing the read result.
		 */
		enum class STORMBYTE_NETWORK_PUBLIC Result {
			Success,
			WouldBlock,
			Closed,
			Failed,
			Timeout,
			ShutdownRequest
		};
	}

	/**
	 * @namespace Write
	 * @brief The namespace containing write items.
	 */
	namespace Write {
		/**
		 * @enum class Result
		 * @brief The enum class representing the write result.
		 */
		enum class STORMBYTE_NETWORK_PUBLIC Result {
			Success,
			Failed
		};
	}

	/**
	 * @brief The function to convert the read result to a string.
	 * @param result The read result to convert.
	 * @return The string representation of the read result.
	 */
	constexpr STORMBYTE_NETWORK_PUBLIC std::string ToString(const StormByte::Network::Connection::Read::Result& result) noexcept {
		switch (result) {
			case StormByte::Network::Connection::Read::Result::Success:
				return "Success";
			case StormByte::Network::Connection::Read::Result::WouldBlock:
				return "WouldBlock";
			case StormByte::Network::Connection::Read::Result::Failed:
				return "Failed";
			case StormByte::Network::Connection::Read::Result::Closed:
				return "Closed";
			case StormByte::Network::Connection::Read::Result::Timeout:
				return "Timeout";
			case StormByte::Network::Connection::Read::Result::ShutdownRequest:
				return "ShutdownRequest";
			default:
				return "Unknown";
		}
	}

	/**
	 * @brief The function to convert the write result to a string.
	 * @param result The write result to convert.
	 * @return The string representation of the write result.
	 */
	constexpr STORMBYTE_NETWORK_PUBLIC std::string ToString(const StormByte::Network::Connection::Write::Result& result) noexcept {
		switch (result) {
			case StormByte::Network::Connection::Write::Result::Success:
				return "Success";
			case StormByte::Network::Connection::Write::Result::Failed:
				return "Failed";
			default:
				return "Unknown";
		}
	}
}
