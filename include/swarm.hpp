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

	void notifyAll()
	{
		cv.notify_all();
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


class Worker
{
	using WorkerJob = std::function<void(int32_t, int32_t)>;
	enum class State
	{
		Ready,
		Running,
		Done,
	};

	uint32_t    _id;
	uint32_t    _job_size;
	bool        _running = true;
	State       _state;
	SwarmState* _swarm_state;
	std::thread _thread;
	WorkerJob   _job;

	void start(SwarmState& swarm_state)
	{
		// Retrieve state
		_swarm_state = &swarm_state;
		// Initialize state and start thread
		_state = State::Ready;
		_thread = std::thread([this]() { run(); });
	}

	void run()
	{
		// Main loop
		while (_running) {
			ready();
			if (!_running) { return; }
			_job(_id, _job_size);
			done();
		}
	}

	void ready()
	{
		_swarm_state->counter_ready.add();
		_swarm_state->cv_ready.wait([this]() { return isRunning(); });
	}

	bool isRunning() const
	{
		return _state == State::Running || !_running;
	}

	void done()
	{
		_state = State::Done;
		// Increase the done counter
		_swarm_state->counter_done.add();
		// Notify the swarm manager to check done count
		_swarm_state->cv_swarm.cv.notify_one();
		// Wait for the others to finish
		_swarm_state->cv_done.wait([this]() { return _state == State::Ready; });
	}

	void stop()
	{
		_running = false;
	}

	friend class Swarm;
};


class Swarm
{
private:
	uint32_t            _workers_count;
	SwarmState          _state;
	std::vector<Worker> _workers;

public:
	Swarm(uint32_t workers_count)
		: _workers_count(workers_count)
		, _workers(workers_count)
	{
		initializeWorkers();
	}

	~Swarm()
	{
		stopWorkers();
		_state.cv_ready.cv.notify_all();
		for (Worker& w : _workers) {
			w._thread.join();
		}
	}

	void execute_async(Worker::WorkerJob f)
	{
		runJob(f);
	}

	void execute(Worker::WorkerJob f)
	{
		execute_async(f);
		waitForCompletion();
		freeWorkers();
	}

	void runJob(Worker::WorkerJob f)
	{
		_state.counter_done.reset();
		setWorkersState(Worker::State::Running);
		setWorkersJob(f);
		_state.cv_ready.notifyAll();
	}

	void waitForCompletion()
	{
		// This condition is here to ensure that we're not waiting forever
		// if all jobs are done
		if (_state.counter_done.count < _workers_count) {
			_state.cv_swarm.wait([&]() { return _state.counter_done.eq(_workers_count); });
		}
		freeWorkers();
	}

	void freeWorkers()
	{
		setWorkersState(Worker::State::Ready);
		_state.counter_ready.reset();
		_state.cv_done.notifyAll();
	}

private:
	void initializeWorkers()
	{
		uint32_t id = 0;
		for (Worker& w : _workers) {
			w._id = id++;
			w._job_size = static_cast<uint32_t>(_workers.size());
			w.start(_state);
		}
	}

	void stopWorkers()
	{
		for (Worker& w : _workers) {
			w.stop();
		}
	}

	void setWorkersState(Worker::State state)
	{
		for (Worker& w : _workers) {
			w._state = state;
		}
	}

	void setWorkersJob(Worker::WorkerJob job)
	{
		for (Worker& w : _workers) {
			w._job = job;
		}
	}
};

}
