#pragma once

#include <StormByte/network/visibility.h>

#include <string>

/**
 * @namespace Connection
 * @brief Connection-level types (protocol, status, read/write results).
 */
namespace StormByte::Network::Connection {
	/**
	 * @namespace Read
	 * @brief Read-side result codes.
	 */
	namespace Read {
		/**
		 * @enum Result
		 * @brief Outcome of a wait/read operation.
		 */
		enum class STORMBYTE_NETWORK_PUBLIC Result {
			Success,			///< Data available or read ok
			WouldBlock,			///< Non-blocking: no data yet
			Closed,				///< Local/socket closed
			Failed,				///< Hard failure
			Timeout,			///< Wait timed out
			ShutdownRequest		///< Peer shutdown detected
		};
	}

	/**
	 * @namespace Write
	 * @brief Write-side result codes.
	 */
	namespace Write {
		/**
		 * @enum Result
		 * @brief Outcome of a write operation.
		 */
		enum class STORMBYTE_NETWORK_PUBLIC Result {
			Success,	///< Write completed
			Failed		///< Write failed
		};
	}

	/**
	 * Converts a read result to a string.
	 * @param result Read result.
	 * @return Human-readable name.
	 */
	constexpr STORMBYTE_NETWORK_PUBLIC std::string ToString(const StormByte::Network::Connection::Read::Result& result) noexcept {
		switch (result) {
			case StormByte::Network::Connection::Read::Result::Success:			return "Success";
			case StormByte::Network::Connection::Read::Result::WouldBlock:		return "WouldBlock";
			case StormByte::Network::Connection::Read::Result::Failed:			return "Failed";
			case StormByte::Network::Connection::Read::Result::Closed:			return "Closed";
			case StormByte::Network::Connection::Read::Result::Timeout:			return "Timeout";
			case StormByte::Network::Connection::Read::Result::ShutdownRequest:	return "ShutdownRequest";
			default:															return "Unknown";
		}
	}

	/**
	 * Converts a write result to a string.
	 * @param result Write result.
	 * @return Human-readable name.
	 */
	constexpr STORMBYTE_NETWORK_PUBLIC std::string ToString(const StormByte::Network::Connection::Write::Result& result) noexcept {
		switch (result) {
			case StormByte::Network::Connection::Write::Result::Success:	return "Success";
			case StormByte::Network::Connection::Write::Result::Failed:		return "Failed";
			default:														return "Unknown";
		}
	}
}
