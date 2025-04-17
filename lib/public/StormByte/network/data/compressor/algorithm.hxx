#pragma once

#include <StormByte/network/data/compressor/typedefs.hxx>
#include <StormByte/network/data/compressor/gzip.hxx>

#include <functional>
#include <string>

/**
 * @namespace Encryption
 * @brief The namespace containing Encryption functions
 */
namespace StormByte::Network::Compressor {
	namespace Algorithm {
		/**
		 * @enum Name
		 * @brief The enum containing the encryption algorithm names
		 */
		enum class STORMBYTE_NETWORK_PUBLIC Name: unsigned short {
			NONE,																		///< No algorithm.
			Gzip,																		///< The Gzip algorithm.
		};

		/**
		 * @brief The function to get the encryptor function for the algorithm
		 * @param algorithm The algorithm to get the encryptor function for
		 * @return The encryptor function for the algorithm
		 */
		template<typename T, Algorithm::Name algorithm>
		constexpr std::function<Data::Compressor::ExpectedCompressorFutureString(T&, const std::string&)> Compressor() {
			if constexpr (algorithm == Algorithm::Name::Gzip) {
				return &Data::Compressor::Gzip::Compress;
			} else {
				static_assert(false, "Unsupported algorithm in Compressor!");
				return nullptr; // Just for completeness, but unreachable due to static_assert
			}
		}

		/**
		 * @brief The function to get the decryptor function for the algorithm
		 * @param algorithm The algorithm to get the decryptor function for
		 * @return The decryptor function for the algorithm
		 */
		template<typename T, Algorithm::Name algorithm>
		constexpr std::function<Data::Compressor::ExpectedCompressorFutureString(T&, const std::string&)> Decompressor() {
			if constexpr (algorithm == Algorithm::Name::Gzip) {
				return &Data::Compressor::Gzip::Decompress;
			} else {
				static_assert(false, "Unsupported algorithm in Compressor!");
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
			case Algorithm::Name::Gzip:
				return "Gzip";
			default:
				return "Unknown";
		}
	};
}