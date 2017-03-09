#include "stdafx.h"

#include <Containers\Fifo.h>

#include <Containers\Array.h> // for move test.

#include <Memory\BoundsCheckingPolicies\NoBoundsChecking.h>
#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>
#include <Memory\AllocationPolicies\LinearAllocator.h>
#include <Memory\MemoryTaggingPolicies\NoMemoryTagging.h>

X_USING_NAMESPACE;

typedef core::MemoryArena<
	core::LinearAllocator,
	core::SingleThreadPolicy,
	core::NoBoundsChecking,
	core::NoMemoryTracking,
	core::NoMemoryTagging
> LinearArea;

using namespace core;

namespace {

	X_ALIGNED_SYMBOL(struct CustomTypeComplex, 64)
	{
		CustomTypeComplex(size_t val, const char* pName) :
			var_(val),
			pName_(pName)
		{
			CONSRUCTION_COUNT++;
		}
		CustomTypeComplex(const CustomTypeComplex& oth) :
			var_(oth.var_),
			pName_(oth.pName_)
		{
			++CONSRUCTION_COUNT;
		}
		CustomTypeComplex(CustomTypeComplex&& oth) :
			var_(oth.var_),
			pName_(oth.pName_)
		{
			++MOVE_COUNT;
		}

		~CustomTypeComplex() {
			DECONSRUCTION_COUNT++;
		}

		CustomTypeComplex& operator=(const CustomTypeComplex& val) {
			var_ = val.var_;
			return *this;
		}

		inline size_t GetVar(void) const {
			return var_;
		}
		inline const char* GetName(void) const {
			return pName_;
		}

	private:
		size_t var_;
		const char* pName_;


	public:
		static int MOVE_COUNT;
		static int CONSRUCTION_COUNT;
		static int DECONSRUCTION_COUNT;
	};

	int CustomTypeComplex::CONSRUCTION_COUNT = 0;
	int CustomTypeComplex::MOVE_COUNT = 0;
	int CustomTypeComplex::DECONSRUCTION_COUNT = 0;

}

typedef ::testing::Types<short, int32_t, int64_t, uint64_t, float, double> MyTypes;
TYPED_TEST_CASE(FifoTest, MyTypes);

template <typename T>
class FifoTest : public ::testing::Test {
public:
};

TYPED_TEST(FifoTest, Types)
{
	Fifo<TypeParam> fifo(g_arena);

	EXPECT_EQ(0, fifo.size());
	EXPECT_EQ(0, fifo.capacity());

	fifo.reserve(3);

	EXPECT_EQ(0, fifo.size());
	ASSERT_EQ(3, fifo.capacity());

	// Push

	fifo.push(16);
	EXPECT_EQ(1, fifo.size());
	EXPECT_LE(3, fifo.capacity());

	fifo.push(32);
	EXPECT_EQ(2, fifo.size());
	EXPECT_LE(3, fifo.capacity());

	fifo.push(48);
	EXPECT_EQ(3, fifo.size());
	EXPECT_LE(3, fifo.capacity());

	// Pop

	EXPECT_EQ(16, fifo.peek());
	fifo.pop();
	EXPECT_EQ(2, fifo.size());
	EXPECT_LE(3, fifo.capacity());


	EXPECT_EQ(32, fifo.peek());
	fifo.pop();
	EXPECT_EQ(1, fifo.size());
	EXPECT_LE(3, fifo.capacity());

	EXPECT_EQ(48, fifo.peek());
	fifo.pop();
	EXPECT_EQ(0, fifo.size());
	EXPECT_LE(3, fifo.capacity());

	// push again so we over wright.
	fifo.push(64);
	EXPECT_EQ(1, fifo.size());
	EXPECT_LE(3, fifo.capacity());

	EXPECT_EQ(64, fifo.peek());
	fifo.clear();
	EXPECT_EQ(0, fifo.size());
	EXPECT_LE(3, fifo.capacity());

	fifo.free();

	EXPECT_EQ(0, fifo.size());
	EXPECT_LE(0, fifo.capacity());
}

