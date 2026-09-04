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

#include <StormByte/network/connection/client.hxx>
#include <StormByte/network/endpoint.hxx>
using namespace StormByte::Network;
Endpoint::Endpoint(const DeserializePacketFunction& deserialize_packet_function, std::shared_ptr<Logger::Log> logger) noexcept:
	m_deserialize_packet_function(deserialize_packet_function),
	m_logger(logger) {}
PacketPointer Endpoint::Send(std::shared_ptr<Connection::Client> client_connection, const Transport::Packet& packet) noexcept {
	if (!SendPacket(client_connection, packet)) {
		return nullptr;
	}
	Transport::Frame response_frame = client_connection->Receive(m_logger);
	return response_frame.ProcessPacket(m_deserialize_packet_function, m_logger);
}
bool Endpoint::Reply(std::shared_ptr<Connection::Client> client_connection, const Transport::Packet& packet) noexcept {
	return SendPacket(client_connection, packet);
}
std::shared_ptr<Connection::Client> Endpoint::CreateConnection(std::shared_ptr<Socket::Client> socket) noexcept {
	Buffer::Pipeline in_pipeline = InputPipeline();
	Buffer::Pipeline out_pipeline = OutputPipeline();
	return std::make_shared<Connection::Client>(socket, std::move(in_pipeline), std::move(out_pipeline));
}
bool Endpoint::SendPacket(std::shared_ptr<Connection::Client> client_connection, const Transport::Packet& packet) noexcept {
	if (!client_connection || !Connection::IsConnected(client_connection->Status())) {
		m_logger << Logger::Level::Error << "Cannot send packet: not connected." << std::endl;
		return false;
	}
	if (!client_connection->Send(packet, m_logger)) {
		m_logger << Logger::Level::Error << "Failed to send packet." << std::endl;
		return false;
	}
	return true;
}
