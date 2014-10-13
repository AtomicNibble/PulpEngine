#include "EngineCommon.h"


#include "SimpleMemoryTagging.h"

X_NAMESPACE_BEGIN(core)




const char* const SimpleMemoryTagging::TYPE_NAME = "MemoryTag";


void SimpleMemoryTagging::TagAllocation(void* memory, size_t size)
{
	 memset( memory, TAG_ALLOCATED, size) ;
}

void SimpleMemoryTagging::TagDeallocation(void* memory, size_t size)
{
	  memset( memory, TAG_FREED, size );
}





X_NAMESPACE_END