TYPED_TEST(FifoTest, CopyConstruct)
{
	// want the operator=(const T& oth) to be used.
	Fifo<TypeParam> fifo(g_arena);

	fifo.reserve(16);
	fifo.push(16);
	fifo.push(16);
	fifo.push(16);
	fifo.push(16);

	// assign
	Fifo<TypeParam> fifo2(fifo);

	EXPECT_EQ(4, fifo2.size());
	EXPECT_EQ(16, fifo2.capacity());

	// check the values are correct.
	for (Fifo<TypeParam>::iterator it = fifo2.begin(); it != fifo2.end(); ++it)
	{
		EXPECT_EQ(16, *it);
	}
}

TYPED_TEST(FifoTest, Iteration)
{
	{
		Fifo<TypeParam> fifo(g_arena);

		fifo.reserve(3);

		EXPECT_EQ(0, fifo.size());
		ASSERT_EQ(3, fifo.capacity());

		fifo.push(16);
		fifo.push(32);
		fifo.push(48);

		fifo.pop();
		fifo.push(128);

		int numvalues = 0;
		TypeParam valueSum = 0;

		for (Fifo<TypeParam>::iterator it = fifo.begin(); it != fifo.end(); ++it)
		{
			numvalues++;
			valueSum += (*it);
		}

		EXPECT_EQ(3, numvalues);
		EXPECT_EQ(208, valueSum);

		numvalues = 0;
		valueSum = 0;

		for (Fifo<TypeParam>::const_iterator it = fifo.begin(); it != fifo.end(); ++it)
		{
			numvalues++;
			valueSum += (*it);
		}

		EXPECT_EQ(3, numvalues);
		EXPECT_EQ(208, valueSum);

		fifo.clear();
	}

}


TYPED_TEST(FifoTest, Move)
{
	const size_t bytes = (sizeof(TypeParam) * 164) + (sizeof(Fifo<TypeParam>) * 2) + 
		(sizeof(size_t) * 6);

	X_ALIGNED_SYMBOL(char buf[bytes], 8) = {};
	LinearAllocator allocator(buf, buf + bytes);
	LinearArea arena(&allocator, "MoveAllocator");

	Array<Fifo<TypeParam>> list(&arena);
	list.setGranularity(2);
	list.reserve(2);

	list.push_back(Fifo<TypeParam>(&arena, 100));
	list.push_back(Fifo<TypeParam>(&arena, 64));
}

TYPED_TEST(FifoTest, Assign)
{
	// want the operator=(const T& oth) to be used.
	Fifo<TypeParam> fifo(g_arena);
	Fifo<TypeParam> fifo2(g_arena);

	fifo.reserve(16);
	fifo.push(16);
	fifo.push(16);
	fifo.push(16);
	fifo.push(16);

	fifo2.reserve(1);
	fifo2.push(0x71);

	EXPECT_EQ(1, fifo2.size());
	EXPECT_LE(1, fifo2.capacity());

	// assign
	fifo2 = fifo;

	EXPECT_EQ(4, fifo2.size());
	EXPECT_EQ(16, fifo2.capacity());

	// check the values are correct.
	for (Fifo<TypeParam>::iterator it = fifo2.begin(); it != fifo2.end(); ++it)
	{
		EXPECT_EQ(16, *it);
	}
}

TYPED_TEST(FifoTest, MoveAssign)
{
	const size_t bytes = (sizeof(TypeParam) * (16 + 64)) +
		(sizeof(size_t) * 4); // Linear header block.


	X_ALIGNED_SYMBOL(char buf[bytes], 8) = {};
	LinearAllocator allocator(buf, buf + bytes);
	LinearArea arena(&allocator, "MoveAllocator");

	// want the operator=(T&& oth) to be used.
	Fifo<TypeParam> fifo(&arena);

	fifo.reserve(16);
	fifo = Fifo<TypeParam>(&arena, 64);

	EXPECT_EQ(0, fifo.size());
	EXPECT_EQ(64, fifo.capacity());
}

