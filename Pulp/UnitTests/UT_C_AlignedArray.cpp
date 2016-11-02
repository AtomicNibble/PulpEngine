#include "stdafx.h"

#include <IFileSys.h>

#include <Containers\Array.h>

#include <Memory\BoundsCheckingPolicies\NoBoundsChecking.h>
#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>
#include <Memory\AllocationPolicies\LinearAllocator.h>
#include <Memory\MemoryTaggingPolicies\NoMemoryTagging.h>

X_USING_NAMESPACE;

using namespace core;


namespace
{

	struct CustomType
	{
		CustomType()
			: var_(16)
		{
			CONSRUCTION_COUNT++;
		}
		CustomType(size_t val)
			: var_(val)
		{
			CONSRUCTION_COUNT++;
		}
		CustomType(const CustomType& oth)
			: var_(oth.var_)
		{
			++CONSRUCTION_COUNT;
		}
		CustomType(CustomType&& oth)
			: var_(oth.var_)
		{
			++CONSRUCTION_COUNT;
		}

		~CustomType() {
			DECONSRUCTION_COUNT++;
		}


		CustomType& operator=(const CustomType& val) {
			var_ = val.var_;
			return *this;
		}
		CustomType& operator=(size_t val) {
			var_ = val;
			return *this;
		}

		bool operator==(const CustomType& oth) const {
			return var_ == oth.var_;
		}
		bool operator==(const size_t& oth) const {
			return var_ == oth;
		}

		inline size_t GetVar(void) const {
			return var_;
		}
	private:
		size_t var_;

	public:
		static int CONSRUCTION_COUNT;
		static int DECONSRUCTION_COUNT;
	};


	X_INLINE bool operator==(const CustomType& a, const size_t& b) {
		return a.GetVar() == b;
	}
	X_INLINE bool operator==(const size_t& a, const CustomType& b) {
		return a == b.GetVar();
	}

	int CustomType::CONSRUCTION_COUNT = 0;
	int CustomType::DECONSRUCTION_COUNT = 0;


	struct CustomTypeComplex
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
		static int CONSRUCTION_COUNT;
		static int MOVE_COUNT;
		static int DECONSRUCTION_COUNT;
	};

	int CustomTypeComplex::CONSRUCTION_COUNT = 0;
	int CustomTypeComplex::MOVE_COUNT = 0;
	int CustomTypeComplex::DECONSRUCTION_COUNT = 0;
}

typedef core::MemoryArena<
	core::LinearAllocator,
	core::SingleThreadPolicy,
	core::NoBoundsChecking,
	core::NoMemoryTracking,
	core::NoMemoryTagging
> LinearArea;

typedef ::testing::Types<short, int, CustomType> MyTypes;
TYPED_TEST_CASE(AlignedArrayTest, MyTypes);

template <typename T>
class AlignedArrayTest : public ::testing::Test {
public:
};



TYPED_TEST(AlignedArrayTest, Contruct)
{
	Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>> list(g_arena);

	list.append(TypeParam());
	list.append(TypeParam());
	list.setGranularity(345);
	list.append(TypeParam());
	list.append(TypeParam());

	Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>> list2(list);

	EXPECT_EQ(4, list.size());
	EXPECT_EQ(4, list2.size());
	EXPECT_EQ(345, list.granularity());
	EXPECT_EQ(345, list2.granularity());

	list.free();

	EXPECT_EQ(0, list.size());
	EXPECT_EQ(4, list2.size());
	EXPECT_EQ(345, list.granularity());
	EXPECT_EQ(345, list2.granularity());
}

TYPED_TEST(AlignedArrayTest, Clear)
{
	CustomType::CONSRUCTION_COUNT = 0;
	CustomType::DECONSRUCTION_COUNT = 0;


	Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>> list(g_arena);

	list.setGranularity(128);

	EXPECT_EQ(128, list.granularity());

	// insert some items
	for (int i = 0; i < 64; i++)
	{
		EXPECT_EQ(i, list.insert(i, i * 2));
	}

	EXPECT_EQ(64, list.size());
	EXPECT_EQ(128, list.capacity());

	list.clear();

	EXPECT_EQ(0, list.size());
	EXPECT_EQ(128, list.capacity());
	EXPECT_EQ(CustomType::CONSRUCTION_COUNT, CustomType::DECONSRUCTION_COUNT);
}

