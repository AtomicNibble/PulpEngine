#include "stdafx.h"

#include <Util\ReferenceCounted.h>
#include <Util\ReferenceCountedOwner.h>

X_USING_NAMESPACE;

using namespace core;

namespace
{
    struct RefCountedTest : public ReferenceCounted<int32_t>
    {
    public:
        static size_t CONSTRUCTOR_COUNT;

    public:
        RefCountedTest()
        {
            CONSTRUCTOR_COUNT++;
        }
        ~RefCountedTest()
        {
            CONSTRUCTOR_COUNT--;
        }

    private:
    };

    size_t RefCountedTest::CONSTRUCTOR_COUNT = 0;
} // namespace

TEST(RefCounted, ArenaObject)
{
    {
        // starts with ref count 1.
        RefCountedTest* obj = X_NEW(RefCountedTest, g_arena, "RefrencedObj");

        ASSERT_EQ(1, RefCountedTest::CONSTRUCTOR_COUNT);

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

        // check assignment
        {
            OwnerType otherOwner = owner;
            otherOwner = owner;
        }

        // check move assignment
        {
            OwnerType otherOwner = owner;
            otherOwner = std::move(owner);
        }

        // back to one.
    }

    // should of been deleted.
    ASSERT_EQ(0, RefCountedTest::CONSTRUCTOR_COUNT);
}
