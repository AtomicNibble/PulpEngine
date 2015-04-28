#include "stdafx.h"

#include "gtest/gtest.h"
// slap goat, just don't tickle it's throat.
// or you will be going home in a boat.
// i wrote it's address on a note.
// be gone!


#include <IFileSys.h>
#include <String\StackString.h>

X_USING_NAMESPACE;

using namespace core;

namespace {
	struct test_data
	{
		test_data() {
			core::zero_this(this);
		}

		uint32_t magic;
		StackString<32> str;
		int ival;

		bool operator==(const test_data& rhs) const {
			return magic == rhs.magic &&
				str.isEqual(rhs.str.c_str()) &&
				ival == rhs.ival;
		}
	};

	static uint32_t TEST_MAGIC = 0xF00DDEAD;
}



TEST(FileSys, Write)
{
	ASSERT_TRUE(NULL != gEnv->pFileSys);
	IFileSys* pFileSys = gEnv->pFileSys;

	XFile* file = pFileSys->openFile(X_ENGINE_NAME"_filesys_ut_data.ut_dat", 
		fileMode::WRITE | fileMode::RECREATE);

	ASSERT_TRUE(NULL != file);
	if (file)
	{
		test_data block;
		block.magic = TEST_MAGIC;
		block.ival = 123456789;
		block.str.set("tickle my pickle");

		ASSERT_EQ(sizeof(test_data),file->writeObj(block));

		pFileSys->closeFile(file);
	}
}

TEST(FileSys, Append)
{
	ASSERT_TRUE(NULL != gEnv->pFileSys);
	IFileSys* pFileSys = gEnv->pFileSys;

	XFile* file = pFileSys->openFile(X_ENGINE_NAME"_filesys_ut_data.ut_dat",
		fileMode::WRITE | fileMode::APPEND);

	ASSERT_TRUE(NULL != file);
	if (file)
	{
		test_data block;
		block.magic = TEST_MAGIC;
		block.ival = 123456789;
		block.str.append("tickle my pickle");

		ASSERT_EQ(sizeof(test_data), file->writeObj(block));

		pFileSys->closeFile(file);
	}
}

TEST(FileSys, Read)
{
	ASSERT_TRUE(NULL != gEnv->pFileSys);
	IFileSys* pFileSys = gEnv->pFileSys;

	XFile* file = pFileSys->openFile(X_ENGINE_NAME"_filesys_ut_data.ut_dat",
		fileMode::READ);

	ASSERT_TRUE(NULL != file);

	test_data block, test_block;
	block.magic = TEST_MAGIC;
	block.ival = 123456789;
	block.str.append("tickle my pickle");

	// read 2 blocks.
	file->readObj(test_block);
	EXPECT_EQ(block, test_block);
	file->readObj(test_block);
	EXPECT_EQ(block, test_block);

	pFileSys->closeFile(file);
}


TEST(FileSys, Async)
{
	ASSERT_TRUE(NULL != gEnv->pFileSys);
	IFileSys* pFileSys = gEnv->pFileSys;

	XFileAsync* file = pFileSys->openFileAsync(X_ENGINE_NAME"_filesys_ut_async_data.ut_dat",
		fileMode::WRITE | fileMode::RANDOM_ACCESS | fileMode::RECREATE);

	ASSERT_TRUE(NULL != file);

	char Buf[256];
	memset(Buf, 1, 128);
	memset(&Buf[128], 0xff, 128);

	core::XFileAsyncOperation write1 = file->writeAsync(Buf, sizeof(Buf), 0);
	core::XFileAsyncOperation write2 = file->writeAsync(Buf, sizeof(Buf), 256);

	write1.waitUntilFinished();
	write2.waitUntilFinished();

	pFileSys->closeFileAsync(file);
}


TEST(FileSys, Find)
{
	ASSERT_TRUE(NULL != gEnv->pFileSys);
	IFileSys* pFileSys = gEnv->pFileSys;

//	pFileSys->setGameDir(strUtil::workingDir());


	int num = 0;
	IFileSys::findData fd;
	uintptr_t handle = pFileSys->findFirst("core_assets/models/*.model", &fd);
	if (handle != IFileSys::INVALID_HANDLE)
	{
		do
		{
			X_LOG0("findresult", "name: \"%s\"", fd.name);
			num++;
		}
		while (pFileSys->findnext(handle, &fd));

		pFileSys->findClose(handle);
	}

	EXPECT_GT(num, 0);
}


TEST(FileSys, FileUtil)
{
	ASSERT_TRUE(NULL != gEnv->pFileSys);
	IFileSys* pFileSys = gEnv->pFileSys;

	EXPECT_TRUE(pFileSys->fileExists(X_ENGINE_NAME"_filesys_ut_data.ut_dat"));
	EXPECT_TRUE(pFileSys->deleteFile(X_ENGINE_NAME"_filesys_ut_data.ut_dat"));
	EXPECT_TRUE(pFileSys->deleteFile(X_ENGINE_NAME"_filesys_ut_async_data.ut_dat"));
}