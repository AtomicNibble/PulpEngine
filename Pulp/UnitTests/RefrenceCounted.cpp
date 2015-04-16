#include "stdafx.h"

#include "gtest/gtest.h"


#include <Util\ReferenceCounted.h>


X_USING_NAMESPACE;

using namespace core;

namespace 
{

		struct RefCountedTest : public ReferenceCountedArena<RefCountedTest>
		{
		public:
			static size_t CONSTRUCTOR_COUNT = 0;

		public:
			RefCountedTest() {
				CONSTRUCTOR_COUNT++;
			}
			~RefCountedTest() {
				CONSTRUCTOR_COUNT--;
			}
		private:


		};


}


TEST(RefCounted, ArenaObject)
{
	{
		// starts with ref count 1.
		RefCountedTest* obj = X_NEW(RefCountedTest,g_arena, "RefrencedObj"); 

		typedef ReferenceCountedOwner<RefCountedTest> OwnerType;

		// saves the instance but keeps ref count at 1.
		OwnerType owner(obj, g_arena);

		{
			// obj ref count becomes 2
			OwnerType otherOwner(owner);
		}

		// obj ref count down to one
		{
			// obj ref count becomes 2
			OwnerType otherOwner = owner;
		}

		// back to one.
	}

	// should of been deleted.

}	