TYPED_TEST(AlignedArrayTest, Free)
{
	CustomType::CONSRUCTION_COUNT = 0;
	CustomType::DECONSRUCTION_COUNT = 0;

	{
		Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>> list(g_arena);

		list.setGranularity(128);

		EXPECT_EQ(0, list.size());
		EXPECT_EQ(0, list.capacity());
		EXPECT_EQ(128, list.granularity());

		// insert some items
		for (int i = 0; i < 64; i++)
		{
			EXPECT_EQ(i, list.insert(i, i * 2));
		}

		EXPECT_EQ(64, list.size());
		EXPECT_EQ(128, list.capacity());

		list.free();

		EXPECT_EQ(0, list.size());
		EXPECT_EQ(0, list.capacity());
		EXPECT_EQ(nullptr, list.ptr());
	}

	EXPECT_EQ(CustomType::CONSRUCTION_COUNT, CustomType::DECONSRUCTION_COUNT);
}

TYPED_TEST(AlignedArrayTest, Move)
{
	// make a stack based arena that can't allocate multiple buffers.
	// meaning allocation will fail if the copy constructors are used.
	const size_t bytes = (sizeof(TypeParam) * (100 + 64)) + (sizeof(Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>>) * 2) +
		(sizeof(size_t) * 3); // Linear header block.

	CustomType::CONSRUCTION_COUNT = 0;
	CustomType::DECONSRUCTION_COUNT = 0;

	{
		X_ALIGNED_SYMBOL(char buf[bytes], 8) = {};
		LinearAllocator allocator(buf, buf + bytes);

		LinearArea arena(&allocator, "MoveAllocator");

		Array<Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>>> list(&arena);
		list.setGranularity(2);
		list.reserve(2);

		list.push_back(Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>>(&arena, 100, TypeParam()));
		list.push_back(Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>>(&arena, 64, TypeParam()));
	}

	EXPECT_EQ(CustomType::CONSRUCTION_COUNT, CustomType::DECONSRUCTION_COUNT);
}

TYPED_TEST(AlignedArrayTest, Append)
{
	Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>> list(g_arena);

	EXPECT_EQ(0, list.size());
	EXPECT_EQ(0, list.capacity());
	EXPECT_LT((Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>>::size_type)0, list.granularity()); // gran should be above 0.

	EXPECT_EQ(nullptr, list.ptr());

	list.reserve(64);

	EXPECT_EQ(0, list.size());
	ASSERT_EQ(64, list.capacity());
	EXPECT_NE(nullptr, list.ptr());

	for (int i = 0; i < 64; i++)
	{
		list.append(i * 4);
	}

	EXPECT_EQ(64, list.size());
	ASSERT_EQ(64, list.capacity());

	for (int i = 0; i < 64; i++)
	{
		EXPECT_EQ(i * 4, list[i]);
	}

	// test the memory block it gives us.
	Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>>::Type* pArr = list.ptr();
	for (int i = 0; i < 64; i++)
	{
		EXPECT_EQ(i * 4, pArr[i]);
	}

	list.clear();

	EXPECT_EQ(0, list.size());
	ASSERT_EQ(64, list.capacity());

	list.reserve(128);

	EXPECT_EQ(0, list.size());
	EXPECT_EQ(128, list.capacity());
	EXPECT_NE(nullptr, list.ptr());

	list.free();

	EXPECT_EQ(0, list.size());
	EXPECT_EQ(0, list.capacity());
	EXPECT_EQ(nullptr, list.ptr());
}

TYPED_TEST(AlignedArrayTest, AppendArr)
{
	// we now allow a array of same tpye to be appended.
	Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>> list(g_arena);

	EXPECT_EQ(0, list.size());
	EXPECT_EQ(0, list.capacity());
	EXPECT_LT((Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>>::size_type)0, list.granularity()); // gran should be above 0.

	EXPECT_EQ(nullptr, list.ptr());

	for (size_t i = 0; i < 39; i++)
	{
		list.append(static_cast<TypeParam>(i * 4));
	}

	EXPECT_EQ(39, list.size());

	Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>> list2(g_arena);
	list2.append(1337);

	EXPECT_EQ(1, list2.size());

	// apend the list.
	list.append(list2);

	// now 40
	EXPECT_EQ(40, list.size());

	// check the values are correct.
	EXPECT_EQ(1337, list[39]);
	for (int i = 0; i < 39; i++)
	{
		EXPECT_EQ(i * 4, list[i]);
	}

	// clear list 1 and make sure list 2 still valid.
	list2.clear();

	EXPECT_EQ(0, list2.size());
	EXPECT_EQ(40, list.size());

	// check again
	EXPECT_EQ(1337, list[39]);
	for (int i = 0; i < 39; i++)
	{
		EXPECT_EQ(i * 4, list[i]);
	}
}



