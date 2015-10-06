#include "stdafx.h"
#include "gtest/gtest.h"


#include <Compression\LZ4.h>

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

TEST(LZ4, Unbuffered)
{
	// none bufferd defate of single buffer.
	const size_t srcBufSize = 4096;
	size_t deflatedSize, DestbufSize;

	// create a buffer.
	uint8_t* pUncompressed = X_NEW_ARRAY(uint8_t, srcBufSize, g_arena, "LZ4Uncompressed");

	FillBufRand(pUncompressed, srcBufSize);

	DestbufSize = LZ4::requiredDeflateDestBuf(srcBufSize);

	uint8_t* pCompressed = X_NEW_ARRAY(uint8_t, DestbufSize, g_arena, "LZ4Compressed");
	memset(pCompressed, 0, DestbufSize);

	bool deflateOk = LZ4::deflate(pUncompressed, srcBufSize, pCompressed, DestbufSize, deflatedSize);
	EXPECT_TRUE(deflateOk);

	if (deflateOk)
	{
		uint8_t* pUncompressed2 = X_NEW_ARRAY(uint8_t, srcBufSize, g_arena, "LZ4bUncompressed2");
		memset(pUncompressed2, 0, srcBufSize);

		bool inflateOk = LZ4::inflate(pCompressed, deflatedSize,
			pUncompressed2, srcBufSize);

		EXPECT_TRUE(inflateOk);
		if (inflateOk)
		{
			EXPECT_EQ(0, memcmp(pUncompressed, pUncompressed2, srcBufSize));
		}

		X_DELETE_ARRAY(pUncompressed2, g_arena);
	}

	// upon delete bounds are checked.
	X_DELETE_ARRAY(pUncompressed, g_arena);
	X_DELETE_ARRAY(pCompressed, g_arena);
}