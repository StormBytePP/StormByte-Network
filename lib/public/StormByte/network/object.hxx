#pragma once

#include <StormByte/clonable.hxx>
#include <StormByte/network/visibility.h>

/**
 * @namespace Network
 * @brief The namespace containing all the network related classes.
 */
namespace StormByte::Network {
	// This class represents a generic network object to be inherited to represent whatever other network entity,
	// such as a list of items from database returned from a server.
	// Subinstances must implement Clone and Move virtual methods from Clonable.
	class STORMBYTE_NETWORK_PUBLIC Object: public StormByte::Clonable<Network::Object> {
		public:
			constexpr Object(const Object& other) noexcept = default;
			constexpr Object(Object&& other) noexcept = default;
			virtual ~Object() noexcept = default;
			constexpr Object& operator=(const Object& other) noexcept = default;
			constexpr Object& operator=(Object&& other) noexcept = default;

		protected:
			constexpr Object() noexcept = default;
	};
}