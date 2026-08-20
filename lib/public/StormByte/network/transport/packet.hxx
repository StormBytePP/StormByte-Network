#pragma once

#include <StormByte/buffer/fifo.hxx>
#include <StormByte/network/visibility.h>
#include <StormByte/serializable.hxx>

/**
 * @namespace Transport
 * @brief Application-layer messages (Packet, Frame) and on-wire layout.
 *
 * Distinct from IP-level protocols (IPv4/IPv6).
 */
namespace StormByte::Network::Transport {
	/**
	 * @class Packet
	 * @brief Polymorphic wire packet: opcode + payload serialization hook.
	 *
	 * Derive and override @ref DoSerialize() for the payload (excluding opcode).
	 * @ref Serialize() writes opcode then payload.
	 *
	 * Opcode values must fit in @ref OpcodeType (unsigned short). Prefer
	 * non-negative enum values convertible to that range.
	 */
	class STORMBYTE_NETWORK_PUBLIC Packet {
		public:
			using OpcodeType = unsigned short;	///< Opcode storage type

			/**
			 * Copy constructor.
			 */
			Packet(const Packet& other) = default;

			/**
			 * Move constructor.
			 */
			Packet(Packet&& other) noexcept = default;

			/**
			 * Destructor.
			 */
			virtual ~Packet() noexcept = default;

			/**
			 * Copy assignment.
			 */
			Packet& operator=(const Packet& other) = default;

			/**
			 * Move assignment.
			 */
			Packet& operator=(Packet&& other) noexcept = default;

			/**
			 * @return Stored opcode.
			 */
			inline const OpcodeType& Opcode() const noexcept {
				return m_opcode;
			}

			/**
			 * Serializes opcode followed by @ref DoSerialize() payload.
			 * @return Complete on-wire buffer.
			 */
			Buffer::FIFO Serialize() const noexcept;

			/**
			 * Opcodes at or above this value run payload through Buffer pipelines
			 * (e.g. compression) when framing.
			 */
			static constexpr unsigned short PROCESS_THRESHOLD = 10;

		protected:
			OpcodeType m_opcode;	///< Packet opcode

			/**
			 * @param opcode Packet opcode.
			 */
			constexpr Packet(const OpcodeType& opcode) noexcept:
			m_opcode(opcode) {}

			/**
			 * Payload-only serialization (no opcode).
			 * @return Payload bytes (may be empty).
			 */
			virtual Buffer::DataType DoSerialize() const noexcept = 0;
	};
}
