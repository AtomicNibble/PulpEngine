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

    if (deflateOk) {
        uint8_t* pUncompressed2 = X_NEW_ARRAY(uint8_t, srcBufSize, g_arena, "ZlibUncompressed2");
        memset(pUncompressed2, 0, srcBufSize);

        bool inflateOk = Zlib::inflate(g_arena, pCompressed, deflatedSize,
            pUncompressed2, srcBufSize);

        EXPECT_TRUE(inflateOk);
        if (inflateOk) {
            EXPECT_EQ(0, memcmp(pUncompressed, pUncompressed2, srcBufSize));
        }

        X_DELETE_ARRAY(pUncompressed2, g_arena);
    }

    // upon delete bounds are checked.
    X_DELETE_ARRAY(pUncompressed, g_arena);
    X_DELETE_ARRAY(pCompressed, g_arena);
}

TEST(Zlib, bufferedInflate)
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

    if (deflateOk) {
        uint8_t* pUncompressed2 = X_NEW_ARRAY(uint8_t, srcBufSize, g_arena, "ZlibUncompressed2");
        memset(pUncompressed2, 0, srcBufSize);

        // do it in steps.
        ZlibInflate inflater(g_arena, [&](const uint8_t* pData, size_t len, size_t inflatedOffset) {
            ASSERT_LE(inflatedOffset + len, srcBufSize);
            std::memcpy(&pUncompressed2[inflatedOffset], pData, len);
        });

        ZlibInflate::Result::Enum res;

        const size_t bufSize = 256;
        size_t bufLeft = deflatedSize;
        size_t i = 0;

        do {
            size_t srcSize = core::Min(bufLeft, bufSize);

            res = inflater.Inflate(&pCompressed[bufSize * i], srcSize);

            bufLeft -= srcSize;

            i++;
        } while (res == ZlibInflate::Result::OK);

        EXPECT_EQ(0, bufLeft);
        EXPECT_EQ(ZlibInflate::Result::DONE, res);

        if (ZlibInflate::Result::DONE == res) {
            EXPECT_EQ(0, memcmp(pUncompressed, pUncompressed2, srcBufSize));
        }

        X_DELETE_ARRAY(pUncompressed2, g_arena);
    }

    X_DELETE_ARRAY(pUncompressed, g_arena);
    X_DELETE_ARRAY(pCompressed, g_arena);
}

struct BufInfo
{
    BufInfo(size_t src, size_t block, size_t out) :
        srcBufSize(src),
        blockBufSize(block),
        outBufSize(out)
    {
    }

    size_t srcBufSize;
    size_t blockBufSize;
    size_t outBufSize;
};

std::ostream& operator<<(std::ostream& s, BufInfo const& bi)
{
    s << "src " << bi.srcBufSize;
    s << " block " << bi.blockBufSize;
    s << " outBuf " << bi.outBufSize;
    return s;
}

class ZlibBufferd : public ::testing::TestWithParam<BufInfo>
{
};

// run the test with a load of diffrent buffer sizes to make sure
// buffers that are allowed to be bigger / smaller than others behave correct
INSTANTIATE_TEST_CASE_P(buffered, ZlibBufferd, ::testing::Values(BufInfo(4096, 1, 256), BufInfo(4096, 4096, 256), BufInfo(4096, 4096, 4096), BufInfo(4096, 8096, 256), BufInfo(4096, 8096, 4096), BufInfo(1, 1, 1), BufInfo(8538, 16, 8096), BufInfo(8538, 16, 8096 * 2)));

