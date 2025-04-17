#pragma once

#include <StormByte/network/data/encryption/typedefs.hxx>
#include <StormByte/network/data/encryption/aes.hxx>
#include <StormByte/network/data/encryption/rsa.hxx>
#include <StormByte/network/data/encryption/ecc.hxx>

#include <functional>
#include <string>

/**
 * @namespace Encryption
 * @brief The namespace containing Encryption functions
 */
namespace StormByte::Network::Encryption {
	namespace Algorithm {
		/**
		 * @enum Name
		 * @brief The enum containing the encryption algorithm names
		 */
		enum class STORMBYTE_NETWORK_PUBLIC Name: unsigned short {
			NONE,																		///< No algorithm.
			AES,																		///< The AES algorithm.
			RSA,																		///< The RSA algorithm.
			ECC																			///< The Ellieptic Curve Cryptography algorithm.
		};

		/**
		 * @enum Type
		 * @brief The enum containing the encryption types
		 */
		enum class STORMBYTE_NETWORK_PUBLIC Type: unsigned short {
			Symmetric,																	///< The symmetric encryption type.
			Asymmetric																	///< The asymmetric encryption type.
		};

		/**
		 * @brief The function to get the encryptor function for the algorithm
		 * @param algorithm The algorithm to get the encryptor function for
		 * @return The encryptor function for the algorithm
		 */
		template<typename T, Algorithm::Name algorithm>
		constexpr std::function<Data::Encryption::ExpectedCryptoFutureString(T&, const std::string&)> Encryptor() {
			if constexpr (algorithm == Algorithm::Name::AES) {
				return &Data::Encryption::AES::Encrypt;
			} else if constexpr (algorithm == Algorithm::Name::RSA) {
				return &Data::Encryption::RSA::Encrypt;
			} else if constexpr (algorithm == Algorithm::Name::ECC) {
				return &Data::Encryption::ECC::Encrypt;
			} else {
				static_assert(algorithm == Algorithm::Name::AES || 
							algorithm == Algorithm::Name::RSA || 
							algorithm == Algorithm::Name::ECC,
							"Unsupported algorithm in Encryptor!");
				return nullptr; // Just for completeness, but unreachable due to static_assert
			}
		}

		/**
		 * @brief The function to get the decryptor function for the algorithm
		 * @param algorithm The algorithm to get the decryptor function for
		 * @return The decryptor function for the algorithm
		 */
		template<typename T, Algorithm::Name algorithm>
		constexpr std::function<Data::Encryption::ExpectedCryptoFutureString(T&, const std::string&)> Decryptor() {
			if constexpr (algorithm == Algorithm::Name::AES) {
				return &Data::Encryption::AES::Decrypt;
			} else if constexpr (algorithm == Algorithm::Name::RSA) {
				return &Data::Encryption::RSA::Decrypt;
			} else if constexpr (algorithm == Algorithm::Name::ECC) {
				return &Data::Encryption::ECC::Decrypt;
			} else {
				static_assert(algorithm == Algorithm::Name::AES || 
							algorithm == Algorithm::Name::RSA || 
							algorithm == Algorithm::Name::ECC,
							"Unsupported algorithm in Decryptor!");
				return nullptr; // Just for completeness, but unreachable due to static_assert
			}
		}

		template<Algorithm::Name algorithm>
		constexpr STORMBYTE_NETWORK_PUBLIC std::function<Data::Encryption::ExpectedKeyPair(const int&)> KeyGenerator() {
			if constexpr (algorithm == Algorithm::Name::RSA) {
				return &Data::Encryption::RSA::GenerateKeyPair;
			} else if constexpr (algorithm == Algorithm::Name::ECC) {
				return &Data::Encryption::ECC::GenerateKeyPair;
			} else {
				static_assert(algorithm == Algorithm::Name::RSA || algorithm == Algorithm::Name::ECC,
							"Unsupported algorithm!");
				return nullptr; // Just for completeness
			}
		}
	}

	/**
	 * @brief The function to get the algorithm type
	 * @param algorithm The algorithm to get the type of
	 * @return The type of the algorithm
	 */
	constexpr Algorithm::Type Type(const Algorithm::Name& algorithm) {
		switch (algorithm) {
			case Algorithm::Name::AES:
				return Algorithm::Type::Symmetric;
			case Algorithm::Name::RSA:
			case Algorithm::Name::ECC:
				return Algorithm::Type::Asymmetric;
		}
	};

	/**
	 * @brief The function to get the algorithm name as a string
	 * @param algorithm The algorithm to get the name of
	 * @return The name of the algorithm
	 */
	constexpr std::string ToString(const Algorithm::Name& algorithm) {
		switch (algorithm) {
			case Algorithm::Name::AES:
				return "AES";
			case Algorithm::Name::RSA:
				return "RSA";
			case Algorithm::Name::ECC:
				return "ECC";
			default:
				return "Unknown";
		}
	};

	/**
	 * @brief The function to get the algorithm type as a string
	 * @param type The type of the algorithm to get the name of
	 * @return The name of the algorithm type
	 */
	constexpr std::string ToString(const Algorithm::Type& type) {
		switch (type) {
			case Algorithm::Type::Symmetric:
				return "Symmetric";
			case Algorithm::Type::Asymmetric:
				return "Asymmetric";
			default:
				return "Unknown";
		}
	};
}