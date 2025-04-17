#pragma once

#include <StormByte/network/exception.hxx>
#include <StormByte/network/typedefs.hxx>

/**
 * @namespace Compressor
 * @brief The namespace containing Compression functions
 */
namespace StormByte::Network::Data::Compressor {
	using ExpectedCompressorFutureBuffer = StormByte::Expected<FutureBuffer, CryptoException>;				///< The expected crypto buffer type.
	using ExpectedCompressorFutureString = StormByte::Expected<std::string, CryptoException>;				///< The expected crypto string type.
}