TYPED_TEST(FifoTest, Reserver)
{
	// check for reserver not alloc
	Fifo<TypeParam> fifo(g_arena);

	EXPECT_EQ(0, fifo.size());
	ASSERT_EQ(0, fifo.capacity());

	fifo.reserve(3);

	EXPECT_EQ(0, fifo.size());
	ASSERT_EQ(3, fifo.capacity());
}

TYPED_TEST(FifoTest, CleanUp)
{
	Fifo<TypeParam> fifo(g_arena);

	fifo.reserve(3);

	EXPECT_EQ(0, fifo.size());
	ASSERT_EQ(3, fifo.capacity());

	fifo.push(16);
	fifo.push(32);

	EXPECT_EQ(2, fifo.size());
	ASSERT_EQ(3, fifo.capacity());

	fifo.clear();

	EXPECT_EQ(0, fifo.size());
	ASSERT_EQ(3, fifo.capacity());
}

TYPED_TEST(FifoTest, Free)
{
	Fifo<TypeParam> fifo(g_arena);

	fifo.reserve(3);

	EXPECT_EQ(0, fifo.size());
	ASSERT_EQ(3, fifo.capacity());

	fifo.push(16);
	fifo.push(32);

	EXPECT_EQ(2, fifo.size());
	ASSERT_EQ(3, fifo.capacity());

	fifo.free();

	EXPECT_EQ(0, fifo.size());
	ASSERT_EQ(0, fifo.capacity());
}

TYPED_TEST(FifoTest, CleanUp_construction)
{
	Fifo<TypeParam> fifo(g_arena, 3);

	EXPECT_EQ(0, fifo.size());
	ASSERT_EQ(3, fifo.capacity());

	fifo.push(16);
	fifo.push(32);

	EXPECT_EQ(2, fifo.size());
	ASSERT_EQ(3, fifo.capacity());

	fifo.clear();

	EXPECT_EQ(0, fifo.size());
	ASSERT_EQ(3, fifo.capacity());
}


TYPED_TEST(FifoTest, Free_construction)
{
	Fifo<TypeParam> fifo(g_arena, 3);

	EXPECT_EQ(0, fifo.size());
	ASSERT_EQ(3, fifo.capacity());

	fifo.push(16);
	fifo.push(32);

	EXPECT_EQ(2, fifo.size());
	ASSERT_EQ(3, fifo.capacity());

	fifo.free();

	EXPECT_EQ(0, fifo.size());
	ASSERT_EQ(0, fifo.capacity());
}

TYPED_TEST(FifoTest, growing)
{
	Fifo<TypeParam> fifo(g_arena);

	EXPECT_EQ(0, fifo.size());
	ASSERT_EQ(0, fifo.capacity());

	fifo.reserve(16);

	for (size_t i = 0; i < 256; i++) {
		fifo.push((TypeParam)i);
	}

	EXPECT_EQ(256, fifo.size());
	ASSERT_LE(256, fifo.capacity());

	fifo.free();

	EXPECT_EQ(0, fifo.size());
	ASSERT_EQ(0, fifo.capacity());
}

TYPED_TEST(FifoTest, growing2)
{
	Fifo<TypeParam> fifo(g_arena);

	EXPECT_EQ(0, fifo.size());
	ASSERT_EQ(0, fifo.capacity());

	fifo.reserve(4);

	EXPECT_EQ(0, fifo.size());
	ASSERT_EQ(4, fifo.capacity());

	fifo.push(0x10);
	fifo.push(0x20);
	fifo.pop();
	fifo.push(0x30);
	fifo.pop();
	fifo.push(0x40);
	fifo.push(0x50);

	// we should not of grown, size should be 3
	// and we should of moved around.
	EXPECT_EQ(3, fifo.size());
	ASSERT_EQ(4, fifo.capacity());

	for (size_t i = 0; i < 256; i++) {
		fifo.push((TypeParam)i);
	}

	EXPECT_EQ(256 + 3, fifo.size());
	ASSERT_LE(256, fifo.capacity());

	// pop them all.
	EXPECT_EQ(0x30, fifo.peek());
	fifo.pop();

	EXPECT_EQ(0x40, fifo.peek());
	fifo.pop();

	EXPECT_EQ(0x50, fifo.peek());
	fifo.pop();

	for (size_t i = 0; i < 256; i++) {
		EXPECT_EQ((TypeParam)i, fifo.peek());
		fifo.pop();
	}

	EXPECT_EQ(0, fifo.size());

	fifo.free();

	EXPECT_EQ(0, fifo.size());
	ASSERT_EQ(0, fifo.capacity());
}

