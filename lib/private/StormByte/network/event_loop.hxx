/*
 * Copyright (C) 2024-2026 David C. Manuelda (StormBytePP)
 *
 * This file is part of StormByte-Network.
 *
 * StormByte-Network is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3
 * or later, as published by the Free Software Foundation.
 */

#pragma once

#include <StormByte/network/socket/server.hxx>
#include <StormByte/network/session.hxx>
#include <StormByte/network/typedefs.hxx>
#include <StormByte/network/visibility.h>

#include <atomic>
#include <functional>
#include <memory>
#include <vector>

namespace StormByte::Network::Detail {
	/**
	 * @class EventLoop
	 * @brief Private listener and wakeup event loop.
	 *
	 * The loop owns listener, wakeup, and session I/O. Packet processing is
	 * intentionally synchronous here until a bounded worker pool is introduced.
	 */
	class STORMBYTE_NETWORK_PRIVATE EventLoop final {
		public:
			using ListenerCallback = std::function<void()>; ///< Listener-ready callback.
			using SessionList = std::vector<std::shared_ptr<Session>>; ///< Session snapshot.
			using SessionSnapshot = std::function<SessionList()>; ///< Session snapshot callback.
			using SessionCallback = std::function<void(const std::shared_ptr<Session>&)>; ///< Session-ready callback.
			using WakeupCallback = std::function<void()>; ///< Wakeup callback.

			/**
			 * @brief Bind the loop to a listener and wakeup read handle.
			 * @param listener Listening socket.
			 * @param wakeup_read Read end of the private wakeup channel.
			 * @param status Server lifecycle status.
			 * @param logger Diagnostic logger.
			 */
			EventLoop(Socket::Server& listener, Connection::HandlerType wakeup_read,
				const std::atomic<Connection::Status>& status,
				std::shared_ptr<Logger::Log> logger) noexcept;

			/**
			 * @brief Run until the server stops or the wakeup is signalled.
			 * @param on_listener_ready Called when the listener is readable.
			 */
			void Run(const ListenerCallback& on_listener_ready,
				const SessionSnapshot& snapshot,
				const SessionCallback& on_session_ready,
				const WakeupCallback& on_wakeup) noexcept;

		private:
			Socket::Server& m_listener; ///< Listening socket.
			Connection::HandlerType m_wakeup_read; ///< Wakeup read handle.
			const std::atomic<Connection::Status>& m_status; ///< Server status.
			std::shared_ptr<Logger::Log> m_logger; ///< Diagnostic logger.

			enum class EventKind: unsigned short { Timeout, Listener, Session, Wakeup }; ///< Wait event kind.
			struct Event { EventKind kind; std::shared_ptr<Session> session; }; ///< Wait event.

			/**
			 * @brief Wait for listener, wakeup, or a session descriptor.
			 * @param sessions Current session snapshot.
			 * @return Wait event.
			 */
			Expected<Event, ConnectionClosed> Wait(const SessionList& sessions) noexcept;
	};
}
