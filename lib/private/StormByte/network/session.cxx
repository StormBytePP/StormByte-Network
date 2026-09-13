/*
 * Copyright (C) 2024-2026 David C. Manuelda (StormBytePP)
 *
 * This file is part of StormByte-Network.
 */

#include <StormByte/network/session.hxx>
#include <StormByte/serializable.hxx>

#include <iterator>

namespace StormByte::Network::Detail {
	using StormByte::Buffer::DataType;

	Session::Session(std::string uuid, std::shared_ptr<Connection::Client> client) noexcept:
	m_uuid(std::move(uuid)), m_client(std::move(client)) {}

	const std::string& Session::UUID() const noexcept {
		return m_uuid;
	}

	std::shared_ptr<Connection::Client>& Session::Client() noexcept {
		return m_client;
	}

	bool Session::Closed() const noexcept {
		return m_closed;
	}

	Connection::HandlerType Session::Handle() const noexcept {
		return m_client && m_client->Socket() ? m_client->Socket()->Handle() : Connection::HandlerType{};
	}

	bool Session::InFlight() const noexcept {
		return m_in_flight;
	}

	void Session::SetInFlight(bool value) noexcept {
		m_in_flight = value;
	}

	bool Session::CanRead() const noexcept {
		return !m_closed && !m_in_flight && !m_task_blocked && m_ready_frames.empty();
	}

	void Session::SetTaskBlocked(bool value) noexcept {
		m_task_blocked = value;
	}

	void Session::QueueFrames(FrameList frames) noexcept {
		m_ready_frames.insert(
			m_ready_frames.end(),
			std::make_move_iterator(frames.begin()),
			std::make_move_iterator(frames.end()));
	}

	bool Session::HasPendingFrame() const noexcept {
		return !m_ready_frames.empty();
	}

	bool Session::ReadyForProcessing() const noexcept {
		return !m_closed && !m_in_flight && !m_task_blocked && !m_ready_frames.empty();
	}

	Transport::Frame Session::TakeFrame() noexcept {
		Transport::Frame frame = std::move(m_ready_frames.front());
		m_ready_frames.erase(m_ready_frames.begin());
		return frame;
	}

	void Session::Close() noexcept {
		m_closed = true;
	}

	StormByte::Expected<Session::FrameList, ConnectionError> Session::AppendReceived(
		DataType&& received, Buffer::Pipeline& in_pipeline,
		std::shared_ptr<Logger::Log> logger) noexcept {
		if (m_closed) {
			return Unexpected<ConnectionError>("Session is closed");
		}
		if (received.empty()) {
			m_closed = true;
			return Unexpected<ConnectionError>("Session received no data");
		}
		m_input.insert(m_input.end(), std::make_move_iterator(received.begin()), std::make_move_iterator(received.end()));
		FrameList frames;
		while (!m_closed) {
			if (m_phase == ParsePhase::Header) {
				if (m_input.size() < FRAME_HEADER_SIZE) {
					m_bytes_needed = FRAME_HEADER_SIZE - m_input.size();
					break;
				}
				DataType opcode_data(m_input.begin(), m_input.begin() + sizeof(Transport::Packet::OpcodeType));
				DataType size_data(m_input.begin() + sizeof(Transport::Packet::OpcodeType), m_input.begin() + FRAME_HEADER_SIZE);
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
					m_payload.insert(m_payload.end(), std::make_move_iterator(m_input.begin()), std::make_move_iterator(m_input.begin() + available));
					m_input.erase(m_input.begin(), m_input.begin() + available);
					m_bytes_needed -= available;
				}
				if (m_bytes_needed > 0) {
					break;
				}
				frames.emplace_back(Transport::Frame::FromWire(m_opcode, std::move(m_payload), in_pipeline, logger));
				m_payload.clear();
				m_phase = ParsePhase::Header;
				m_bytes_needed = FRAME_HEADER_SIZE;
			}
		}
		return frames;
	}

	StormByte::Expected<Session::FrameList, ConnectionError> Session::ReadReady(
		Buffer::Pipeline& in_pipeline, std::shared_ptr<Logger::Log> logger) noexcept {
		if (m_closed || !m_client || !m_client->Socket()) {
			m_closed = true;
			return Unexpected<ConnectionError>("Session is closed");
		}
		auto expected_buffer = m_client->Socket()->Receive(0);
		if (!expected_buffer) {
			m_closed = true;
			return Unexpected(expected_buffer.error());
		}
		DataType received;
		if (!expected_buffer->Extract(0, received)) {
			m_closed = true;
			return Unexpected<ConnectionError>("Session received no data");
		}
		return AppendReceived(std::move(received), in_pipeline, std::move(logger));
	}
}
