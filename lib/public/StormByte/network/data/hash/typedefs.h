#pragma once

#include <StormByte/network/typedefs.hxx>

/**
 * @namespace Hash
 * @brief The namespace containing hashing functions
 */
namespace StormByte::Network::Data::Hash {
	using ExpectedHashFutureBuffer = StormByte::Expected<FutureBuffer, CryptoException>;			///< The expected crypto buffer type.
	using ExpectedHashFutureString = StormByte::Expected<std::string, CryptoException>;				///< The expected crypto string type.
}