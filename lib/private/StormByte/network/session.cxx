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

	bool Session::HasOutput() noexcept {
		return !m_output_frames.empty() && PrepareOutput();
	}

	bool Session::QueueResponse(const PacketPointer& packet, std::shared_ptr<Logger::Log> logger) noexcept {
		if (!packet || m_closed || m_output_frame_count >= MAX_OUTPUT_FRAMES) {
			return false;
		}

		Transport::Frame frame(*packet);
		Buffer::Consumer consumer = frame.ProcessOutput(m_client->OutputPipeline(), std::move(logger));
		m_output_frames.emplace_back(std::move(consumer));
		++m_output_frame_count;
		auto& stream = m_output_frames.back();
		if (!stream.source.EoF()) {
			Buffer::DataType chunk;
			if (stream.source.Extract(64 * 1024, chunk) && !chunk.empty()) {
				m_output_bytes += chunk.size();
				stream.data = std::move(chunk);
			}
		}

		return true;
	}

	StormByte::Expected<bool, ConnectionError> Session::FlushOutput() noexcept {
		if (m_output_frames.empty()) {
			return true;
		}

		if (!PrepareOutput()) {
			return false;
		}

		bool would_block = false;
		auto& frame = m_output_frames.front();
		const std::span<const std::byte> remaining(frame.data.data() + frame.offset, frame.data.size() - frame.offset);
		auto written = m_client->Socket()->TryWrite(remaining, would_block);
		if (!written) {
			return Unexpected(written.error());
		}

		if (would_block) {
			return false;
		}

		frame.offset += written.value();
		m_output_bytes -= written.value();
		if (frame.offset == frame.data.size()) {
			frame.data.clear();
			frame.offset = 0;
			if (frame.source.EoF()) {
				m_output_frames.pop_front();
				--m_output_frame_count;
			}
		}

		return m_output_frames.empty() || PrepareOutput();
	}

	bool Session::PrepareOutput() noexcept {
		if (m_output_frames.empty()) {
			return false;
		}

		auto& frame = m_output_frames.front();
		if (!frame.data.empty()) {
			return true;
		}

		if (frame.source.EoF()) {
			m_output_frames.pop_front();
			--m_output_frame_count;
			return !m_output_frames.empty() && PrepareOutput();
		}

		const std::size_t available_room = MAX_OUTPUT_BYTES - m_output_bytes;
		if (available_room == 0) {
			return false;
		}

		Buffer::DataType chunk;
		const std::size_t available = frame.source.AvailableBytes();
		if (available == 0) {
			return false;
		}

		const std::size_t count = std::min<std::size_t>({ available_room, available, 64 * 1024 });
		if (!frame.source.Extract(count, chunk) || chunk.empty()) {
			return false;
		}

		frame.data = std::move(chunk);
		m_output_bytes += frame.data.size();
		return true;
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

		bool would_block = false;
		auto expected_buffer = m_client->Socket()->TryRead(would_block);
		if (!expected_buffer) {
			m_closed = true;
			return Unexpected(expected_buffer.error());
		}

		if (would_block || expected_buffer->empty()) {
			return FrameList{};
		}

		return AppendReceived(std::move(expected_buffer.value()), in_pipeline, std::move(logger));
	}
}
