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

#include <StormByte/buffer/fifo.hxx>
#include <StormByte/network/visibility.h>
#include <StormByte/serializable.hxx>

/**
 * @brief Transport types of the Network module.
 */
namespace StormByte::Network::Transport {
	/**
	 * @class Packet
	 * @brief Polymorphic wire packet: opcode + payload hook.
	 *
	 * Derive and override DoSerialize() for the payload (excluding opcode). Serialize() writes opcode then payload.
	 *
	 * Opcodes must fit OpcodeType (unsigned short).
	 */
	class STORMBYTE_NETWORK_PUBLIC Packet {
		public:
			using OpcodeType = unsigned short;	///< Opcode storage type

			/**
			 * @brief Copy constructor.
			 */
			Packet(const Packet& other) = default;

			/**
			 * @brief Move constructor.
			 */
			Packet(Packet&& other) noexcept = default;

			/**
			 * @brief Destructor.
			 */
			virtual ~Packet() noexcept = default;

			/**
			 * @brief Copy assignment.
			 */
			Packet& operator=(const Packet& other) = default;

			/**
			 * @brief Move assignment.
			 */
			Packet& operator=(Packet&& other) noexcept = default;

			/**
			 * @brief Stored opcode.
			 * @return Opcode.
			 */
			inline const OpcodeType& Opcode() const noexcept {
				return m_opcode;
			}

			/**
			 * @brief Serialize opcode followed by DoSerialize() payload.
			 * @return Complete on-wire buffer.
			 */
			Buffer::FIFO Serialize() const noexcept;

			/**
			 * @brief Opcodes at or above this value run payload through Buffer pipelines when framing.
			 */
			static constexpr unsigned short PROCESS_THRESHOLD = 10;

		protected:
			OpcodeType m_opcode;	///< Packet opcode

			/**
			 * @brief Construct with an opcode.
			 * @param opcode Packet opcode.
			 */
			constexpr Packet(const OpcodeType& opcode) noexcept:
			m_opcode(opcode) {}

			/**
			 * @brief Payload-only serialization (no opcode).
			 * @return Payload bytes (may be empty).
			 */
			virtual Buffer::DataType DoSerialize() const noexcept = 0;
	};
}
