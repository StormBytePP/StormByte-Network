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
	 * @brief Represents a network packet.
	 * 
	 * This class serves as a base for implementing custom packets. To create a custom packet:
	 * - Add custom data members as needed.
	 * - Implement the `Deserialize` function to populate the data members using the `reader` function.
	 * - Override the `Serialize` function to populate the buffer. Ensure you call the base class `Serialize` function to include the opcode.
	 */
	class STORMBYTE_NETWORK_PUBLIC Packet {
		public:
			/**
			 * @brief Copy constructor.
			 * @param other The other Packet to copy.
			 */
			Packet(const Packet& other) 								= default;

			/**
			 * @brief Move constructor.
			 * @param other The other Packet to move.
			 */
			Packet(Packet&& other) noexcept								= default;

			/**
			 * @brief Destructor.
			 */
			virtual ~Packet() noexcept									= default;

			/**
			 * @brief Copy assignment operator.
			 * @param other The other Packet to assign.
			 * @return A reference to the assigned Packet.
			 */
			Packet& operator=(const Packet& other) 						= default;

			/**
			 * @brief Move assignment operator.
			 * @param other The other Packet to assign.
			 * @return A reference to the assigned Packet.
			 */
			Packet& operator=(Packet&& other) noexcept					= default;

			/**
			 * @brief Transfers ownership of the buffer to the caller.
			 * @return An rvalue reference to the packet's buffer.
			 */
			Buffer::Simple&&											Buffer() noexcept;

			/**
			 * @brief Provides a memory view of the packet.
			 * @return A constant byte span representing the packet's memory.
			 */
			const Buffer::ConstByteSpan									Span() const noexcept;

			/**
			 * @brief Retrieves the opcode of the packet.
			 * @return A constant reference to the packet's opcode.
			 */
			const unsigned short&										Opcode() const noexcept;

			/**
			 * @brief Reads a packet instance using the provided functions.
			 * @param pif The packet instance function.
			 * @param reader The function to read the packet data.
			 * @return An expected packet or an error.
			 */
			static ExpectedPacket										Read(const PacketInstanceFunction& pif, PacketReaderFunction reader) noexcept;

		protected:
			mutable Buffer::Simple m_buffer;							///< The data buffer of the packet.

			/**
			 * @brief Constructs a Packet with the specified opcode.
			 * @param opcode The opcode of the packet.
			 */
			Packet(const unsigned short& opcode) noexcept;

			/**
			 * @brief Deserializes the packet.
			 * @param reader The function to read the packet data.
			 * @return An expected result or an error.
			 */
			virtual Expected<void, PacketError>							Deserialize(PacketReaderFunction reader) noexcept = 0;

			/**
			 * @brief Serializes the packet.
			 * 
			 * Override this function in derived classes to populate the buffer with custom data.
			 * Ensure the base class `Serialize` function is called to include the opcode.
			 */
			virtual void 												Serialize() const noexcept;

		private:
			const unsigned short m_opcode;								///< The opcode of the packet.
	};
}