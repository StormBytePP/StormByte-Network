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
using namespace StormByte::Network::Connection;
Client::Client(std::shared_ptr<Socket::Client> socket, Buffer::Pipeline in_pipeline, Buffer::Pipeline out_pipeline) noexcept:
	m_socket(socket),
	m_in_pipeline(in_pipeline),
	m_out_pipeline(out_pipeline)
{}
bool Client::Send(Transport::Frame&& frame, std::shared_ptr<Logger::Log> logger) noexcept {
	ExpectedVoid result = m_socket->Send(frame.ProcessOutput(m_out_pipeline, logger));
	if (!result) {
		logger << Logger::Level::Error << "Failed to send frame to socket: " << result.error()->what();
		return false;
	}
	return true;
}
StormByte::Network::Transport::Frame Client::Receive(std::shared_ptr<Logger::Log> logger) noexcept {
	return Transport::Frame::ProcessInput(m_socket, m_in_pipeline, logger);
}
