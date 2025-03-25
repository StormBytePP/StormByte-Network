#pragma once

#include <StormByte/network/data/hash/typedefs.h>
#include <StormByte/network/data/hash/sha256.hxx>

#include <functional>
#include <string>

/**
 * @namespace Encryption
 * @brief The namespace containing Encryption functions
 */
namespace StormByte::Network::Hash {
	namespace Algorithm {
		/**
		 * @enum Name
		 * @brief The enum containing the encryption algorithm names
		 */
		enum class STORMBYTE_NETWORK_PUBLIC Name: unsigned short {
			NONE,																		///< No algorithm.
			SHA256,																		///< The SHA-256 algorithm.
		};

		/**
		 * @brief The function to get the encryptor function for the algorithm
		 * @param algorithm The algorithm to get the encryptor function for
		 * @return The encryptor function for the algorithm
		 */
		template<typename T, Algorithm::Name algorithm>
		constexpr std::function<Data::Hash::ExpectedHashString(T&, const std::string&)> Hasher() {
			if constexpr (algorithm == Algorithm::Name::SHA256) {
				return &Data::Hash::SHA256::Hash;
			} else {
				static_assert(false, "Unsupported algorithm in Hasher!");
				return nullptr; // Just for completeness, but unreachable due to static_assert
			}
		}
	}

	/**
	 * @brief The function to get the algorithm name as a string
	 * @param algorithm The algorithm to get the name of
	 * @return The name of the algorithm
	 */
	constexpr std::string ToString(const Algorithm::Name& algorithm) {
		switch (algorithm) {
			case Algorithm::Name::SHA256:
				return "SHA256";
			default:
				return "Unknown";
		}
	};
}