/*
 * Copyright (C) 2024-2026 David C. Manuelda (StormBytePP)
 *
 * This file is part of StormByte-Network.
 *
 * StormByte-Network is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3
 * or later, as published by the Free Software Foundation.
 */
/**
 * @brief Private server implementation details.
 */
namespace StormByte::Network::Detail {
	/**
	 * @class Session
	 * @brief Incremental frame state for one server-side client connection.
	 *
	 * The class isolates receive buffering and frame parsing so a later event
	 * loop can replace the thread without changing the public server API.
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

			/**
			 * @brief Client UUID.
			 * @return UUID.
			 */
			const std::string& UUID() const noexcept;

			/**
			 * @brief Underlying high-level connection.
			 * @return Connection.
			 */
			std::shared_ptr<Connection::Client>& Client() noexcept;

			/**
			 * @brief Whether the session has reached a terminal receive state.
			 * @return true after a receive or parse failure.
			 */
			bool Closed() const noexcept;

			/**
			 * @brief Native socket handle for event-loop registration.
			 * @return Socket handle.
			 */
			Connection::HandlerType Handle() const noexcept;

			/**
			 * @brief Read and parse complete frames from a ready socket.
			 * @param in_pipeline Input payload pipeline.
			 * @param logger Diagnostic logger.
			 * @return Complete frames, or connection error.
			 */
			StormByte::Expected<FrameList, ConnectionError> ReadReady(
				Buffer::Pipeline& in_pipeline, std::shared_ptr<Logger::Log> logger) noexcept;

			/**
			 * @brief Close the session and wake its worker.
			 */
			void Close() noexcept;

			/**
			 * @brief Receive bytes and extract every complete frame currently available.
			 * @param in_pipeline Input payload pipeline.
			 * @param logger Diagnostic logger.
			 * @return Complete frames, or a connection error.
		 */
			StormByte::Expected<FrameList, ConnectionError> AppendAndTakeFrames(
				Buffer::Pipeline& in_pipeline, std::shared_ptr<Logger::Log> logger) noexcept;

		private:
			enum class ParsePhase: unsigned short {
				Header,
				Payload
			};

			static constexpr std::size_t FRAME_HEADER_SIZE =
				sizeof(Transport::Packet::OpcodeType) + sizeof(std::size_t); ///< Wire header size

			std::string m_uuid; ///< Client UUID
			std::shared_ptr<Connection::Client> m_client; ///< Client connection
			Buffer::DataType m_input; ///< Unparsed bytes
			Buffer::DataType m_payload; ///< Partial payload for the current frame
			Transport::Packet::OpcodeType m_opcode = 0; ///< Current frame opcode
			std::size_t m_bytes_needed = FRAME_HEADER_SIZE; ///< Remaining bytes in the current phase
			ParsePhase m_phase = ParsePhase::Header; ///< Current parser phase
			bool m_closed = false; ///< Terminal receive state

			/**
			 * @brief Append bytes to the parser and extract complete frames.
			 * @param received Newly received bytes.
			 * @param in_pipeline Input payload pipeline.
			 * @param logger Diagnostic logger.
			 * @return Empty on success, or connection error.
			 */
			StormByte::Expected<FrameList, ConnectionError> AppendReceived(
				Buffer::DataType&& received, Buffer::Pipeline& in_pipeline,
				std::shared_ptr<Logger::Log> logger) noexcept;
	};
}
