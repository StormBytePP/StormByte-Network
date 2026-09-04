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

#include <StormByte/network/transport/packet.hxx>
#include <StormByte/serializable.hxx>
using StormByte::Buffer::DataType;
using StormByte::Buffer::FIFO;
using namespace StormByte::Network::Transport;
FIFO Packet::Serialize() const noexcept {
	FIFO result;
	result.Write(Serializable<OpcodeType>(m_opcode).Serialize());
	DataType payload = DoSerialize();
	if (!payload.empty()) {
		result.Write(std::move(payload));
	}
	return result;
}
