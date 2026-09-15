/*
 * Copyright (C) 2024-2026 David C. Manuelda (StormBytePP)
 *
 * This file is part of StormByte-Network.
 */

#include <StormByte/network/worker_pool.hxx>

#include <algorithm>

namespace StormByte::Network::Detail {
	namespace {
		thread_local WorkerPool* current_pool = nullptr;
	}

	WorkerPool::WorkerPool(std::size_t worker_count, std::size_t queue_capacity,
		HandlerCallback handler, CompletionCallback on_completion):
	m_queue_capacity(queue_capacity), m_on_completion(std::move(on_completion)), m_handler(std::move(handler)) {
		if (worker_count == 0) {
			worker_count = 4;
		}

		m_workers.reserve(worker_count);
		for (std::size_t index = 0; index < worker_count; ++index) {
			m_workers.emplace_back(&WorkerPool::Run, this);
		}
	}

	WorkerPool::~WorkerPool() noexcept {
		Stop();
		Join();
	}

	void WorkerPool::Stop() noexcept {
		{
			std::scoped_lock lock(m_mutex);
			m_stopping = true;
		}

		m_condition.notify_all();
	}

	void WorkerPool::Join() noexcept {
		for (auto& worker: m_workers) {
			if (worker.joinable() && worker.get_id() != std::this_thread::get_id()) {
				worker.join();
			}
		}
	}

	bool WorkerPool::Submit(Task task) noexcept {
		std::scoped_lock lock(m_mutex);
		if (m_stopping || m_tasks.size() >= m_queue_capacity) {
			return false;
		}

		m_tasks.push_back(std::move(task));
		m_condition.notify_one();
		return true;
	}

	bool WorkerPool::HasCapacity() const noexcept {
		std::scoped_lock lock(m_mutex);
		return !m_stopping && m_tasks.size() < m_queue_capacity;
	}

	bool WorkerPool::IsWorkerThread() const noexcept {
		return current_pool == this;
	}

	void WorkerPool::Run() noexcept {
		current_pool = this;
		while (true) {
			Task task;
			{
				std::unique_lock lock(m_mutex);
				m_condition.wait(lock, [this]() { return m_stopping || !m_tasks.empty(); });
				if (m_tasks.empty()) {
					if (m_stopping) {
						break;
					}

					continue;
				}

				task = std::move(m_tasks.front());
				m_tasks.pop_front();
			}

			Completion completion{ task.uuid, nullptr, CompletionReason::Error };
			try {
				completion.packet = m_handler(task.uuid, std::move(task.packet));
				completion.reason = completion.packet ? CompletionReason::Success : CompletionReason::NullHandler;
			} catch (...) {
				completion.reason = CompletionReason::Error;
			}

			m_on_completion(Completion{ std::move(task.uuid), std::move(completion.packet), completion.reason });
		}

		current_pool = nullptr;
	}
}
