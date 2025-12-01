#pragma once

#include <StormByte/network/typedefs.hxx>

#include <memory>

/**
 * @namespace StormByte::Network
 * @brief The namespace containing all the network-related classes.
 */
namespace StormByte::Network {
	/**
	 * @class Packet
	 * @brief Base class for wire-level packets.
	 *
	 * `Packet` provides a minimal, polymorphic representation of an application
	 * protocol packet. It encapsulates an opcode (message identifier) and a
	 * serialization hook that derived types override to append their payload.
	 *
	 * Usage guidance for implementers:
	 * - Store packet-specific fields as members in your derived class.
	 * - Override `DoSerialize()` to produce a `Buffer::FIFO` containing the
	 *   packet's on-the-wire bytes. The base implementation serializes the
	 *   opcode; derived implementations typically call `Packet::DoSerialize()`
	 *   to obtain that opcode buffer and then append their payload.
	 * - Callers should use `Serialize()` as the public convenience wrapper to
	 *   obtain the final byte buffer for transport.
	 *
	 * Notes:
	 * - The destructor is declared abstract in the header to make `Packet`
	 *   an abstract base class; a definition is provided in the implementation
	 *   file so destruction works correctly for derived types.
	 */
	class STORMBYTE_NETWORK_PUBLIC Packet {
		public:
			/**
			 * @brief Copy constructor.
			 *
			 * Performs a member-wise copy of the opcode and any other members
			 * provided by derived classes. If a derived class contains
			 * non-copyable resources it should provide its own copy constructor.
			 */
			Packet(const Packet& other) 							= default;

			/**
			 * @brief Move constructor.
			 *
			 * Move-constructs members. Marked `noexcept` to allow efficient
			 * moves in containers; override this if derived classes require
			 * different move semantics.
			 */
			Packet(Packet&& other) noexcept							= default;

			/**
			 * @brief Virtual destructor.
			 *
			 * Declared abstract in the header to keep `Packet` an abstract base
			 * type; a concrete definition exists in the implementation file to
			 * ensure proper destruction of derived objects.
			 */
			virtual ~Packet() noexcept									= 0;

			/**
			 * @brief Copy assignment operator.
			 *
			 * Defaulted; performs member-wise assignment. Derived types that
			 * manage non-copyable state should provide their own assignment
			 * operator.
			 */
			Packet& operator=(const Packet& other) 						= default;

			/**
			 * @brief Move assignment operator.
			 *
			 * Defaulted and marked `noexcept`; moves member state where possible.
			 * Override in derived classes if different semantics are required.
			 */
			Packet& operator=(Packet&& other) noexcept					= default;

			/**
			 * @brief Retrieve the packet opcode.
			 *
			 * Returns a reference to the stored opcode. The value is stable for
			 * the lifetime of the object.
			 *
			 * @return Constant reference to the opcode.
			 */
			const unsigned short&										Opcode() const noexcept;

			/**
			 * @brief Public serialization entry point.
			 *
			 * Convenience wrapper that returns the buffer produced by
			 * `DoSerialize()`. Call this to obtain the final bytes that
			 * should be written to the transport.
			 *
			 * @return A `Buffer::FIFO` containing the serialized packet.
			 */
			inline Buffer::FIFO											Serialize() const noexcept {
				return DoSerialize();
			}

		protected:
			const unsigned short m_opcode;								///< The opcode of the packet.

			/**
			 * @brief Protected ctor.
			 *
			 * Construct a packet with the given opcode. Protected so that only
			 * derived classes may create concrete packets.
			 *
			 * @param opcode The packet opcode.
			 */
			Packet(const unsigned short& opcode) noexcept;

			/**
			 * @brief Protected serialization hook.
			 *
			 * Derived classes override this to produce the on-the-wire byte
			 * representation. The base implementation serializes the packet's
			 * opcode and returns a buffer containing only that opcode; derived
			 * implementations should typically call `Packet::DoSerialize()` to
			 * obtain the opcode buffer and then append their payload before
			 * returning the combined buffer.
			 *
			 * @return A `Buffer::FIFO` with the serialized packet bytes.
			 */
			virtual Buffer::FIFO										DoSerialize() const noexcept;
	};
}