TYPED_TEST(FifoTest, pushEmpty)
{
	Fifo<TypeParam> fifo(g_arena);

	EXPECT_EQ(0, fifo.size());
	ASSERT_EQ(0, fifo.capacity());

	fifo.push(0x10);
	fifo.push(0x20);
	fifo.pop();
	fifo.push(0x30);
	fifo.pop();
	fifo.push(0x40);
	fifo.push(0x50);

	// we should not of grown, size should be 3
	// and we should of moved around.
	EXPECT_EQ(3, fifo.size());
	ASSERT_LE(4, fifo.capacity());

	for (size_t i = 0; i < 256; i++) {
		fifo.push((TypeParam)i);
	}

	EXPECT_EQ(256 + 3, fifo.size());
	ASSERT_LE(256, fifo.capacity());

	// pop them all.
	EXPECT_EQ(0x30, fifo.peek());
	fifo.pop();

	EXPECT_EQ(0x40, fifo.peek());
	fifo.pop();

	EXPECT_EQ(0x50, fifo.peek());
	fifo.pop();

	for (size_t i = 0; i < 256; i++) {
		EXPECT_EQ((TypeParam)i, fifo.peek());
		fifo.pop();
	}

	EXPECT_EQ(0, fifo.size());

	fifo.free();

	EXPECT_EQ(0, fifo.size());
	ASSERT_EQ(0, fifo.capacity());
}


TEST(FifoTest, Complex_CopyConstruct)
{
	CustomTypeComplex::MOVE_COUNT = 0;
	CustomTypeComplex::CONSRUCTION_COUNT = 0;
	CustomTypeComplex::DECONSRUCTION_COUNT = 0;

	{
		// want the operator=(const T& oth) to be used.
		// checking construction and de-construction counts are correct.
		Fifo<CustomTypeComplex> fifo(g_arena);

		fifo.reserve(16);
		fifo.push(CustomTypeComplex(16, "meow"));
		fifo.push(CustomTypeComplex(16, "meow"));
		fifo.push(CustomTypeComplex(16, "meow"));
		fifo.push(CustomTypeComplex(16, "meow"));

		EXPECT_EQ(4, CustomTypeComplex::MOVE_COUNT);
		EXPECT_EQ(4, CustomTypeComplex::CONSRUCTION_COUNT);
		EXPECT_EQ(4, CustomTypeComplex::DECONSRUCTION_COUNT);

		// copy con
		Fifo<CustomTypeComplex> fifo2(fifo);

		// 4 copyies made.
		EXPECT_EQ(4, CustomTypeComplex::MOVE_COUNT);
		EXPECT_EQ(4 + 4, CustomTypeComplex::CONSRUCTION_COUNT);
		EXPECT_EQ(4, CustomTypeComplex::DECONSRUCTION_COUNT);

		EXPECT_EQ(4, fifo2.size());
		EXPECT_EQ(16, fifo2.capacity());

		// check the values are correct.
		for (Fifo<CustomTypeComplex>::iterator it = fifo2.begin(); it != fifo2.end(); ++it)
		{
			EXPECT_EQ(16, it->GetVar());
			EXPECT_STREQ("meow", it->GetName());
		}
	}

	EXPECT_EQ(4, CustomTypeComplex::MOVE_COUNT);
	EXPECT_EQ(4 + 4, CustomTypeComplex::CONSRUCTION_COUNT);
	EXPECT_EQ(4 + 8, CustomTypeComplex::DECONSRUCTION_COUNT);
}


