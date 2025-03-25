#pragma once

#include <StormByte/network/exception.hxx>
#include <StormByte/network/typedefs.hxx>

/**
 * @namespace Compressor
 * @brief The namespace containing Compression functions
 */
namespace StormByte::Network::Data::Compressor {
	using ExpectedCompressorBuffer = StormByte::Expected<FutureBuffer, CryptoException>;				///< The expected crypto buffer type.
	using ExpectedCompressorString = StormByte::Expected<std::string, CryptoException>;					///< The expected crypto string type.
}