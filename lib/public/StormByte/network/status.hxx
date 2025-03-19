#pragma once

#include <StormByte/visibility.h>

/**
 * @namespace Network
 * @brief The namespace containing all the network related classes.
 */
namespace StormByte::Network {
	/**
	 * @enum Status
	 * @brief The enumeration representing the status of the connection.
	 */
	enum STORMBYTE_NETWORK_PUBLIC Status: unsigned short {
		Connected,			///< The connection is established.
		Disconnected,		///< The connection is closed.
		Connecting,			///< The connection is being established.
		Disconnecting,		///< The connection is being closed.
		Error				///< An error occurred in the connection.
	};

	/**
	 * @brief The function to convert the status to a string.
	 * @param status The status to convert.
	 * @return The string representation of the status.
	 */
	constexpr STORMBYTE_NETWORK_PUBLIC const char* StatusToString(const Status& status) {
		switch (status) {
			case Status::Connected:		return "Connected";
			case Status::Disconnected:	return "Disconnected";
			case Status::Connecting:	return "Connecting";
			case Status::Disconnecting:	return "Disconnecting";
			case Status::Error:
			default:
										return "Error";
		}
	}
}