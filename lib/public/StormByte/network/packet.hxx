#pragma once

#include <StormByte/network/typedefs.hxx>

#include <memory>

/**
 * @namespace Network
 * @brief The namespace containing all the network related classes.
 */
namespace StormByte::Network {
	/**
	 * @class Packet
	 * @brief The class representing a packet.
	 * Explicitelly instantiate this class to get packets identified by their opcode
	 * The buffer should contain the opcode and the data.
	 */
	class Packet {
		public:
			/**
			 * @brief The copy constructor of the Packet class.
			 * @param other The other Packet to copy.
			 */
			Packet(const Packet& other) 								= default;

			/**
			 * @brief The move constructor of the Packet class.
			 * @param other The other Packet to move.
			 */
			Packet(Packet&& other) noexcept								= default;

			/**
			 * @brief The destructor of the Packet class.
			 */
			virtual ~Packet() noexcept									= default;

			/**
			 * @brief The assignment operator of the Packet class.
			 * @param other The other Packet to assign.
			 * @return The reference to the assigned Packet.
			 */
			Packet& operator=(const Packet& other) 						= default;

			/**
			 * @brief The move assignment operator of the Packet class.
			 * @param other The other Packet to assign.
			 * @return The reference to the assigned Packet.
			 */
			Packet& operator=(Packet&& other) noexcept					= default;

			/**
			 * @brief The function to get the opcode of the packet.
			 * @return The opcode of the packet.
			 */
			const Buffers::ConstByteSpan								Data() const noexcept;

			/**
			 * @brief The function to get the opcode of the packet.
			 * @return The opcode of the packet.
			 */
			const unsigned short&										Opcode() const noexcept;

			/**
			 * @brief The function to get the packet instance
			 * @param pif The packet instance function.
			 * @param client The client socket.
			 * @return The size of the packet.
			 */
			static ExpectedPacket										Read(const PacketInstanceFunction& pif, Socket::Client& client) noexcept;

		protected:
			Buffers::Simple m_buffer;									///< The data buffer of the packet.

			STORMBYTE_NETWORK_PUBLIC Packet(const unsigned short& opcode) noexcept;

			/**
			 * @brief The function to initialize the packet
			 * @return The expected result
			 */
			virtual Expected<void, PacketError>							Initialize(Socket::Client& client) noexcept = 0;

		private:
			const unsigned short m_opcode;								///< The opcode of the packet.
	};
}