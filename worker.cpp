#include "worker.hpp"

#include "swarm.hpp"

namespace swrm
{

Worker::Worker()
	: m_swarm(nullptr)
	, m_job(nullptr)
{}

Worker::Worker(Swarm* swarm, uint32_t worker_id)
	: m_swarm(swarm)
	, id(worker_id)
	, running(true)
	, m_ready_mutex()
	, m_done_mutex()
{
}

void Worker::startThread()
{
	m_thread = std::thread(&Worker::run, this);
}

void Worker::run()
{
	while (true) {
		waitReady();

		if (!running) {
			break;
		}

		if (m_job) {
			m_job(id);
		}

		m_swarm->notifyWorkerDone();

		lockDone();
	}
}

void Worker::lock_ready()
{
	m_ready_mutex.lock();
}

void Worker::unlock_ready()
{
	m_ready_mutex.unlock();
}

void Worker::lockDone()
{
	m_done_mutex.lock();
}

void Worker::unlock_done()
{
	m_done_mutex.unlock();
}

void Worker::setJob(WorkerFunction job)
{
	m_job = job;
}

void Worker::stop()
{
	running = false;
}

void Worker::join()
{
	m_thread.join();
}

void Worker::waitReady()
{
	m_swarm->notifyReady();
	lock_ready();
	unlock_ready();
}

}