TYPED_TEST(AlignedArrayTest, Insert)
{
	Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>> list(g_arena);

	// it should resize for us.
	for (int i = 0; i < 64; i++)
	{
		EXPECT_EQ(i, list.insert(i, i));
	}

	// check contents
	EXPECT_EQ(64, list.size());
	EXPECT_EQ(64, list.capacity());

	for (int i = 0; i < 64; i++)
	{
		EXPECT_EQ(i, list[i]);
	}

	list.free();

	EXPECT_EQ(0, list.size());
	EXPECT_EQ(0, list.capacity());
	EXPECT_EQ(nullptr, list.ptr());
}


TYPED_TEST(AlignedArrayTest, Remove)
{
	CustomType::CONSRUCTION_COUNT = 0;
	CustomType::DECONSRUCTION_COUNT = 0;

	{
		Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>> list(g_arena);

		list.setGranularity(128);

		EXPECT_EQ(128, list.granularity());

		// insert some items
		for (int i = 0; i < 64; i++)
		{
			EXPECT_EQ(i, list.insert(i, i * 2));
		}

		EXPECT_EQ(64, list.size());
		EXPECT_EQ(128, list.capacity());

		EXPECT_TRUE(list.removeIndex(4));
		EXPECT_TRUE(list.removeIndex(10));
		EXPECT_TRUE(list.removeIndex(16));
		EXPECT_TRUE(list.removeIndex(28));

		EXPECT_EQ(60, list.size());
		EXPECT_EQ(128, list.capacity());

		list.remove(20);
		list.remove(40);

		EXPECT_EQ(58, list.size());
		EXPECT_EQ(128, list.capacity());

		list.free();

		EXPECT_EQ(0, list.size());
		EXPECT_EQ(0, list.capacity());
		EXPECT_EQ(nullptr, list.ptr());
	}

	EXPECT_EQ(CustomType::CONSRUCTION_COUNT, CustomType::DECONSRUCTION_COUNT);
}


TYPED_TEST(AlignedArrayTest, Iterator)
{
	Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>> list(g_arena);

	// 64 items all with value of 128
	list.resize(64, 128);


	Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>>::Iterator it = list.begin();
	for (; it != list.end(); ++it)
	{
		EXPECT_EQ(128, *it);
		*it = 64;
	}

	Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>>::ConstIterator cit = list.begin();
	for (; cit != list.end(); ++cit)
	{
		EXPECT_EQ(64, *cit);
	}

}

TYPED_TEST(AlignedArrayTest, Serialize)
{
	if (!compileTime::IsPOD<TypeParam>::Value) {
		return;
	}

	Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>> list(g_arena);

	EXPECT_EQ(0, list.size());
	EXPECT_EQ(0, list.capacity());
	EXPECT_LT((Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>>::size_type)0, list.granularity()); // gran should be above 0.

	EXPECT_EQ(nullptr, list.ptr());

	list.reserve(64);

	EXPECT_EQ(0, list.size());
	ASSERT_EQ(64, list.capacity());
	EXPECT_NE(nullptr, list.ptr());

	for (int i = 0; i < 64; i++)
	{
		list.append(i * 4);
	}

	EXPECT_EQ(64, list.size());
	ASSERT_EQ(64, list.capacity());

	// create a file.
	ASSERT_TRUE(NULL != gEnv->pFileSys);
	IFileSys* pFileSys = gEnv->pFileSys;

	const char* TypeName = typeid(TypeParam).name();
	core::Path<char> fileName;

	fileName /= X_ENGINE_NAME;
	fileName /= "_ut_array_serialize_Type(";
	fileName /= TypeName;
	fileName /= ").ut_dat";

	XFile* file = pFileSys->openFile(fileName.c_str(),
		fileMode::WRITE | fileMode::READ | fileMode::RECREATE | fileMode::RANDOM_ACCESS);
	ASSERT_TRUE(NULL != file);
	if (file)
	{
		ASSERT_TRUE(list.SSave(file));

		// add data.
		for (int i = 0; i < 28; i++)
		{
			list.append(i * 3);
		}
		// diffrent data.
		for (int i = 0; i < 28; i++)
		{
			list[i] = i * 5;
		}

		file->seek(0, SeekMode::SET);

		ASSERT_TRUE(list.SLoad(file));
		EXPECT_EQ(64, list.size());
		ASSERT_EQ(64, list.capacity());

		for (int i = 0; i < 64; i++)
		{
			EXPECT_EQ(i * 4, list[i]);
		}

		ASSERT_EQ(0, file->remainingBytes());

		pFileSys->closeFile(file);
	}
}

