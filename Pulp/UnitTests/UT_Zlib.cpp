#include "stdafx.h"
#include "gtest/gtest.h"


#include <Compression\Zlib.h>

X_USING_NAMESPACE;

using namespace core::Compression;

namespace
{
	void FillBufRand(uint8_t* pBuf, size_t len)
	{
		for (size_t i = 0; i < len; i++)
			pBuf[i] = rand() % 256;
	}

}

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

	bool deflateOk = Zlib::deflate(pUncompressed, srcBufSize, pCompressed, DestbufSize, deflatedSize);
	EXPECT_TRUE(deflateOk);

	if (deflateOk)
	{
		uint8_t* pUncompressed2 = X_NEW_ARRAY(uint8_t, srcBufSize, g_arena, "ZlibUncompressed2");
		memset(pUncompressed2, 0, srcBufSize);

		bool inflateOk = Zlib::inflate(pCompressed, deflatedSize,
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

	bool deflateOk = Zlib::deflate(pUncompressed, srcBufSize, pCompressed, DestbufSize, deflatedSize);
	EXPECT_TRUE(deflateOk);

	if (deflateOk)
	{
		uint8_t* pUncompressed2 = X_NEW_ARRAY(uint8_t, srcBufSize, g_arena, "ZlibUncompressed2");
		memset(pUncompressed2, 0, srcBufSize);

		// do it in steps.
		ZlibInflate inflater(pUncompressed2, srcBufSize);
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