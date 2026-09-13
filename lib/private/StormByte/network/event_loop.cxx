/*
 * Copyright (C) 2024-2026 David C. Manuelda (StormBytePP)
 *
 * This file is part of StormByte-Network.
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
	m_listener(listener), m_wakeup_read(wakeup_read), m_status(status), m_logger(std::move(logger)) {}

	Expected<EventLoop::Event, ConnectionClosed> EventLoop::Wait(const SessionList& sessions) noexcept {
#ifdef UNIX
		std::vector<pollfd> descriptors;
		descriptors.reserve(2 + sessions.size());
		descriptors.push_back({ m_listener.Handle(), POLLIN, 0 });
		descriptors.push_back({ m_wakeup_read, POLLIN, 0 });
		for (const auto& session: sessions) {
			descriptors.push_back({ session->Handle(), static_cast<short>(session->HasPendingFrame() ? 0 : POLLIN), 0 });
		}
		const int result = poll(descriptors.data(), descriptors.size(), 1000);
		if (result < 0) {
			return Unexpected<ConnectionClosed>("Failed to wait for server events");
		}
		if (result == 0) {
			return Event{ EventKind::Timeout, nullptr };
		}
		if (descriptors[1].revents & POLLIN) {
			char signal;
			[[maybe_unused]] const ssize_t received = ::read(m_wakeup_read, &signal, sizeof(signal));
			return Event{ EventKind::Wakeup, nullptr };
		}
		if (descriptors[0].revents & POLLIN) {
			return Event{ EventKind::Listener, nullptr };
		}
		for (std::size_t index = 0; index < sessions.size(); ++index) {
			if (descriptors[index + 2].revents & (POLLIN | POLLERR | POLLHUP | POLLRDHUP)) {
				return Event{ EventKind::Session, sessions[index] };
			}
		}
		return Unexpected<ConnectionClosed>("Server reported an invalid event");
#else
		fd_set read_fds;
		FD_ZERO(&read_fds);
		FD_SET(m_listener.Handle(), &read_fds);
		FD_SET(m_wakeup_read, &read_fds);
		std::shared_ptr<Session> ready_session;
		for (const auto& session: sessions) {
			if (!session->HasPendingFrame()) {
				FD_SET(session->Handle(), &read_fds);
			}
		}
		timeval timeout{ .tv_sec = 1, .tv_usec = 0 };
		const int result = select(0, &read_fds, nullptr, nullptr, &timeout);
		if (result == SOCKET_ERROR) {
			return Unexpected<ConnectionClosed>("Failed to wait for server events");
		}
		if (result == 0) {
			return Event{ EventKind::Timeout, nullptr };
		}
		if (FD_ISSET(m_wakeup_read, &read_fds)) {
			char signal;
			(void)::recv(m_wakeup_read, &signal, sizeof(signal), 0);
			return Event{ EventKind::Wakeup, nullptr };
		}
		if (FD_ISSET(m_listener.Handle(), &read_fds)) {
			return Event{ EventKind::Listener, nullptr };
		}
		for (const auto& session: sessions) {
			if (!session->HasPendingFrame() && FD_ISSET(session->Handle(), &read_fds)) {
				ready_session = session;
				break;
			}
		}
		if (ready_session) {
			return Event{ EventKind::Session, std::move(ready_session) };
		}
		return Unexpected<ConnectionClosed>("Server reported an invalid event");
#endif
	}

	void EventLoop::Run(const ListenerCallback& on_listener_ready,
		const SessionSnapshot& snapshot,
		const SessionCallback& on_session_ready,
		const WakeupCallback& on_wakeup) noexcept {
		while (Connection::IsConnected(m_status.load(std::memory_order_acquire))) {
			auto wait_result = Wait(snapshot());
			if (!wait_result) {
				m_logger << Logger::Level::Error << wait_result.error()->what() << std::endl;
				return;
			}
			switch (wait_result->kind) {
				case EventKind::Listener:
					on_listener_ready();
					break;
				case EventKind::Session:
					on_session_ready(wait_result->session);
					break;
				case EventKind::Wakeup:
					if (Connection::IsConnected(m_status.load(std::memory_order_acquire))) {
						on_wakeup();
					}
					break;
				case EventKind::Timeout:
					break;
			}
		}
	}
}
