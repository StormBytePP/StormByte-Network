/*
 * Copyright (C) 2024-2026 David C. Manuelda (StormBytePP)
 *
 * This file is part of StormByte-Network.
 *
 * StormByte-Network is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3
 * or later, as published by the Free Software Foundation.
 */

#include <StormByte/network/session.hxx>
#include <StormByte/serializable.hxx>

#include <iterator>

namespace StormByte::Network::Detail {
	Session::Session(std::string uuid, std::shared_ptr<Connection::Client> client) noexcept:
	m_uuid(std::move(uuid)),
	m_client(std::move(client)) {}

	const std::string& Session::UUID() const noexcept {
		return m_uuid;
	}

	std::shared_ptr<Connection::Client>& Session::Client() noexcept {
		return m_client;
	}

	bool Session::Closed() const noexcept {
		return m_closed;
	}

	StormByte::Expected<Session::FrameList, ConnectionError> Session::AppendAndTakeFrames(
		Buffer::Pipeline& in_pipeline, std::shared_ptr<Logger::Log> logger) noexcept {
		if (m_closed || !m_client || !m_client->Socket()) {
			return Unexpected<ConnectionError>("Session is closed");
		}

		auto expected_buffer = m_client->Socket()->Receive(0);
		if (!expected_buffer) {
			m_closed = true;
			return Unexpected(expected_buffer.error());
		}

		Buffer::DataType received;
		if (!expected_buffer->Extract(0, received) || received.empty()) {
			m_closed = true;
			return Unexpected<ConnectionError>("Session received no data");
		}
		m_input.insert(m_input.end(),
			std::make_move_iterator(received.begin()),
			std::make_move_iterator(received.end()));

		FrameList frames;
		while (!m_closed) {
			if (m_phase == ParsePhase::Header) {
				if (m_input.size() < FRAME_HEADER_SIZE) {
					m_bytes_needed = FRAME_HEADER_SIZE - m_input.size();
					break;
				}

				Buffer::DataType opcode_data(
					m_input.begin(),
					m_input.begin() + sizeof(Transport::Packet::OpcodeType));
				Buffer::DataType size_data(
					m_input.begin() + sizeof(Transport::Packet::OpcodeType),
					m_input.begin() + FRAME_HEADER_SIZE);
				auto expected_opcode = Serializable<Transport::Packet::OpcodeType>::Deserialize(opcode_data);
				auto expected_size = Serializable<std::size_t>::Deserialize(size_data);
				if (!expected_opcode || !expected_size) {
					m_closed = true;
					return Unexpected<ConnectionError>("Session received an invalid frame header");
				}

				m_opcode = *expected_opcode;
				m_payload.clear();
				m_payload.reserve(*expected_size);
				m_bytes_needed = *expected_size;
				m_phase = ParsePhase::Payload;
				m_input.erase(m_input.begin(), m_input.begin() + FRAME_HEADER_SIZE);
			}

			if (m_phase == ParsePhase::Payload) {
				const std::size_t available = std::min(m_bytes_needed, m_input.size());
				if (available > 0) {
					m_payload.insert(m_payload.end(),
						std::make_move_iterator(m_input.begin()),
						std::make_move_iterator(m_input.begin() + available));
					m_input.erase(m_input.begin(), m_input.begin() + available);
					m_bytes_needed -= available;
				}
				if (m_bytes_needed > 0) {
					break;
				}

				frames.emplace_back(Transport::Frame::FromWire(
					m_opcode, std::move(m_payload), in_pipeline, logger));
				m_payload.clear();
				m_phase = ParsePhase::Header;
				m_bytes_needed = FRAME_HEADER_SIZE;
			}
		}
		return frames;
	}
}
