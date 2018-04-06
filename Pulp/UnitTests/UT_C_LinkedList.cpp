#include "stdafx.h"

#include <Containers\LinkedListIntrusive.h>
#include <Containers\LinkedList.h>

#include <Random\MultiplyWithCarry.h>

X_USING_NAMESPACE;

using namespace core;

namespace
{
    struct Data
    {
        INTRUSIVE_LIST_LINK(Data)
        forward;
        INTRUSIVE_LIST_LINK(Data)
        reverse;
        unsigned value;
    };

    struct MyData
    {
        int value;
        XLinkedList<MyData> node;
    };

    static void InsertIntoBefore(INTRUSIVE_LIST_PTR(Data) list, Data* data)
    {
        Data* before = list->head();
        while (before) {
            if (before->value >= data->value)
                break;
            before = list->next(before);
        }
        list->insertBefore(data, before);
    }

    static void InsertIntoAfter(INTRUSIVE_LIST_PTR(Data) list, Data* data)
    {
        Data* after = list->tail();
        while (after) {
            if (after->value <= data->value)
                break;
            after = list->prev(after);
        }
        list->insertAfter(data, after);
    }
} // namespace

TEST(LinkedList, Intrusive)
{
    INTRUSIVE_LIST_DECLARE(Data, forward)
    forward;
    INTRUSIVE_LIST_DECLARE(Data, reverse)
    reverse;

    for (unsigned j = 0; j < 100; ++j) {
        const unsigned COUNT = 10;
        ASSERT_TRUE(forward.isEmpty());
        ASSERT_TRUE(reverse.isEmpty());
        Data* last = NULL;
        for (unsigned i = 0; i < COUNT; ++i) {
            Data* data = X_NEW(Data, g_arena, "LinkedListtestData");
            data->value = i;
            forward.insertTail(data);
            reverse.insertHead(data);
            ASSERT_EQ(forward.prev(data), last);
            ASSERT_EQ(reverse.next(data), last);
            last = data;
        }
        ASSERT_FALSE(forward.isEmpty());
        ASSERT_FALSE(reverse.isEmpty());

        Data* f = forward.head();
        Data* r = reverse.head();
        for (unsigned i = 0; i < COUNT; ++i) {
            EXPECT_EQ(f->value, i);
            EXPECT_EQ(r->value, COUNT - 1 - i);
            EXPECT_EQ(f->forward.next(), forward.next(f));
            EXPECT_EQ(r->reverse.next(), reverse.next(r));
            f = f->forward.next();
            r = r->reverse.next();
        }

        ASSERT_TRUE(!f);
        ASSERT_TRUE(!r);

        f = forward.tail();
        r = reverse.tail();
        for (unsigned i = 0; i < COUNT; ++i) {
            EXPECT_EQ(f->value, COUNT - 1 - i);
            EXPECT_EQ(r->value, i);
            EXPECT_EQ(f->forward.prev(), forward.prev(f));
            EXPECT_EQ(r->reverse.prev(), reverse.prev(r));
            f = f->forward.prev();
            r = r->reverse.prev();
        }
        ASSERT_TRUE(!f);
        ASSERT_TRUE(!r);

        forward.unlinkAll();
        EXPECT_TRUE(forward.isEmpty());

        EXPECT_TRUE(!reverse.isEmpty());
        reverse.deleteAll(g_arena);
        EXPECT_TRUE(reverse.isEmpty());
    }
}

TEST(LinkedList, RandomInsert)
{
    for (unsigned j = 0; j < 1000; ++j) {
        INTRUSIVE_LIST_DECLARE(Data, forward)
        forward;
        INTRUSIVE_LIST_DECLARE(Data, reverse)
        reverse;

        // Insert random items sequentially
        for (unsigned i = 0; i < 20; ++i) {
            Data* data = X_NEW(Data, g_arena, "LinkedListtestData");
            data->value = gEnv->xorShift.rand();
            InsertIntoBefore(&forward, data);
            InsertIntoAfter(&reverse, data);
        }

        // Ensure all items were inserted in order
        for (const Data* f = forward.head(); const Data* next = forward.next(f); f = next)
            EXPECT_LE(f->value, next->value);
        for (const Data* r = reverse.tail(); const Data* prev = reverse.prev(r); r = prev)
            EXPECT_GE(r->value, prev->value);

        // Cleanup
        forward.deleteAll(g_arena);
        EXPECT_TRUE(reverse.isEmpty());
    }
}

TEST(LinkedList, Normal)
{
    XLinkedList<MyData> list;

    EXPECT_EQ(0, list.num());
    EXPECT_TRUE(list.isListEmpty());

    MyData data[10];
    int i;

    for (i = 0; i < 10; i++) {
        data[i].value = 1 << i;
        data[i].node.addToEnd(list);
        data[i].node.setOwner(&data[i]);
    }

    EXPECT_EQ(10, list.num());
    EXPECT_FALSE(list.isListEmpty());

    MyData* ent;
    for (i = 0, ent = list.next(); ent != nullptr; ent = ent->node.next(), i++) {
        EXPECT_EQ(1 << i, ent->value);
    }
}