TEST(FifoTest, Complex_Assign)
{
	CustomTypeComplex::MOVE_COUNT = 0;
	CustomTypeComplex::CONSRUCTION_COUNT = 0;
	CustomTypeComplex::DECONSRUCTION_COUNT = 0;

	{
		// want the operator=(const T& oth) to be used.
		// checking construction and de-construction counts are correct.
		Fifo<CustomTypeComplex> fifo(g_arena); 
		Fifo<CustomTypeComplex> fifo2(g_arena);

		fifo.reserve(16);
		fifo.push(CustomTypeComplex(16, "meow"));
		fifo.push(CustomTypeComplex(16, "meow"));
		fifo.push(CustomTypeComplex(16, "meow"));
		fifo.push(CustomTypeComplex(16, "meow"));

		EXPECT_EQ(4, CustomTypeComplex::MOVE_COUNT);
		EXPECT_EQ(4, CustomTypeComplex::CONSRUCTION_COUNT);
		EXPECT_EQ(4, CustomTypeComplex::DECONSRUCTION_COUNT);

		fifo2.reserve(2);
		fifo2.push(CustomTypeComplex(0x71, "meow"));

		EXPECT_EQ(5, CustomTypeComplex::MOVE_COUNT);
		EXPECT_EQ(5, CustomTypeComplex::CONSRUCTION_COUNT);
		EXPECT_EQ(5, CustomTypeComplex::DECONSRUCTION_COUNT);

		EXPECT_EQ(1, fifo2.size());
		EXPECT_LE(1, fifo2.capacity());

		// assign
		fifo2 = fifo;

		// the item in fifo should of been cleaned up.
		// and 4 copyies made.
		EXPECT_EQ(5, CustomTypeComplex::MOVE_COUNT);
		EXPECT_EQ(5 + 4, CustomTypeComplex::CONSRUCTION_COUNT);
		EXPECT_EQ(5 + 1, CustomTypeComplex::DECONSRUCTION_COUNT);

		EXPECT_EQ(4, fifo2.size());
		EXPECT_EQ(16, fifo2.capacity());

		// check the values are correct.
		for (Fifo<CustomTypeComplex>::iterator it = fifo2.begin(); it != fifo2.end(); ++it)
		{
			EXPECT_EQ(16, it->GetVar());
			EXPECT_STREQ("meow", it->GetName());
		}
	}

	EXPECT_EQ(5, CustomTypeComplex::MOVE_COUNT);
	EXPECT_EQ(5 + 4, CustomTypeComplex::CONSRUCTION_COUNT);
	EXPECT_EQ(5 + 1 + 8, CustomTypeComplex::DECONSRUCTION_COUNT);
}



TEST(FifoTest, Complex_Clear)
{
	CustomTypeComplex::MOVE_COUNT = 0;
	CustomTypeComplex::CONSRUCTION_COUNT = 0;
	CustomTypeComplex::DECONSRUCTION_COUNT = 0;

	{
		Fifo<CustomTypeComplex> fifo(g_arena);

		fifo.reserve(16);
		fifo.push(CustomTypeComplex(16, "meow"));
		fifo.push(CustomTypeComplex(16, "meow"));
		fifo.push(CustomTypeComplex(16, "meow"));
		fifo.push(CustomTypeComplex(16, "meow"));

		EXPECT_EQ(4, CustomTypeComplex::MOVE_COUNT);
		EXPECT_EQ(4, CustomTypeComplex::CONSRUCTION_COUNT);
		EXPECT_EQ(4, CustomTypeComplex::DECONSRUCTION_COUNT);

		EXPECT_EQ(4, fifo.size());
		EXPECT_EQ(16, fifo.capacity());

		fifo.clear();

		EXPECT_EQ(0, fifo.size());
		EXPECT_EQ(16, fifo.capacity());

		// check they all got deconstructed
		EXPECT_EQ(4, CustomTypeComplex::MOVE_COUNT);
		EXPECT_EQ(4, CustomTypeComplex::CONSRUCTION_COUNT);
		EXPECT_EQ(8, CustomTypeComplex::DECONSRUCTION_COUNT);
	}

	// checxk they don't get double deconstructed.
	EXPECT_EQ(4, CustomTypeComplex::MOVE_COUNT);
	EXPECT_EQ(4, CustomTypeComplex::CONSRUCTION_COUNT);
	EXPECT_EQ(8, CustomTypeComplex::DECONSRUCTION_COUNT);
}

