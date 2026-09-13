/*
 * Copyright (C) 2024-2026 David C. Manuelda (StormBytePP)
 *
 * This file is part of StormByte-Network.
 */

#pragma once

#include <StormByte/network/typedefs.hxx>
#include <StormByte/network/visibility.h>

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace StormByte::Network::Detail {
	/**
	 * @class WorkerPool
	 * @brief Bounded private executor for packet handlers.
	 */
	class STORMBYTE_NETWORK_PRIVATE WorkerPool final {
		public:
			enum class CompletionReason: unsigned short { Success, NullHandler, Error }; ///< Handler outcome.

			struct Task {
				std::string uuid; ///< Client UUID.
				PacketPointer packet; ///< Request packet.
			};

			struct Completion {
				std::string uuid; ///< Client UUID.
				PacketPointer packet; ///< Response packet, or null.
				CompletionReason reason; ///< Handler outcome.
			};

			using CompletionCallback = std::function<void(Completion)>; ///< Completion sink.
			using HandlerCallback = std::function<PacketPointer(const std::string&, PacketPointer)>; ///< Packet handler.

			/**
			 * @brief Starts a bounded worker pool.
			 * @param worker_count Number of workers.
			 * @param queue_capacity Maximum queued tasks.
			 * @param on_completion Completion sink.
			 */
			WorkerPool(std::size_t worker_count, std::size_t queue_capacity,
				HandlerCallback handler, CompletionCallback on_completion);

			/** @brief Stops accepting tasks. */
			void Stop() noexcept;

			/** @brief Joins all workers. */
			void Join() noexcept;

			/**
			 * @brief Attempts to enqueue one task without blocking.
			 * @param task Task to execute.
			 * @return true when queued.
			 */
			bool Submit(Task task) noexcept;

			/** @brief Whether another task can be accepted without blocking. */
			bool HasCapacity() const noexcept;

			/**
			 * @brief Whether the calling thread belongs to this pool.
			 * @return true for a pool worker.
			 */
			bool IsWorkerThread() const noexcept;

			/** @brief Destructor stops and joins workers. */
			~WorkerPool() noexcept;

		private:
			void Run() noexcept;

			const std::size_t m_queue_capacity; ///< Maximum queued tasks.
			CompletionCallback m_on_completion; ///< Completion sink.
			HandlerCallback m_handler; ///< Packet handler.
			mutable std::mutex m_mutex; ///< Task queue lock.
			std::condition_variable m_condition; ///< Worker wakeup.
			std::deque<Task> m_tasks; ///< Bounded task queue.
			std::vector<std::thread> m_workers; ///< Worker threads.
			bool m_stopping = false; ///< Stop flag.
	};
}
