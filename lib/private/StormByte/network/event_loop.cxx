/*
 * Copyright (C) 2024-2026 David C. Manuelda (StormBytePP)
 *
 * This file is part of StormByte-Network.
 *
 * StormByte-Network is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3
 * or later, as published by the Free Software Foundation.
 */

#include <StormByte/network/event_loop.hxx>

#ifdef UNIX
#include <poll.h>
#include <unistd.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <array>

namespace StormByte::Network::Detail {
	EventLoop::EventLoop(Socket::Server& listener, Connection::HandlerType wakeup_read,
		const std::atomic<Connection::Status>& status,
		std::shared_ptr<Logger::Log> logger) noexcept:
	m_listener(listener),
	m_wakeup_read(wakeup_read),
	m_status(status),
	m_logger(std::move(logger)) {}

	ExpectedReadResult EventLoop::Wait() noexcept {
#ifdef UNIX
		std::array<pollfd, 2> descriptors{{
			{ m_listener.Handle(), POLLIN, 0 },
			{ m_wakeup_read, POLLIN, 0 }
		}};
		const int result = poll(descriptors.data(), descriptors.size(), 1000);
		if (result < 0) {
			return Unexpected<ConnectionClosed>("Failed to wait for server events");
		}
		if (result == 0) {
			return Connection::Read::Result::Timeout;
		}
		if (descriptors[1].revents & POLLIN) {
			char signal;
			[[maybe_unused]] const ssize_t received = ::read(m_wakeup_read, &signal, sizeof(signal));
			return Connection::Read::Result::Closed;
		}
		if (descriptors[0].revents & POLLIN) {
			return Connection::Read::Result::Success;
		}
		return Unexpected<ConnectionClosed>("Server listener reported an invalid event");
#else
		fd_set read_fds;
		FD_ZERO(&read_fds);
		FD_SET(m_listener.Handle(), &read_fds);
		FD_SET(m_wakeup_read, &read_fds);
		timeval timeout{ .tv_sec = 1, .tv_usec = 0 };
		const int result = select(0, &read_fds, nullptr, nullptr, &timeout);
		if (result == SOCKET_ERROR) {
			return Unexpected<ConnectionClosed>("Failed to wait for server events");
		}
		if (result == 0) {
			return Connection::Read::Result::Timeout;
		}
		if (FD_ISSET(m_wakeup_read, &read_fds)) {
			char signal;
			(void)::recv(m_wakeup_read, &signal, sizeof(signal), 0);
			return Connection::Read::Result::Closed;
		}
		if (FD_ISSET(m_listener.Handle(), &read_fds)) {
			return Connection::Read::Result::Success;
		}
		return Unexpected<ConnectionClosed>("Server listener reported an invalid event");
#endif
	}

	void EventLoop::Run(const ListenerCallback& on_listener_ready) noexcept {
		while (Connection::IsConnected(m_status.load(std::memory_order_acquire))) {
			auto wait_result = Wait();
			if (!wait_result) {
				m_logger << Logger::Level::Error << wait_result.error()->what() << std::endl;
				return;
			}
			switch (wait_result.value()) {
				case Connection::Read::Result::Success:
					on_listener_ready();
					break;
				case Connection::Read::Result::Timeout:
					continue;
				case Connection::Read::Result::Closed:
					return;
				default:
					continue;
			}
		}
	}
}
