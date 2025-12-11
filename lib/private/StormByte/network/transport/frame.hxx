#pragma once

#include <StormByte/buffer/pipeline.hxx>
#include <StormByte/network/transport/packet.hxx>
#include <StormByte/network/typedefs.hxx>

namespace StormByte::Network::Socket {
	class Client;													///< Forward declaration
}

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
	 * @class Frame
	 * @brief Represents a network frame with an opcode and raw payload data.
	 * @details
	 * Add info here
	 * Frame format:
	 * Opcode: sizeof(unsigned short) bytes
	 * Payload size: sizeof(std::size_t) bytes
	 * Payload data: variable size, can be empty
	 */
	class STORMBYTE_NETWORK_PRIVATE Frame {
		public:
			/**
			 * @brief Construct a Frame from a Packet.
			 * @param packet The Packet to serialize into the Frame.
			 */
			Frame(const Packet& packet) noexcept;

			/**
			 * @brief Copy constructor
			 * @param other The Frame to copy from.
			 */
			Frame(const Frame& other) noexcept						= default;
			
			/**
			 * @brief Move constructor
			 * @param other The Frame to move from.
			 */
			Frame(Frame&& other) noexcept							= default;
			
			/**
			 * @brief Destructor
			 */
			virtual ~Frame() noexcept 								= default;
			
			/**
			 * @brief Copy-assignment operator
			 * @param other The Frame to copy from.
			 * @return Reference to this Frame.
			 */
			Frame& operator=(const Frame& other) 					= default;
			
			/**
			 * @brief Move-assignment operator
			 * @param other The Frame to move from.
			 * @return Reference to this Frame.
			 */
			Frame& operator=(Frame&& other) noexcept				= default;

			/**
			 * @brief Process input buffer and extract a Frame.
			 * @param input_data The input buffer to read from.
			 * @param in_pipeline The buffer pipeline to use to process input.
			 * @param logger The logger to use.
			 * @return A Frame containing the processed data.
			 */
			static Frame 											ProcessInput(std::shared_ptr<Socket::Client> client, Buffer::Pipeline& in_pipeline, Logger::Log& logger) noexcept;

			/**
			 * @brief Process input buffer and extract a Frame.
			 * @param packet_fn The function to deserialize packets.
			 * @param logger The logger to use.
			 * @return An PacketPointer containing the processed Packet.
			 */
			PacketPointer 											ProcessPacket(const DeserializePacketFunction& packet_fn, Logger::Log& logger) const noexcept;
			
			/**
			 * @brief Process a Packet and serialize it into a Frame.
			 * @param packet The Packet to process.
			 * @param out_pipeline The buffer pipeline to use to process output.
			 * @param logger The logger to use.
			 * @return A DataType containing the processed Frame as raw bytes.
			 */
			Buffer::Consumer 										ProcessOutput(Buffer::Pipeline& out_pipeline, Logger::Log& logger) const noexcept;

		private:
			Packet::OpcodeType m_opcode;							///< The opcode of the Frame.
			Buffer::DataType m_payload;								///< The payload data of the Frame.

			/**
			 * @brief Default constructor
			 */
			Frame() noexcept 										= default;

			/**
			 * @brief Construct a Frame from a FIFO buffer.
			 * @param buffer The FIFO buffer to use for the Frame.
			 */
			Frame(Packet::OpcodeType opcode, Buffer::DataType&& payload) noexcept:
			m_opcode(opcode),
			m_payload(std::move(payload)) {}
	};
}
