#pragma once

#include <StormByte/network/visibility.h>
#include <StormByte/util/buffer.hxx>

/**
 * @namespace Connection
 * @brief The namespace containing connection items
 */
namespace StormByte::Network {
	/**
	 * @class Packet
	 * @brief The class representing a packet.
	 * Inherit from this class to have custom packets with custom opcodes
	 */
	class STORMBYTE_NETWORK_PUBLIC Packet {
		public:
			/**
			 * @brief The constructor of the Packet class.
			 */
			Packet(const unsigned short& opcode) noexcept;

			/**
			 * @brief The copy constructor of the Packet class.
			 * @param other The other Packet to copy.
			 */
			Packet(const Packet& other) 					= default;

			/**
			 * @brief The move constructor of the Packet class.
			 * @param other The other Packet to move.
			 */
			Packet(Packet&& other) noexcept					= default;

			/**
			 * @brief The destructor of the Packet class.
			 */
			virtual ~Packet() noexcept						= default;

			/**
			 * @brief The assignment operator of the Packet class.
			 * @param other The other Packet to assign.
			 * @return The reference to the assigned Packet.
			 */
			Packet& operator=(const Packet& other) 			= default;

			/**
			 * @brief The move assignment operator of the Packet class.
			 * @param other The other Packet to assign.
			 * @return The reference to the assigned Packet.
			 */
			Packet& operator=(Packet&& other) noexcept		= default;

			/**
			 * @brief The function to get the opcode of the packet.
			 * @return The opcode of the packet.
			 */
			const std::span<const std::byte>				Data() const noexcept;

		protected:
			unsigned short m_opcode;						///< The opcode of the packet.
			mutable Util::Buffer m_buffer;					///< The data buffer of the packet.

			/**
			 * @brief The function to prepare the buffer.
			 * All derived classes need to implement this function.
			 */
			virtual void 									PrepareBuffer() const noexcept = 0;
	};
}