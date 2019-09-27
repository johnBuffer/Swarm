#pragma once

#include "worker.hpp"
#include <list>

namespace swrm
{

class ExecutionGroup
{
public:
	ExecutionGroup();

	void waitExecutionDone();

private:
	std::list<Worker*> m_group;


};

}

