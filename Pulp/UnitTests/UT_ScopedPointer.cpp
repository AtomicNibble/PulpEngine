#include "stdafx.h"



#include <EngineCommon.h>
#include <Util\ScopedPointer.h>

X_USING_NAMESPACE;

using namespace core;


TEST(ScopedPointer, Align)
{
	ScopedPointer<uint8_t[]> scoped_array(X_NEW_ARRAY(uint8_t, 10, gEnv->pArena, "TestScopedArray"), gEnv->pArena);
	
	scoped_array.get()[5] = 4;

	ScopedPointer<uint8_t> scoped_obj(X_NEW(uint8_t, gEnv->pArena, "TestScopedObj"), gEnv->pArena);

	*scoped_obj = 6;
}