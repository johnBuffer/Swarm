#pragma once

#include <thread>
#include <mutex>
#include <list>
#include <functional>
#include <atomic>
#include <iostream>
#include <chrono>
#include <condition_variable>
#include <memory>

#include "worker.hpp"
#include "synchronizer.hpp"
#include "execution_group.hpp"
#include "write_synchronizer.hpp"

namespace swrm
{

class Swarm
{
public:
	Swarm(uint32_t thread_count)
		: m_thread_count(thread_count)
		, m_done_count(0U)
		, m_ready_count(0U)
	{
		for (uint32_t i(thread_count); i--;) {
			Worker* new_worker = new Worker(this);
			m_workers.push_back(new_worker);
			new_worker->lockReady();
			new_worker->createThread();
		}
	}

	~Swarm()
	{
		Synchronizer::stop(m_workers);
		Synchronizer::unlockAtReady(m_workers);
		Synchronizer::join(m_workers);
		for (Worker* worker : m_workers) {
			delete worker;
		}
	}

	uint32_t getWorkerCount() const {
		return m_thread_count;
	}

	WorkGroup execute(WorkerFunction job, uint32_t group_size)
	{
		if (group_size > m_available_workers.size()) {
			return WorkGroup();
		}

		std::shared_ptr<ExecutionGroup> group(std::make_unique<ExecutionGroup>(job, group_size, m_available_workers));

		return WorkGroup(group);
	}


private:
	const uint32_t m_thread_count;

	std::atomic<uint32_t> m_done_count;
	std::atomic<uint32_t> m_ready_count;
	std::list<Worker*>  m_workers;
	std::list<Worker*>  m_available_workers;
	std::mutex m_mutex;

	void notifyWorkerDone(Worker* worker)
	{
		++m_done_count;
	}

	void notifyWorkerReady(Worker* worker)
	{
		std::lock_guard<std::mutex> lg(m_mutex);
		++m_ready_count;
		m_available_workers.push_back(worker);
	}

	friend Worker;
};

}
