# Swarm

Multithreading lib

## Usage

### 1. Include header
```cpp
#include "swarm.hpp"
```

### 2. Create concurrent function
```cpp
#include <vector>

const size_t vec_size(100000000);

std::vector<float> v1(vec_size);
std::vector<float> v2(vec_size);
std::vector<float> v3(vec_size);

void job(uint32_t worker_id, uint32_t worker_count)
{
    // Number of values for which the thread is responsible
    const uint32_t step = vec_size / worker_count;
    // First value for the thread
    const uint32_t start_index = worker_id * step;
    // Last value
    const uint32_t end_index = (worker_id < worker_count - 1) ? start_index + step : vec_size - 1;
    // The actuat loop
    for (uint32_t i(start_index); i < end_index; ++i)
    {
        vec3[i] = vec1[i] + vec2[i]
    }
}
```

### 3. Create swarm and execute job
```cpp
// Create a Swarm object with 16 thread
const uint32_t thread_count(16);
swrm::Swarm swarm(thread_count);
swarm.execute(job);
```

### Putting all together
```cpp
#include "swarm.hpp"
#include <vector>

const size_t vec_size(100000000);

std::vector<float> v1(vec_size);
std::vector<float> v2(vec_size);
std::vector<float> v3(vec_size);

void job(uint32_t worker_id, uint32_t worker_count)
{
    // Number of values for which the thread is responsible
    const uint32_t step = vec_size / worker_count;
    // First value for the thread
    const uint32_t start_index = worker_id * step;
    // Last value
    const uint32_t end_index = (worker_id == worker_count - 1) ? start_index + step : vec_size - 1;
  
    // The actuat loop
    for (uint32_t i(start_index); i < end_index; ++i)
    {
        v3[i] = v1[i] + v2[i]
    }
}


int main()
{
    // Create a Swarm object with 16 thread
    const uint32_t thread_count(16);
    swrm::Swarm swarm(thread_count);
    // Start parallel job
    swarm.execute(job);
    // Wait for the job to terminate
    swarm.waitExecutionDone();
  
    return 0;
}
```

## Performance

1D mean filter on **100.000.000** values with a kernel width of **64** on an *i9-9900K*

|Thread count|Execution time|
|------------|--------------|
|1           |3.1 seconds   |
|16          |0.28 seconds  | 