TEST(FifoTest, Complex_Free)
{
	CustomTypeComplex::MOVE_COUNT = 0;
	CustomTypeComplex::CONSRUCTION_COUNT = 0;
	CustomTypeComplex::DECONSRUCTION_COUNT = 0;

	{
		Fifo<CustomTypeComplex> fifo(g_arena);

		fifo.reserve(16);
		fifo.push(CustomTypeComplex(16, "meow"));
		fifo.push(CustomTypeComplex(16, "meow"));
		fifo.push(CustomTypeComplex(16, "meow"));
		fifo.push(CustomTypeComplex(16, "meow"));

		EXPECT_EQ(4, CustomTypeComplex::MOVE_COUNT);
		EXPECT_EQ(4, CustomTypeComplex::CONSRUCTION_COUNT);
		EXPECT_EQ(4, CustomTypeComplex::DECONSRUCTION_COUNT);

		EXPECT_EQ(4, fifo.size());
		EXPECT_EQ(16, fifo.capacity());

		fifo.free();

		EXPECT_EQ(0, fifo.size());
		EXPECT_EQ(0, fifo.capacity());

		// check they all got deconstructed
		EXPECT_EQ(4, CustomTypeComplex::MOVE_COUNT);
		EXPECT_EQ(4, CustomTypeComplex::CONSRUCTION_COUNT);
		EXPECT_EQ(8, CustomTypeComplex::DECONSRUCTION_COUNT);
	}

	// checxk they don't get double deconstructed.
	EXPECT_EQ(4, CustomTypeComplex::MOVE_COUNT);
	EXPECT_EQ(4, CustomTypeComplex::CONSRUCTION_COUNT);
	EXPECT_EQ(8, CustomTypeComplex::DECONSRUCTION_COUNT);
}


TEST(FifoTest, Complex_push_move)
{
	CustomTypeComplex::MOVE_COUNT = 0;
	CustomTypeComplex::CONSRUCTION_COUNT = 0;
	CustomTypeComplex::DECONSRUCTION_COUNT = 0;

	{
		Fifo<CustomTypeComplex> fifo(g_arena);

		fifo.reserve(3);
		
		fifo.push(CustomTypeComplex(0x1414, "meow"));
		fifo.push(CustomTypeComplex(0x1414, "meow"));
	}

	EXPECT_EQ(2, CustomTypeComplex::MOVE_COUNT);
	EXPECT_EQ(2, CustomTypeComplex::CONSRUCTION_COUNT);
	EXPECT_EQ(4, CustomTypeComplex::DECONSRUCTION_COUNT);
}

TEST(FifoTest, Complex_emplace)
{
	CustomTypeComplex::MOVE_COUNT = 0;
	CustomTypeComplex::CONSRUCTION_COUNT = 0;
	CustomTypeComplex::DECONSRUCTION_COUNT = 0;

	{
		Fifo<CustomTypeComplex> fifo(g_arena);

		fifo.reserve(3);
		fifo.emplace(0x1414, "meow");
		fifo.emplace(0x1414, "meow");
	}

	EXPECT_EQ(0, CustomTypeComplex::MOVE_COUNT);
	EXPECT_EQ(2, CustomTypeComplex::CONSRUCTION_COUNT);
	EXPECT_EQ(2, CustomTypeComplex::DECONSRUCTION_COUNT);
}
