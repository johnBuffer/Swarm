#include <iostream>
#include "swarm.hpp"

#include <vector>
#include <ctime>



struct Compute
{
	Compute()
		: swarm(16)
	{
		uint32_t vec_size(100000000);
		
		vec1.resize(vec_size);
		vec2.resize(vec_size);

		for (uint32_t i(0); i < vec_size; ++i) {
			vec1[i] = (rand() % 10);
		}

	}

	void job(uint32_t id)
	{
		uint32_t data_size = vec1.size();
		uint32_t step = data_size / swarm.getWorkerCount();
		uint32_t start_index = id * step;
		uint32_t end_index = start_index + step;
		if (id == swarm.getWorkerCount() - 1) {
			end_index = data_size - 1;
		}

		uint32_t filter_width(64);

		for (uint32_t i(start_index); i < end_index; ++i) {
			float value = 0.0f;
			for (uint32_t k(0); k < filter_width; ++k) {
				if (i >= filter_width/2) {
					value += vec1[i + k - filter_width / 2];
				}
			}
			
			vec2[i] = value / float(filter_width);
		}
	}

	void sum()
	{
		swarm.executeJob([&](uint32_t id) {job(id); });
		swarm.waitJobDone();
	}

	Swarm swarm;
	std::vector<float> vec1;
	std::vector<float> vec2;
};

int32_t main()
{
	Compute c;

	std::cout << "START" << std::endl;
	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
	c.sum();
	std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
	std::cout << "END" << std::endl;


	std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);

	std::cout << "Elapsed time: " << time_span.count() << " ms" << std::endl;

	return 0;
}