TYPED_TEST(AlignedArrayTest, InitializerConstruct)
{
	Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>> list(g_arena, {
		static_cast<TypeParam>(4),
		static_cast<TypeParam>(8),
		static_cast<TypeParam>(1),
		static_cast<TypeParam>(2)
	});


	EXPECT_EQ(4, list.size());
	ASSERT_EQ(list.granularity(), list.capacity());

	EXPECT_EQ(4, list[0]);
	EXPECT_EQ(8, list[1]);
	EXPECT_EQ(1, list[2]);
	EXPECT_EQ(2, list[3]);
}

TYPED_TEST(AlignedArrayTest, InitializerAsing)
{
	Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>> list(g_arena);

	EXPECT_EQ(0, list.size());
	EXPECT_EQ(0, list.capacity());
	EXPECT_LT((static_cast<Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>>::size_type>(0)), list.granularity());

	EXPECT_EQ(nullptr, list.ptr());

	list = {
		static_cast<TypeParam>(4),
		static_cast<TypeParam>(8),
		static_cast<TypeParam>(1),
		static_cast<TypeParam>(2)
	};

	EXPECT_EQ(4, list.size());
	ASSERT_EQ(list.granularity(), list.capacity());

	EXPECT_EQ(4, list[0]);
	EXPECT_EQ(8, list[1]);
	EXPECT_EQ(1, list[2]);
	EXPECT_EQ(2, list[3]);

	list = {
		static_cast<TypeParam>(4),
		static_cast<TypeParam>(8),
		static_cast<TypeParam>(1),
		static_cast<TypeParam>(1),
		static_cast<TypeParam>(1),
		static_cast<TypeParam>(1),
		static_cast<TypeParam>(2)
	};

	EXPECT_EQ(7, list.size());
	ASSERT_EQ(list.granularity(), list.capacity());

	EXPECT_EQ(4, list[0]);
	EXPECT_EQ(8, list[1]);
	EXPECT_EQ(1, list[2]);
	EXPECT_EQ(1, list[3]);
	EXPECT_EQ(1, list[4]);
	EXPECT_EQ(1, list[5]);
	EXPECT_EQ(2, list[6]);
}


TYPED_TEST(AlignedArrayTest, EmplaceBack)
{
	Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>> list(g_arena);

	EXPECT_EQ(0, list.size());
	EXPECT_EQ(0, list.capacity());
	EXPECT_LT((static_cast<Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>>::size_type>(0)), list.granularity()); // gran should be above 0.

	EXPECT_EQ(nullptr, list.ptr());

	list.reserve(64);

	EXPECT_EQ(0, list.size());
	ASSERT_EQ(64, list.capacity());
	EXPECT_NE(nullptr, list.ptr());

	for (int i = 0; i < 64; i++)
	{
		list.emplace_back(static_cast<Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>>::Type>(i * 4));
	}

	EXPECT_EQ(64, list.size());
	ASSERT_EQ(64, list.capacity());

	for (int i = 0; i < 64; i++)
	{
		EXPECT_EQ(i * 4, list[i]);
	}

	// test the memory block it gives us.
	Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>>::Type* pArr = list.ptr();
	for (int i = 0; i < 64; i++)
	{
		EXPECT_EQ(i * 4, pArr[i]);
	}

	list.clear();

	EXPECT_EQ(0, list.size());
	ASSERT_EQ(64, list.capacity());

	list.reserve(128);

	EXPECT_EQ(0, list.size());
	EXPECT_EQ(128, list.capacity());
	EXPECT_NE(nullptr, list.ptr());

	list.free();

	EXPECT_EQ(0, list.size());
	EXPECT_EQ(0, list.capacity());
	EXPECT_EQ(nullptr, list.ptr());
}

