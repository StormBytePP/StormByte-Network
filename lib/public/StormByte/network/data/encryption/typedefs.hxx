#pragma once

#include <StormByte/network/exception.hxx>
#include <StormByte/network/typedefs.hxx>

/**
 * @namespace Encryption
 * @brief The namespace containing Encryption functions
 */
namespace StormByte::Network::Data::Encryption {
	/**
	 * @struct KeyPair
	 * @brief The struct containing private and public keys
	 */
	struct STORMBYTE_NETWORK_PUBLIC KeyPair {
		std::string Private;																		///< The private key.
		std::string Public;																			///< The public key.
	};

	using ExpectedCryptoBuffer = StormByte::Expected<FutureBuffer, CryptoException>;				///< The expected crypto buffer type.
	using ExpectedCryptoString = StormByte::Expected<std::string, CryptoException>;					///< The expected crypto string type.
	using ExpectedKeyPair = StormByte::Expected<KeyPair, CryptoException>;							///< The expected key pair type.
}