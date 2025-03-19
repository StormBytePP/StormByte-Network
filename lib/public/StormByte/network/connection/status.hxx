#pragma once

#include <StormByte/visibility.h>

#include <string>

/**
 * @namespace Connection
 * @brief The namespace containing connection items
 */
namespace StormByte::Network::Connection {
	/**
	 * @enum Status
	 * @brief The enumeration representing the status of the connection.
	 */
	enum class STORMBYTE_NETWORK_PUBLIC Status: unsigned short {
		Connected,			///< The connection is established.
		Disconnected,		///< The connection is closed.
		Connecting,			///< The connection is being established.
		Disconnecting,		///< The connection is being closed.
		Negotiating,		///< The connection is being negotiated.
		Rejected,			///< The connection is rejected.
		PeerClosed,			///< The peer closed the connection.
		Error				///< An error occurred in the connection.
	};

	/**
	 * @brief The function to convert the status to a string.
	 * @param status The status to convert.
	 * @return The string representation of the status.
	 */
	constexpr STORMBYTE_NETWORK_PUBLIC std::string StatusToString(const Status& status) {
		switch (status) {
			case Status::Connected:		return "Connected";
			case Status::Disconnected:	return "Disconnected";
			case Status::Connecting:	return "Connecting";
			case Status::Disconnecting:	return "Disconnecting";
			case Status::Negotiating:	return "Negotiating";
			case Status::Rejected:		return "Rejected";
			case Status::PeerClosed:	return "PeerClosed";
			case Status::Error:
			default:
										return "Error";
		}
	}

	/**
	 * @brief The function to check if the connection is connected.
	 * @param status The status to check.
	 * @return True if the connection is connected, false otherwise.
	 */
	constexpr STORMBYTE_NETWORK_PUBLIC bool IsConnected(const Status& status) noexcept {
		return status == Status::Connected || status == Status::Negotiating;
	}
}