TEST(AlignedArrayTest, EmplaceBackComplex)
{
	Array<CustomTypeComplex, core::ArrayAlignedAllocator<CustomTypeComplex>> list(g_arena);

	EXPECT_EQ(0, list.size());
	EXPECT_EQ(0, list.capacity());
	EXPECT_LT((static_cast<Array<CustomTypeComplex, core::ArrayAlignedAllocator<CustomTypeComplex>>::size_type>(0)), list.granularity()); // gran should be above 0.

	EXPECT_EQ(nullptr, list.ptr());

	list.reserve(64);

	EXPECT_EQ(0, list.size());
	ASSERT_EQ(64, list.capacity());
	EXPECT_NE(nullptr, list.ptr());

	EXPECT_EQ(0, CustomTypeComplex::CONSRUCTION_COUNT);
	EXPECT_EQ(0, CustomTypeComplex::MOVE_COUNT);
	EXPECT_EQ(0, CustomTypeComplex::DECONSRUCTION_COUNT);


	for (int i = 0; i < 32; i++)
	{
		list.emplace_back(i * 4, "HEllo");
	}

	EXPECT_EQ(32, CustomTypeComplex::CONSRUCTION_COUNT);
	EXPECT_EQ(0, CustomTypeComplex::MOVE_COUNT);
	EXPECT_EQ(0, CustomTypeComplex::DECONSRUCTION_COUNT);

	for (int i = 32; i < 64; i++)
	{
		list.push_back(CustomTypeComplex(i * 4, "HEllo"));
	}

	EXPECT_EQ(64, CustomTypeComplex::CONSRUCTION_COUNT);
	EXPECT_EQ(32, CustomTypeComplex::MOVE_COUNT);
	EXPECT_EQ(32, CustomTypeComplex::DECONSRUCTION_COUNT);


	EXPECT_EQ(64, list.size());
	ASSERT_EQ(64, list.capacity());

	for (int i = 0; i < 64; i++)
	{
		EXPECT_EQ(i * 4, list[i].GetVar());
		EXPECT_STREQ("HEllo", list[i].GetName());
	}


	list.clear();

	EXPECT_EQ(0, list.size());
	ASSERT_EQ(64, list.capacity());

	list.reserve(128);

	EXPECT_EQ(0, list.size());
	EXPECT_EQ(128, list.capacity());
	EXPECT_NE(nullptr, list.ptr());

	list.free();

	EXPECT_EQ(0, list.size());
	EXPECT_EQ(0, list.capacity());
	EXPECT_EQ(nullptr, list.ptr());
}



TYPED_TEST(AlignedArrayTest, front)
{
	Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>> array(g_arena);

	array.append(static_cast<TypeParam>(5));
	array.append(static_cast<TypeParam>(6));
	EXPECT_EQ(5, array.front());

	Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>>::ConstReference constRef = array.front();

	EXPECT_EQ(5, constRef);
}

TYPED_TEST(AlignedArrayTest, back)
{
	Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>> array(g_arena);

	array.append(static_cast<TypeParam>(5));
	array.append(static_cast<TypeParam>(6));
	EXPECT_EQ(6, array.back());

	Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>>::ConstReference constRef = array.back();
	EXPECT_EQ(6, constRef);
}

X_PRAGMA(optimize("", off))

TYPED_TEST(AlignedArrayTest, front_fail)
{
	Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>> array(g_arena);
	const Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>> const_array(g_arena);

	core::debugging::EnableBreakpoints(false);
	g_AssetChecker.ExpectAssertion(true);

	// should throw assert.
	array.front();
	EXPECT_TRUE(g_AssetChecker.HadCorrectAssertions());

	g_AssetChecker.ExpectAssertion(true);

	const_array.front();
	EXPECT_TRUE(g_AssetChecker.HadCorrectAssertions());

	g_AssetChecker.ExpectAssertion(false);
	core::debugging::EnableBreakpoints(true);
}

TYPED_TEST(AlignedArrayTest, back_fail)
{
	Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>> array(g_arena);
	const Array<TypeParam, core::ArrayAlignedAllocator<TypeParam>> const_array(g_arena);

	core::debugging::EnableBreakpoints(false);
	g_AssetChecker.ExpectAssertion(true);

	// should throw assert.
	array.back();
	EXPECT_TRUE(g_AssetChecker.HadCorrectAssertions());

	g_AssetChecker.ExpectAssertion(true);

	const_array.back();
	EXPECT_TRUE(g_AssetChecker.HadCorrectAssertions());


	g_AssetChecker.ExpectAssertion(false);
	core::debugging::EnableBreakpoints(true);
}

X_PRAGMA(optimize("", on))
