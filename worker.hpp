#pragma once

#include <inttypes.h>
#include <thread>
#include <mutex>


class Swarm;

using WorkerFunction = std::function<void(uint32_t)>;


class Worker
{
public:
	Worker();
	Worker(Swarm* swarm, uint32_t worker_id);

	void startThread();

	void lock_ready();
	void lockDone();
	void unlock_ready();
	void unlock_done();

	void setJob(WorkerFunction job);
	void stop();
	void join();

private:
	bool running;
	uint32_t id;
	Swarm* m_swarm;
	std::thread m_thread;
	WorkerFunction m_job;

	std::mutex m_ready_mutex;
	std::mutex m_done_mutex;

	void run();
	void waitReady();
};
