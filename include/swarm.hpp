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
#include <algorithm>

#include "worker.hpp"
#include "synchronizer.hpp"
#include "execution_group.hpp"
#include "write_synchronizer.hpp"

namespace swrm
{

// Handles workers
class Swarm
{
public:
	Swarm(uint32_t thread_count)
		: m_thread_count(thread_count)
		, m_ready_count(0U)
	{
		for (uint32_t i(thread_count); i--;) {
			createWorker();
		}

		while (m_ready_count < m_thread_count) {}
	}

	~Swarm()
	{
		Synchronizer::stop(m_workers);
		Synchronizer::unlockAtReady(m_workers);
		Synchronizer::join(m_workers);
		deleteWorkers();
	}

	WorkGroup execute(WorkerFunction job, uint32_t group_size)
	{
		const uint32_t available_threads(m_available_workers.size());
		
		std::shared_ptr<ExecutionGroup> group(std::make_shared<ExecutionGroup>(job, group_size, m_available_workers));
		uint32_t thread_to_add = std::min(group_size, available_threads);
		for (uint32_t i(thread_to_add); i--;) {
			group->addWorker(m_available_workers.front());
			m_available_workers.pop_front();
		}

		if (group_size > available_threads) {
			std::cout << "Group " << &(*group) << " is missing threads." << std::endl;
			m_waiting_threads.push_back(group);
		}

		return WorkGroup(group);
	}


private:
	const uint32_t m_thread_count;

	std::atomic<uint32_t> m_ready_count;
	std::list<Worker*>    m_workers;
	std::list<Worker*>    m_available_workers;
	std::list<std::shared_ptr<ExecutionGroup>> m_waiting_threads;
	std::mutex m_mutex;

	void createWorker()
	{
		Worker* new_worker = new Worker(this);
		new_worker->createThread();
		m_workers.push_back(new_worker);
	}

	void deleteWorkers()
	{
		for (Worker* worker : m_workers) {
			delete worker;
		}
	}

	void notifyWorkerReady(Worker* worker)
	{
		std::lock_guard<std::mutex> lg(m_mutex);
		++m_ready_count;
		m_available_workers.push_back(worker);
	}

	bool checkWaitingGroups(Worker* worker)
	{
		std::lock_guard<std::mutex> lg(m_mutex);
		if (m_waiting_threads.empty()) {
			return false;
		}

		std::shared_ptr<ExecutionGroup> group(m_waiting_threads.front());
		if (group->isNeedingThreads()) {
			std::cout << "Thread reaffected to " << &(*group) << std::endl;
			group->addWorker(worker);
		} else {
			m_waiting_threads.pop_front();
			return false;
		}

		return true;
	}

	friend Worker;
};

}
