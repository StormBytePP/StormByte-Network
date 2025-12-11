#pragma once

#include <StormByte/buffer/fifo.hxx>
#include <StormByte/network/visibility.h>
#include <StormByte/serializable.hxx>

/**
 * @namespace Transport
 * @brief Application-layer transport types: packets, frames and serialization.
 *
 * @details
 * The `Transport` namespace contains types that represent application-layer
 * messages and their on-the-wire encodings (for example `Packet` and
 * `Frame`). These types describe the structure and serialization of
 * messages used by higher-level protocols and application code. This
 * namespace intentionally focuses on message format and framing and is
 * distinct from IP/network-layer protocol concepts (such as IPv4/IPv6).
 */
namespace StormByte::Network::Transport {
	/**
	 * @class Packet
	 * @brief Base class for wire-level packets.
	 *
	 * @details
	 * `Packet` provides a minimal, polymorphic representation of an application
	 * protocol packet. It stores an opcode value (see `OpcodeType`) and exposes
	 * a protected serialization hook that derived classes override to append
	 * their payload bytes. The base `Serialize()` implementation serializes
	 * the opcode and then appends the result of `DoSerialize()`.
	 *
	 * @par Usage
	 * - Implementers should store packet-specific fields as members in their
	 *   derived class and override `DoSerialize()` to return the on-wire
	 *   representation of the packet payload (excluding the opcode).
	 * - Callers should use `Serialize()` to obtain the complete byte buffer
	 *   for transport.
	 *
	 * @note
	 * - `OpcodeType` is the integral type used to store opcode values (currently
	 *   `unsigned short`). When constructing derived packets you may use your
	 *   own enum types, provided their values are convertible to `OpcodeType`
	 *   and fall within its non-negative representable range (no negative
	 *   values and not greater than the maximum representable value of
	 *   `OpcodeType`, e.g. 65535 for `unsigned short`). Avoid targeting this
	 *   base's `OpcodeType` with signed enums that may contain negative values.
	 *
	 * @note
	 * - If you require runtime polymorphism across different opcode enum types
	 *   (for example storing packets whose opcodes come from distinct enums in
	 *   the same container), introduce a non-templated `PacketBase` interface in
	 *   your project and have your `Packet`-derived types inherit from it. That
	 *   interface should declare the non-templated virtual API you need (for
	 *   example `Serialize()` / `DoSerialize()` returning `Buffer::FIFO`).
	 */
	class STORMBYTE_NETWORK_PUBLIC Packet {
		public:
			using OpcodeType = unsigned short;					///< The type of the opcode.

			/**
			 * @brief Copy constructor.
			 *
			 * Performs a member-wise copy of the opcode and any other members
			 * provided by derived classes. If a derived class contains
			 * non-copyable resources it should provide its own copy constructor.
			 */
			Packet(const Packet& other) 						= default;

			/**
			 * @brief Move constructor.
			 *
			 * Move-constructs members. Marked `noexcept` to allow efficient
			 * moves in containers; override this if derived classes require
			 * different move semantics.
			 */
			Packet(Packet&& other) noexcept 					= default;

			/**
			 * @brief Virtual destructor.
			 *
			 * Virtual to allow derived types to be destroyed through base
			 * pointers. The class remains abstract because `DoSerialize()`
			 * is pure virtual.
			 */
			virtual ~Packet() noexcept 							= default;

			/**
			 * @brief Copy assignment operator.
			 *
			 * Defaulted; performs member-wise assignment. Derived types that
			 * manage non-copyable state should provide their own assignment
			 * operator.
			 */
			Packet& operator=(const Packet& other) 				= default;

			/**
			 * @brief Move assignment operator.
			 *
			 * Defaulted and marked `noexcept`; moves member state where possible.
			 * Override in derived classes if different semantics are required.
			 */
			Packet& operator=(Packet&& other) noexcept 			= default;

			/**
			 * @brief Retrieve the packet opcode.
			 *
			 * Returns a reference to the stored opcode. The value is stable for
			 * the lifetime of the object.
			 *
			 * @return Constant reference to the opcode.
			 */
			inline const OpcodeType& 							Opcode() const noexcept {
				return m_opcode;
			}

			/**
			 * @brief Serialize the packet to a byte buffer.
			 *
			 * Serializes the packet by first writing the opcode as
			 * `OpcodeType`, followed by the result of `DoSerialize()`.
			 *
			 * @return A `Buffer::FIFO` containing the serialized packet.
			 */
			Buffer::FIFO 										Serialize() const noexcept;

			/**
			 * @brief Threshold for processing packets.
			 * @details
			 * Packets with payload sizes equal to or above this threshold
			 * will be processed (e.g., compressed/encrypted) when being
			 * serialized into frames. Packets with smaller payloads will be
			 * transmitted unprocessed.
			 */
			static constexpr unsigned short 					PROCESS_THRESHOLD = 10;			///< The threshold for processing packets.

		protected:
			OpcodeType m_opcode; 								///< The opcode of the packet.

			/**
			 * @brief Protected ctor.
			 *
			 * Construct a packet with the given opcode. Protected so that only
			 * derived classes may create concrete packets.
			 *
			 * @param opcode The packet opcode.
			 */
			constexpr Packet(const OpcodeType& opcode) noexcept:
			m_opcode(opcode) {}

			/**
			 * @brief Packet-specific serialization hook.
			 *
			 * Derived classes must override this method to provide their
			 * specific on-wire payload serialization (excluding the opcode).
			 * The base `Serialize()` implementation calls this method and
			 * appends its result after serializing the opcode.
			 *
			 * @return A `Buffer::DataType` containing the serialized payload.
			 */
			virtual Buffer::DataType 							DoSerialize() const noexcept = 0;
	};
}