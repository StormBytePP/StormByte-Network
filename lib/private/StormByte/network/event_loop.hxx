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
#include <StormByte/network/typedefs.hxx>
#include <StormByte/network/visibility.h>

#include <atomic>
#include <functional>
#include <memory>

namespace StormByte::Network::Detail {
	/**
	 * @class EventLoop
	 * @brief Private listener and wakeup event loop.
	 *
	 * The loop currently covers the listener and shutdown wakeup. Client workers
	 * remain separate while session descriptors are introduced later.
	 */
	class STORMBYTE_NETWORK_PRIVATE EventLoop final {
		public:
			using ListenerCallback = std::function<void()>; ///< Listener-ready callback.

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
			void Run(const ListenerCallback& on_listener_ready) noexcept;

		private:
			Socket::Server& m_listener; ///< Listening socket.
			Connection::HandlerType m_wakeup_read; ///< Wakeup read handle.
			const std::atomic<Connection::Status>& m_status; ///< Server status.
			std::shared_ptr<Logger::Log> m_logger; ///< Diagnostic logger.

			/**
			 * @brief Wait for listener or wakeup activity.
			 * @return Read result.
			 */
			ExpectedReadResult Wait() noexcept;
	};
}
