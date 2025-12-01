#pragma once

#include <StormByte/clonable.hxx>
#include <StormByte/network/visibility.h>

/**
 * @namespace Network
 * @brief The namespace containing all the network related classes.
 */
namespace StormByte::Network {
	/**
	 * @class Object
	 * @brief Generic base for objects transported over the network.
	 *
	 * `Network::Object` is a lightweight polymorphic base type intended for
	 * classes that represent domain objects exchanged between client and
	 * server. Subclasses should implement the cloning/moving contract provided
	 * by `StormByte::Clonable<T>` so instances can be copied or transferred
	 * in a type-erased manner by framework code.
	 *
	 * Design notes:
	 * - The class itself provides defaulted copy/move constructors and
	 *   assignment operators and a virtual destructor to allow safe polymorphic
	 *   destruction.
	 * - Concrete subclasses must implement the `Clone` (and if applicable
	 *   move) virtual functions defined by `StormByte::Clonable<Network::Object>`.
	 * - This type intentionally does not provide any payload members; it is
	 *   purely an interface/marker for network-transportable objects.
	 *
	 * @see StormByte::Clonable
	 * @see https://dev.stormbyte.org/StormByte/classStormByte_1_1Clonable.html
	 */
	class STORMBYTE_NETWORK_PUBLIC Object: public StormByte::Clonable<Network::Object> {
		public:
			/**
			 * @brief Defaulted copy constructor.
			 *
			 * Defaulted and `noexcept` so that derived objects follow value-like
			 * semantics where possible.
			 */
			constexpr Object(const Object& other) noexcept = default;

			/**
			 * @brief Defaulted move constructor.
			 */
			constexpr Object(Object&& other) noexcept = default;

			/**
			 * @brief Virtual destructor.
			 *
			 * Ensures proper cleanup of derived types through base pointers.
			 */
			virtual ~Object() noexcept = default;

			/**
			 * @brief Defaulted copy-assignment.
			 */
			constexpr Object& operator=(const Object& other) noexcept = default;

			/**
			 * @brief Defaulted move-assignment.
			 */
			constexpr Object& operator=(Object&& other) noexcept = default;

		protected:
			/**
			 * @brief Protected default constructor.
			 *
			 * Protected to prevent direct instantiation of `Object`; use a
			 * concrete derived type instead.
			 */
			constexpr Object() noexcept = default;
	};
}