TEST_P(ZlibBufferd, bufferedInflate2)
{
    const BufInfo& bufInfo = GetParam();

    const size_t srcBufSize = bufInfo.srcBufSize;
    size_t deflatedSize, DestbufSize;

    // create a buffer.
    uint8_t* pUncompressed = X_NEW_ARRAY(uint8_t, srcBufSize, g_arena, "ZlibUncompressed");

    FillBufRand(pUncompressed, srcBufSize);

    DestbufSize = Zlib::requiredDeflateDestBuf(srcBufSize);

    uint8_t* pCompressed = X_NEW_ARRAY(uint8_t, DestbufSize, g_arena, "ZlibCompressed");
    memset(pCompressed, 0, DestbufSize);

    bool deflateOk = Zlib::deflate(g_arena, pUncompressed, srcBufSize, pCompressed, DestbufSize, deflatedSize);
    EXPECT_TRUE(deflateOk);

    if (deflateOk) {
        uint8_t* pUncompressed2 = X_NEW_ARRAY(uint8_t, srcBufSize, g_arena, "ZlibUncompressed2");
        memset(pUncompressed2, 0, srcBufSize);

        // do it in steps.
        ZlibInflate inflater(g_arena, [&](const uint8_t* pData, size_t len, size_t inflatedOffset) {
            ASSERT_LE(inflatedOffset + len, srcBufSize);
            std::memcpy(&pUncompressed2[inflatedOffset], pData, len);
        });

        inflater.setBufferSize(bufInfo.outBufSize);

        ZlibInflate::Result::Enum res;

        const size_t bufSize = bufInfo.outBufSize;
        size_t bufLeft = deflatedSize;
        size_t i = 0;

        do {
            size_t srcSize = core::Min(bufLeft, bufSize);

            res = inflater.Inflate(&pCompressed[bufSize * i], srcSize);

            bufLeft -= srcSize;

            i++;
        } while (res == ZlibInflate::Result::OK);

        EXPECT_EQ(0, bufLeft);
        EXPECT_EQ(ZlibInflate::Result::DONE, res);
        ASSERT_EQ(inflater.inflatedSize(), srcBufSize);

        if (ZlibInflate::Result::DONE == res) {
            EXPECT_EQ(0, memcmp(pUncompressed, pUncompressed2, srcBufSize));
        }

        X_DELETE_ARRAY(pUncompressed2, g_arena);
    }

    X_DELETE_ARRAY(pUncompressed, g_arena);
    X_DELETE_ARRAY(pCompressed, g_arena);
}

TEST_P(ZlibBufferd, bufferedDeflate)
{
    const BufInfo& bufInfo = GetParam();

    const size_t srcBufSize = bufInfo.srcBufSize;

    uint8_t* pUncompressed = X_NEW_ARRAY(uint8_t, srcBufSize, g_arena, "ZlibUncompressed");
    uint8_t* pUncompressed2 = X_NEW_ARRAY(uint8_t, srcBufSize, g_arena, "ZlibUncompressed");
    FillBufRand(pUncompressed, srcBufSize);

    size_t comrpessedBufSize = Zlib::requiredDeflateDestBuf(srcBufSize);
    uint8_t* pDeflated = X_NEW_ARRAY(uint8_t, comrpessedBufSize, g_arena, "ZlibCompressed");

    ZlibDefalte deflater(g_arena, [&](const uint8_t* pData, size_t len, size_t deflateOffset) {
        ASSERT_LE(deflateOffset + len, comrpessedBufSize);
        std::memcpy(&pDeflated[deflateOffset], pData, len);
    });

    deflater.setBufferSize(bufInfo.outBufSize);
    ZlibDefalte::Result::Enum res;

    {
        const size_t bufSize = bufInfo.blockBufSize;
        size_t bufLeft = srcBufSize;
        size_t i = 0;

        do {
            size_t srcSize = core::Min(bufLeft, bufSize);
            bufLeft -= srcSize;

            const bool last = (bufLeft == 0);

            res = deflater.Deflate(&pUncompressed[bufSize * i], srcSize, last);

            i++;
        } while (res == ZlibDefalte::Result::OK && bufLeft > 0);

        EXPECT_EQ(0, bufLeft);
        EXPECT_EQ(ZlibDefalte::Result::DONE, res);
    }

    // now deflate normal and compare.
    const size_t deflatedSize = deflater.deflatedSize();
    bool inflateOk = Zlib::inflate(g_arena, pDeflated, deflatedSize, pUncompressed2, srcBufSize);
    EXPECT_TRUE(inflateOk);

    if (ZlibDefalte::Result::DONE == res) {
        EXPECT_EQ(0, memcmp(pUncompressed, pUncompressed2, srcBufSize));
    }

    X_DELETE_ARRAY(pUncompressed, g_arena);
    X_DELETE_ARRAY(pUncompressed2, g_arena);
    X_DELETE_ARRAY(pDeflated, g_arena);
}

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