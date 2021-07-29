#include <vector>
#include <thread>
#include <mutex>
#include <functional>


namespace swm
{


struct Barrier
{
	std::mutex mutex;
	std::condition_variable cv;

	void wait(std::function<bool(void)> predicate)
	{
		std::unique_lock<std::mutex> lock(mutex);
		cv.wait(lock, predicate);
	}
};


struct Counter
{
	std::mutex mutex;
	uint32_t count = 0;

	void reset()
	{
		count = 0;
	}

	bool eq(uint32_t value) const
	{
		return count == value;
	}

	void add()
	{
		std::lock_guard<std::mutex> lock(mutex);
		++count;
	}
};


struct SwarmState
{
	Counter counter_done;
	Counter counter_ready;

	Barrier cv_swarm;
	Barrier cv_ready;
	Barrier cv_done;
};


struct Worker
{
	using WorkerFunction = std::function<void(int32_t, int32_t)>;
	enum class State
	{
		Ready,
		Running,
		Done,
	};

	uint32_t id;
	uint32_t job_size;

	bool running = true;
	State state;
	
	SwarmState* swarm_state;
	WorkerFunction job;

	std::thread thread;

	void start(SwarmState& swm_state)
	{
		// Retrieve state
		swarm_state = &swm_state;
		state = State::Ready;
		thread = std::thread([this]() { run(); });
	}

	void run()
	{
		// Main loop
		while (running) {
			ready();
			if (!running) {
				return;
			}
			job(id, job_size);
			done();
		}
	}

	void ready()
	{
		swarm_state->counter_ready.add();
		swarm_state->cv_ready.wait([this]() { return state == State::Running || !running; });
	}

	void done()
	{
		state = State::Done;
		// Increase the done counter
		swarm_state->counter_done.add();
		// Notify the swarm manager to check done count
		swarm_state->cv_swarm.cv.notify_one();
		// Wait for the others to finish
		swarm_state->cv_done.wait([this]() { return state == State::Ready; });
	}

	void stop()
	{
		running = false;
	}
};


class Swarm
{
private:
	SwarmState state;
	std::vector<Worker> workers;

public:
	Swarm(uint32_t threads_count)
		: workers(threads_count)
	{
		initializeWorkers();
	}

	~Swarm()
	{
		stopWorkers();
		state.cv_ready.cv.notify_all();
		for (Worker& w : workers) {
			w.thread.join();
		}
	}

	void execute(Worker::WorkerFunction f)
	{
		const uint32_t workers_count = static_cast<uint32_t>(workers.size());
		state.counter_done.reset();
		setWorkersState(Worker::State::Running);
		setWorkersJob(f);
		state.cv_ready.cv.notify_all();
		if (state.counter_done.count < workers_count) {
			state.cv_swarm.wait([&]() { return state.counter_done.eq(workers_count); });
		}
		setWorkersState(Worker::State::Ready);
		state.counter_ready.reset();
		state.cv_done.cv.notify_all();
	}

private:
	void initializeWorkers()
	{
		uint32_t id = 0;
		for (Worker& w : workers) {
			w.id = id++;
			w.job_size = static_cast<uint32_t>(workers.size());
			w.start(state);
		}
	}

	void stopWorkers()
	{
		for (Worker& w : workers) {
			w.stop();
		}
	}

	void foreachWorker(std::function<void(Worker&)> callback)
	{
		for (Worker& w : workers) {
			callback(w);
		}
	}

	void setWorkersState(Worker::State state)
	{
		for (Worker& w : workers) {
			w.state = state;
		}
	}

	void setWorkersJob(Worker::WorkerFunction job)
	{
		for (Worker& w : workers) {
			w.job = job;
		}
	}
};

}
