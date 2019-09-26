#pragma once

#include <thread>
#include <mutex>
#include <list>
#include <functional>
#include <atomic>
#include <iostream>
#include <chrono>
#include <condition_variable>

#include "worker.hpp"


namespace swrm
{

class Swarm
{
public:
	Swarm(uint32_t thread_count)
		: m_thread_count(thread_count)
		, m_done_count(0U)
		, m_waiting_others(thread_count)
		, m_condition()
		, m_condition_mutex()
	{
		for (uint32_t i(0); i < thread_count; ++i) {
			std::cout << "Created Worker[" << i << "]" << std::endl;
			m_workers.emplace_back(this, i);
		}

		lockAllAtReady();

		for (Worker& w : m_workers) {
			w.startThread();
		}
	}

	~Swarm()
	{
		for (Worker& w : m_workers) {
			w.stop();
		}
		unlockAllAtReady();
		for (Worker& w : m_workers) {
			w.join();
		}
	}

	uint32_t getWorkerCount() const {
		return m_thread_count;
	}

	void execute(WorkerFunction function)
	{
		for (Worker& worker : m_workers) {
			worker.setJob(function);
		}

		start();
	}

	void waitExecutionDone()
	{
		std::unique_lock<std::mutex> ul(m_condition_mutex);
		m_condition.wait(ul, [&]{ return m_done_count == m_thread_count; });

		lockAllAtReady();
		unlockAllAtDone();
	}

private:
	const uint32_t m_thread_count;

	std::atomic<uint32_t> m_done_count;
	std::atomic<uint32_t> m_waiting_others;
	std::condition_variable m_condition;
	std::mutex m_condition_mutex;
	std::list<Worker> m_workers;

	void lockAllAtReady()
	{
		for (Worker& worker : m_workers) {
			worker.lockReady();
		}
	}

	void unlockAllAtReady()
	{
		for (Worker& worker : m_workers) {
			worker.unlockReady();
		}
	}

	void lockAllAtDone()
	{
		for (Worker& worker : m_workers) {
			worker.lockDone();
		}
	}

	void unlockAllAtDone()
	{
		for (Worker& worker : m_workers) {
			worker.unlockDone();
		}
	}

	void start()
	{
		m_done_count = 0;
		while (m_waiting_others) {}
		lockAllAtDone();
		unlockAllAtReady();
	}

	void notifyWorkerDone()
	{
		{
			std::lock_guard<std::mutex> lg(m_condition_mutex);
			++m_done_count;
		}
		m_condition.notify_one();
	}

	void notifyReady()
	{
		--m_waiting_others;
	}

	friend Worker;
};

}
