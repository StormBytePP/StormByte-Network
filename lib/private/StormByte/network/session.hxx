/*
 * Copyright (C) 2024-2026 David C. Manuelda (StormBytePP)
 *
 * This file is part of StormByte-Network.
 */

#pragma once

#include <StormByte/network/connection/client.hxx>
#include <StormByte/network/transport/frame.hxx>
#include <StormByte/network/visibility.h>

#include <vector>

namespace StormByte::Network::Detail {
	/**
	 * @class Session
	 * @brief Incremental frame state for one server-side client connection.
	 */
	class STORMBYTE_NETWORK_PRIVATE Session final {
		public:
			using FrameList = std::vector<Transport::Frame>; ///< Frames parsed from one receive operation.

			/**
			 * @brief Create a session around an existing connection.
			 * @param uuid Client UUID.
			 * @param client High-level connection.
			 */
			Session(std::string uuid, std::shared_ptr<Connection::Client> client) noexcept;

			/** @brief Client UUID. */
			const std::string& UUID() const noexcept;

			/** @brief Underlying high-level connection. */
			std::shared_ptr<Connection::Client>& Client() noexcept;

			/** @brief Whether this session is closed. */
			bool Closed() const noexcept;

			/** @brief Native socket handle. */
			Connection::HandlerType Handle() const noexcept;

			/** @brief Whether one request is executing in the pool. */
			bool InFlight() const noexcept;

			/** @brief Mark one request as executing or completed. */
			void SetInFlight(bool value) noexcept;

			/** @brief Whether the socket can be polled for more input. */
			bool CanRead() const noexcept;

			/** @brief Temporarily block reads when the task queue is full. */
			void SetTaskBlocked(bool value) noexcept;

			/** @brief Queue parsed frames owned by the EventLoop. */
			void QueueFrames(FrameList frames) noexcept;

			/** @brief Whether a parsed frame is waiting for submission. */
			bool HasPendingFrame() const noexcept;

			/** @brief Whether a pending frame may be submitted now. */
			bool ReadyForProcessing() const noexcept;

			/** @brief Remove the next parsed frame. */
			Transport::Frame TakeFrame() noexcept;

			/**
			 * @brief Read and parse complete frames from a ready socket.
			 * @param in_pipeline Input payload pipeline.
			 * @param logger Diagnostic logger.
			 * @return Complete frames, or connection error.
			 */
			StormByte::Expected<FrameList, ConnectionError> ReadReady(
				Buffer::Pipeline& in_pipeline, std::shared_ptr<Logger::Log> logger) noexcept;

			/** @brief Close the session. */
			void Close() noexcept;

		private:
			enum class ParsePhase: unsigned short { Header, Payload }; ///< Parser phase.
			static constexpr std::size_t FRAME_HEADER_SIZE =
				sizeof(Transport::Packet::OpcodeType) + sizeof(std::size_t); ///< Wire header size.

			std::string m_uuid; ///< Client UUID.
			std::shared_ptr<Connection::Client> m_client; ///< Client connection.
			Buffer::DataType m_input; ///< Unparsed bytes.
			Buffer::DataType m_payload; ///< Partial payload.
			Transport::Packet::OpcodeType m_opcode = 0; ///< Current opcode.
			std::size_t m_bytes_needed = FRAME_HEADER_SIZE; ///< Remaining bytes.
			ParsePhase m_phase = ParsePhase::Header; ///< Current parser phase.
			bool m_closed = false; ///< Terminal state.
			bool m_in_flight = false; ///< Request executing in pool.
			bool m_task_blocked = false; ///< Pool queue was full.
			FrameList m_ready_frames; ///< Parsed frames waiting for submission.

			/**
			 * @brief Append bytes and extract complete frames.
			 * @param received Newly received bytes.
			 * @param in_pipeline Input payload pipeline.
			 * @param logger Diagnostic logger.
			 * @return Complete frames or connection error.
			 */
			StormByte::Expected<FrameList, ConnectionError> AppendReceived(
				Buffer::DataType&& received, Buffer::Pipeline& in_pipeline,
				std::shared_ptr<Logger::Log> logger) noexcept;
	};
}
