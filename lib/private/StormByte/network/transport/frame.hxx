/*
 * Copyright (C) 2024-2026 David C. Manuelda (StormBytePP)
 *
 * This file is part of StormByte-Network.
 *
 * StormByte-Network is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3
 * or later, as published by the Free Software Foundation.
 *
 * StormByte-Network is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with StormByte-Network. If not, see
 * <https://www.gnu.org/licenses/lgpl-3.0.html>.
 */

#pragma once

#include <StormByte/buffer/pipeline.hxx>
#include <StormByte/network/transport/packet.hxx>
#include <StormByte/network/typedefs.hxx>

namespace StormByte::Network::Socket {
	class Client;	///< Forward declaration
}

/**
 * @brief Transport types of the Network module.
 */
namespace StormByte::Network::Transport {
	/**
	 * @class Frame
	 * @brief On-wire unit: opcode + payload size + payload.
	 *
	 * Layout: Opcode (OpcodeType) + payload size (size_t) + payload.
	 * Opcodes >= Packet::PROCESS_THRESHOLD run payload through pipelines.
	 */
	class STORMBYTE_NETWORK_PRIVATE Frame {
		public:
			/**
			 * @brief Build a frame from a packet.
			 * @param packet Source packet.
			 */
			Frame(const Packet& packet) noexcept;

			/**
			 * @brief Copy constructor.
			 */
			Frame(const Frame& other) noexcept = default;

			/**
			 * @brief Move constructor.
			 */
			Frame(Frame&& other) noexcept = default;

			/**
			 * @brief Destructor.
			 */
			virtual ~Frame() noexcept = default;

			/**
			 * @brief Copy assignment.
			 */
			Frame& operator=(const Frame& other) = default;

			/**
			 * @brief Move assignment.
			 */
			Frame& operator=(Frame&& other) noexcept = default;

			/**
			 * @brief Read one frame from the socket.
			 * @param client Socket client.
			 * @param in_pipeline Input pipeline.
			 * @param logger Logger.
			 * @return Frame (default-constructed on failure).
			 */
			static Frame ProcessInput(std::shared_ptr<Socket::Client> client, Buffer::Pipeline& in_pipeline, std::shared_ptr<Logger::Log> logger) noexcept;

			/**
			 * @brief Deserialize payload into a Packet.
			 * @param packet_fn Deserializer callback.
			 * @param logger Logger.
			 * @return Packet pointer, or nullptr on failure.
			 */
			PacketPointer ProcessPacket(const DeserializePacketFunction& packet_fn, std::shared_ptr<Logger::Log> logger) noexcept;

			/**
			 * @brief Serialize this frame to a Consumer.
			 * @param out_pipeline Output pipeline.
			 * @param logger Logger.
			 * @return Consumer of framed bytes.
			 */
			Buffer::Consumer ProcessOutput(Buffer::Pipeline& out_pipeline, std::shared_ptr<Logger::Log> logger) noexcept;

		private:
			Packet::OpcodeType m_opcode;	///< Opcode
			Buffer::DataType m_payload;		///< Payload bytes

			/**
			 * @brief Empty frame (error path).
			 */
			Frame() noexcept = default;

			/**
			 * @brief Construct from opcode and payload.
			 * @param opcode Opcode.
			 * @param payload Payload (moved).
			 */
			Frame(Packet::OpcodeType opcode, Buffer::DataType&& payload) noexcept:
			m_opcode(opcode),
			m_payload(std::move(payload)) {}
	};
}
