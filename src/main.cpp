#include <iostream>
#include "swarm.hpp"

#include <vector>
#include <ctime>


int32_t main()
{
	const uint32_t vec_size = 4;
	std::vector<uint32_t> res(4);

	swm::Swarm swarm(4);
	swarm.execute_async([&](uint32_t thread_id, uint32_t threads_count) {
		res[thread_id] = thread_id;
	});
	swarm.waitForCompletion();

	for (uint32_t i : res) {
		std::cout << i << ' ';
	}
	std::cout << std::endl;

	return 0;
}