#include "EngineCommon.h"

#include "SimpleMemoryTracking.h"


X_NAMESPACE_BEGIN(core)

const char* const SimpleMemoryTracking::TYPE_NAME = "SimpleTracking";

SimpleMemoryTracking::SimpleMemoryTracking(void) :
	numAllocations_(0)
{

}

SimpleMemoryTracking::~SimpleMemoryTracking(void)
{
	// o dear o.o
	X_ASSERT(numAllocations_ == 0, "Memory leaks detected. Num: %d", numAllocations_)();
}



X_NAMESPACE_END

