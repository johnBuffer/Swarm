#include <thread>
#include <mutex>
#include <list>
#include <functional>
#include <atomic>
#include <iostream>
#include <chrono>
#include <condition_variable>

#include "synchronizer.hpp"

namespace swrm
{

class ExecutionGroup
{
public:
	ExecutionGroup(WorkerFunction job, uint32_t group_size, std::list<Worker*>& available_workers)
		: m_job(job)
		, m_group_size(group_size)
		, m_done_count(0U)
		, m_condition()
		, m_condition_mutex()
	{
		//retrieveWorkers(available_workers);
		//start();
	}

	~ExecutionGroup()
	{
	}

	void addWorker(Worker* worker)
	{
		m_workers.push_back(worker);
		worker->setJob(m_workers.size(), this);
		worker->lockDone();
		worker->unlockReady();
	}

	void start()
	{
		//m_done_count = 0U;
		//Synchronizer::lockAtDone(m_workers);
		//Synchronizer::unlockAtReady(m_workers);
	}

	uint32_t isNeedingThreads() const
	{
		return m_group_size > m_workers.size();
	}

	void waitExecutionDone()
	{
		waitWorkersDone();
	}

private:
	const uint32_t       m_group_size;
	const WorkerFunction m_job;
	std::list<Worker*>   m_workers;

	std::atomic<uint32_t>   m_done_count;
	std::condition_variable m_condition;
	std::mutex              m_condition_mutex;

	void notifyWorkerDone()
	{
		std::lock_guard<std::mutex> lg(m_condition_mutex);
		++m_done_count;
		m_condition.notify_one();
	}

	void waitWorkersDone()
	{
		std::unique_lock<std::mutex> ul(m_condition_mutex);
		m_condition.wait(ul, [&] { return m_done_count == m_group_size; });
		Synchronizer::lockAtReady(m_workers);
		Synchronizer::unlockAtDone(m_workers);
	}

	void retrieveWorkers(std::list<Worker*>& available_workers)
	{
		for (uint32_t i(m_group_size); i--;) {
			Worker* worker = available_workers.front();
			available_workers.pop_front();
			worker->setJob(i, this);
			m_workers.push_back(worker);
		}
	}

	/*bool releaseDoneWorker()
	{
		//std::cout << lol << " is done. TOTAL: " << m_done_count << std::endl;
		for (Worker* worker : m_workers) {
			if (worker->isDone()) {
				worker->lockReady();
				worker->unlockDone();
			}
		}

		m_workers.remove_if([&](Worker* w) {return w->isDone(); });
		std::cout << "Remaining " << m_workers.size() << std::endl;

		return m_workers.empty();
	}*/

	friend Worker;
};


class WorkGroup
{
public:
	WorkGroup()
		: m_group(nullptr)
	{}

	WorkGroup(std::shared_ptr<ExecutionGroup> execution_group)
		: m_group(execution_group)
	{}

	void waitExecutionDone()
	{
		if (m_group) {
			m_group->waitExecutionDone();
		} else {
			std::cout << "No threads" << std::endl;
		}
	}

private:
	std::shared_ptr<ExecutionGroup> m_group;
};

}