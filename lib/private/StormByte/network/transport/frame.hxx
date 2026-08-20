#pragma once

#include <StormByte/buffer/pipeline.hxx>
#include <StormByte/network/transport/packet.hxx>
#include <StormByte/network/typedefs.hxx>

namespace StormByte::Network::Socket {
	class Client;	///< Forward declaration
}

/**
 * @namespace Transport
 * @brief Application-layer messages (Packet, Frame) and on-wire layout.
 */
namespace StormByte::Network::Transport {
	/**
	 * @class Frame
	 * @brief On-wire unit: opcode + payload size + payload.
	 *
	 * Layout:
	 * - Opcode: sizeof(Packet::OpcodeType)
	 * - Payload size: sizeof(std::size_t)
	 * - Payload: variable (may be empty)
	 *
	 * Opcodes >= Packet::PROCESS_THRESHOLD run payload through pipelines.
	 */
	class STORMBYTE_NETWORK_PRIVATE Frame {
		public:
			/**
			 * Builds a frame from a packet (serializes payload, strips opcode from buffer).
			 * @param packet Source packet.
			 */
			Frame(const Packet& packet) noexcept;

			/**
			 * Copy constructor.
			 */
			Frame(const Frame& other) noexcept = default;

			/**
			 * Move constructor.
			 */
			Frame(Frame&& other) noexcept = default;

			/**
			 * Destructor.
			 */
			virtual ~Frame() noexcept = default;

			/**
			 * Copy assignment.
			 */
			Frame& operator=(const Frame& other) = default;

			/**
			 * Move assignment.
			 */
			Frame& operator=(Frame&& other) noexcept = default;

			/**
			 * Reads one frame from the socket (opcode, size, payload + optional pipeline).
			 * @param client Socket client.
			 * @param in_pipeline Input pipeline.
			 * @param logger Logger.
			 * @return Frame (default-constructed on failure).
			 */
			static Frame ProcessInput(std::shared_ptr<Socket::Client> client, Buffer::Pipeline& in_pipeline, std::shared_ptr<Logger::Log> logger) noexcept;

			/**
			 * Deserializes payload into a Packet via @p packet_fn.
			 * @param packet_fn Deserializer callback.
			 * @param logger Logger.
			 * @return Packet pointer, or nullptr on failure.
			 */
			PacketPointer ProcessPacket(const DeserializePacketFunction& packet_fn, std::shared_ptr<Logger::Log> logger) noexcept;

			/**
			 * Serializes this frame to a Consumer (opcode, size, payload + optional pipeline).
			 * @param out_pipeline Output pipeline.
			 * @param logger Logger.
			 * @return Consumer of framed bytes.
			 */
			Buffer::Consumer ProcessOutput(Buffer::Pipeline& out_pipeline, std::shared_ptr<Logger::Log> logger) noexcept;

		private:
			Packet::OpcodeType m_opcode;	///< Opcode
			Buffer::DataType m_payload;		///< Payload bytes

			/**
			 * Empty frame (error path).
			 */
			Frame() noexcept = default;

			/**
			 * @param opcode Opcode.
			 * @param payload Payload (moved).
			 */
			Frame(Packet::OpcodeType opcode, Buffer::DataType&& payload) noexcept:
			m_opcode(opcode),
			m_payload(std::move(payload)) {}
	};
}
