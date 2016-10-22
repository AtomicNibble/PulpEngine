#include "stdafx.h"



#include <Compression\Zlib.h>

X_USING_NAMESPACE;

using namespace core::Compression;

namespace
{

	template<typename T>
	void FillBufRand(T* pBuf, size_t len)
	{
		FillBufRand(reinterpret_cast<uint8_t*>(pBuf), len * sizeof(T));
	}

	void FillBufRand(uint8_t* pBuf, size_t len)
	{
		for (size_t i = 0; i < len; i++)
			pBuf[i] = rand() % 256;
	}


	struct PodType
	{
		uint32_t meow;
		uint32_t meow1;
	};

} // namespace

TEST(Zlib, Unbuffered)
{
	// none bufferd defate of single buffer.
	const size_t srcBufSize = 4096;
	size_t deflatedSize, DestbufSize;

	// create a buffer.
	uint8_t* pUncompressed = X_NEW_ARRAY(uint8_t, srcBufSize, g_arena, "ZlibUncompressed");

	FillBufRand(pUncompressed, srcBufSize);

	DestbufSize = Zlib::requiredDeflateDestBuf(srcBufSize);

	uint8_t* pCompressed = X_NEW_ARRAY(uint8_t, DestbufSize, g_arena, "ZlibCompressed");
	memset(pCompressed, 0, DestbufSize);

	bool deflateOk = Zlib::deflate(g_arena, pUncompressed, srcBufSize, pCompressed, DestbufSize, deflatedSize);
	EXPECT_TRUE(deflateOk);

	if (deflateOk)
	{
		uint8_t* pUncompressed2 = X_NEW_ARRAY(uint8_t, srcBufSize, g_arena, "ZlibUncompressed2");
		memset(pUncompressed2, 0, srcBufSize);

		bool inflateOk = Zlib::inflate(g_arena, pCompressed, deflatedSize,
			pUncompressed2, srcBufSize);

		EXPECT_TRUE(inflateOk);
		if (inflateOk)
		{
			EXPECT_EQ(0,memcmp(pUncompressed, pUncompressed2, srcBufSize));
		}

		X_DELETE_ARRAY(pUncompressed2, g_arena);
	}

	// upon delete bounds are checked.
	X_DELETE_ARRAY(pUncompressed, g_arena);
	X_DELETE_ARRAY(pCompressed, g_arena);
}

#if 0
TEST(Zlib, buffered)
{
	const size_t srcBufSize = 4096;
	size_t deflatedSize, DestbufSize;

	// create a buffer.
	uint8_t* pUncompressed = X_NEW_ARRAY(uint8_t, srcBufSize, g_arena, "ZlibUncompressed");

	FillBufRand(pUncompressed, srcBufSize);

	DestbufSize = Zlib::requiredDeflateDestBuf(srcBufSize);

	uint8_t* pCompressed = X_NEW_ARRAY(uint8_t, DestbufSize, g_arena, "ZlibCompressed");
	memset(pCompressed, 0, DestbufSize);

	bool deflateOk = Zlib::deflate(g_arena, pUncompressed, srcBufSize, pCompressed, DestbufSize, deflatedSize);
	EXPECT_TRUE(deflateOk);

	if (deflateOk)
	{
		uint8_t* pUncompressed2 = X_NEW_ARRAY(uint8_t, srcBufSize, g_arena, "ZlibUncompressed2");
		memset(pUncompressed2, 0, srcBufSize);

		// do it in steps.
		ZlibInflate inflater(g_arena, pUncompressed2, srcBufSize);
		ZlibInflate::InflateResult::Enum res;

		const size_t bufSize = 256;
		size_t bufLeft = deflatedSize;
		size_t i = 0;

		do
		{
			size_t srcSize = core::Min(bufLeft, bufSize);

			res = inflater.Inflate(&pCompressed[bufSize * i], srcSize);

			bufLeft -= srcSize;

			i++;
		} while (res == ZlibInflate::InflateResult::OK);


		EXPECT_EQ(0, bufLeft);
		EXPECT_EQ(ZlibInflate::InflateResult::DONE, res);

		if (ZlibInflate::InflateResult::DONE == res)
		{
			EXPECT_EQ(0, memcmp(pUncompressed, pUncompressed2, srcBufSize));
		}

		X_DELETE_ARRAY(pUncompressed2, g_arena);
	}
}
#endif

TEST(Zlib, Array)
{
	core::Array<uint8_t> data(g_arena);
	core::Array<uint8_t> deflated(g_arena);

	data.resize(4096);
	FillBufRand(data.ptr(), data.size());

	bool deflateOk = Zlib::deflate(g_arena, data, deflated);
	ASSERT_TRUE(deflateOk);

	core::Array<uint8_t> inflated(g_arena, data.size());

	bool inflateOk = Zlib::inflate(g_arena, deflated, inflated);
	ASSERT_TRUE(inflateOk);

	// check it's the same
	ASSERT_EQ(data.size(), inflated.size());
	EXPECT_EQ(0, memcmp(data.ptr(), inflated.ptr(), inflated.size()));
}

TEST(Zlib, ArrayCustomType)
{

	core::Array<PodType> data(g_arena);
	core::Array<uint8_t> deflated(g_arena);

	data.resize(4096);
	FillBufRand(data.ptr(), data.size());

	bool deflateOk = Zlib::deflate(g_arena, data, deflated);
	ASSERT_TRUE(deflateOk);

	core::Array<PodType> inflated(g_arena, data.size());

	bool inflateOk = Zlib::inflate(g_arena, deflated, inflated);
	ASSERT_TRUE(inflateOk);

	// check it's the same
	ASSERT_EQ(data.size(), inflated.size());
	EXPECT_EQ(0, memcmp(data.ptr(), inflated.ptr(), inflated.size() * sizeof(PodType)));
}