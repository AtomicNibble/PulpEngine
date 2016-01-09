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

	struct CustomType
	{
		static int CONSTRUCTION_COUNT;
		static int DECONSTRUCTION_COUNT;

		CustomType() : val_(0) {
			CONSTRUCTION_COUNT++;
		}
		CustomType(int val) : val_(val) {
			CONSTRUCTION_COUNT++;
		}
		CustomType(const CustomType& oth) : val_(oth.val_) {
			CONSTRUCTION_COUNT++;
		}
		~CustomType() {
			DECONSTRUCTION_COUNT++;
		}

		CustomType& operator += (const CustomType& oth) {
			val_ += oth.val_;
			return *this;
		}

		operator int() const { return val_; }
	private:
		int val_;
	};

	int CustomType::CONSTRUCTION_COUNT = 0;
	int CustomType::DECONSTRUCTION_COUNT = 0;
}

typedef ::testing::Types<short, int, float, CustomType> MyTypes;
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
	ASSERT_EQ(3, fifo.capacity());

	fifo.push(32);
	EXPECT_EQ(2, fifo.size());
	ASSERT_EQ(3, fifo.capacity());

	fifo.push(48);
	EXPECT_EQ(3, fifo.size());
	ASSERT_EQ(3, fifo.capacity());

	// Pop

	EXPECT_EQ(16, fifo.peek());
	fifo.pop();
	EXPECT_EQ(2, fifo.size());
	ASSERT_EQ(3, fifo.capacity());


	EXPECT_EQ(32, fifo.peek());
	fifo.pop();
	EXPECT_EQ(1, fifo.size());
	ASSERT_EQ(3, fifo.capacity());

	EXPECT_EQ(48, fifo.peek());
	fifo.pop();
	EXPECT_EQ(0, fifo.size());
	ASSERT_EQ(3, fifo.capacity());

	// push again so we over wright.
	fifo.push(64);
	EXPECT_EQ(1, fifo.size());
	ASSERT_EQ(3, fifo.capacity());

	EXPECT_EQ(64, fifo.peek());
	fifo.clear();
	EXPECT_EQ(0, fifo.size());
	ASSERT_EQ(3, fifo.capacity());

	fifo.free();

	EXPECT_EQ(0, fifo.size());
	ASSERT_EQ(0, fifo.capacity());

	// only matters on custom type.
	EXPECT_EQ(CustomType::CONSTRUCTION_COUNT, CustomType::DECONSTRUCTION_COUNT);
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

		for (int i = 0; i < 64; i++)
			fifo.push((TypeParam)(64 + i));

		fifo.clear();
	}

	// only matters on custom type.
	EXPECT_EQ(CustomType::CONSTRUCTION_COUNT, CustomType::DECONSTRUCTION_COUNT);
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
	CustomType::CONSTRUCTION_COUNT = 0;
	CustomType::DECONSTRUCTION_COUNT = 0;

	// check for reserver not constructing
	{
		Fifo<TypeParam> fifo(g_arena);

		fifo.reserve(3);
	}

	EXPECT_EQ(0, CustomType::DECONSTRUCTION_COUNT);
	EXPECT_EQ(CustomType::CONSTRUCTION_COUNT, CustomType::DECONSTRUCTION_COUNT);
}

TYPED_TEST(FifoTest, CleanUp)
{
	CustomType::CONSTRUCTION_COUNT = 0;
	CustomType::DECONSTRUCTION_COUNT = 0;

	// check for double deconstruction.
	{
		Fifo<TypeParam> fifo(g_arena);

		fifo.reserve(3);

		EXPECT_EQ(0, fifo.size());
		ASSERT_EQ(3, fifo.capacity());

		fifo.push(16);
		fifo.push(32);
	}

	EXPECT_EQ(CustomType::CONSTRUCTION_COUNT, CustomType::DECONSTRUCTION_COUNT);
}