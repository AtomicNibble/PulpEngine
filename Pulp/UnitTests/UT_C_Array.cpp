#include "stdafx.h"

#include <gtest\gtest.h>

#include <Containers\Array.h>

#include <String\Path.h>

X_USING_NAMESPACE;

using namespace core;


typedef ::testing::Types<short, int> MyTypes;
TYPED_TEST_CASE(ArrayTest, MyTypes);

template <typename T>
class ArrayTest : public ::testing::Test {
public:
};

namespace {
	X_ALIGNED_SYMBOL(struct CustomType, 32)
	{
		CustomType() : var_(16) {
			CONSRUCTION_COUNT++;
		}

		CustomType(const CustomType& oth)
			: var_(oth.var_)
		{
			++CONSRUCTION_COUNT;
		}

		~CustomType() {
			DECONSRUCTION_COUNT++;
		}

		inline int GetVar() const {
			return var_;
		}
	private:
		int var_;

	public:
		static int CONSRUCTION_COUNT;
		static int DECONSRUCTION_COUNT;
	};

	int CustomType::CONSRUCTION_COUNT = 0;
	int CustomType::DECONSRUCTION_COUNT = 0;
}


TYPED_TEST(ArrayTest, Contruct)
{
	Array<TypeParam> list(g_arena);

	list.append(TypeParam());
	list.append(TypeParam());
	list.setGranularity(345);
	list.append(TypeParam());
	list.append(TypeParam());

	Array<TypeParam> list2(list);

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

TYPED_TEST(ArrayTest, Append)
{
	Array<TypeParam> list(g_arena);

	EXPECT_EQ(0, list.size());
	EXPECT_EQ(0, list.capacity());
	EXPECT_LT((Array<TypeParam>::size_type)0, list.granularity()); // gran should be above 0.

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
		EXPECT_EQ(i*4,list[i]);
	}

	// test the memory block it gives us.
	Array<TypeParam>::Type* pArr = list.ptr();
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

TYPED_TEST(ArrayTest, AppendArr)
{
		// we now allow a array of same tpye to be appended.
	Array<TypeParam> list(g_arena);

	EXPECT_EQ(0, list.size());
	EXPECT_EQ(0, list.capacity());
	EXPECT_LT((Array<TypeParam>::size_type)0, list.granularity()); // gran should be above 0.

	EXPECT_EQ(nullptr, list.ptr());
	
	for (size_t i = 0; i < 39; i++)
	{
		list.append(static_cast<TypeParam>(i * 4));
	}

	EXPECT_EQ(39, list.size());

	Array<TypeParam> list2(g_arena);
	list2.append(1337);

	EXPECT_EQ(1, list2.size());

	// apend the list.
	list.append(list2);

	// now 40
	EXPECT_EQ(40, list.size());

	// check the values are correct.
	EXPECT_EQ(1337,list[39]);
	for (int i = 0; i < 39; i++)
	{
		EXPECT_EQ(i*4,list[i]);
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



TYPED_TEST(ArrayTest, Insert)
{
	Array<TypeParam> list(g_arena);

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


TYPED_TEST(ArrayTest, Remove)
{
	Array<TypeParam> list(g_arena);

	list.setGranularity(128);

	EXPECT_EQ(128, list.granularity());

	// insert some items
	for (int i = 0; i < 64; i++)
	{
		EXPECT_EQ(i, list.insert(i, i*2));
	}

	EXPECT_EQ(64, list.size());
	EXPECT_EQ(128, list.capacity());

	EXPECT_TRUE(list.removeIndex(4));
	EXPECT_TRUE(list.removeIndex(10));
	EXPECT_TRUE(list.removeIndex(16));
	EXPECT_TRUE(list.removeIndex(28));
	
	EXPECT_EQ(60, list.size());
	EXPECT_EQ(128, list.capacity());

	list.free();

	EXPECT_EQ(0, list.size());
	EXPECT_EQ(0, list.capacity());
	EXPECT_EQ(nullptr, list.ptr());
}


TYPED_TEST(ArrayTest, Iterator)
{
	Array<TypeParam> list(g_arena);

	// 64 items all with value of 128
	list.resize(64, 128);


	Array<TypeParam>::Iterator it = list.begin();
	for (; it != list.end(); ++it)
	{
		EXPECT_EQ(128, *it);
		*it = 64;
	}

	Array<TypeParam>::ConstIterator cit = list.begin();
	for (; cit != list.end(); ++cit)
	{
		EXPECT_EQ(64, *cit);
	}

}

TYPED_TEST(ArrayTest, Serialize)
{
	Array<TypeParam> list(g_arena);

	EXPECT_EQ(0, list.size());
	EXPECT_EQ(0, list.capacity());
	EXPECT_LT((Array<TypeParam>::size_type)0, list.granularity()); // gran should be above 0.

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
	core::Path fileName;

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

		ASSERT_EQ(0,file->remainingBytes());

		pFileSys->closeFile(file);
	}
}

TYPED_TEST(ArrayTest, CustomType)
{
	CustomType val;

	Array<CustomType> list(g_arena);

	list.setGranularity(1024);
	list.reserve(120);

	EXPECT_EQ( 0, list.size());
	ASSERT_EQ(1024, list.capacity());
	EXPECT_EQ(1024, list.granularity());
	EXPECT_NE(nullptr, list.ptr());

	for (int i = 0; i < 110; i++)
	{
		list.append(val);
	}

	EXPECT_EQ(110, list.size());
	EXPECT_EQ(1024, list.capacity());

}

