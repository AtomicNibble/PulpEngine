#include "stdafx.h"

#include <Compression\LZ4.h>

#include <Memory\MemCursor.h>

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

    template<size_t N>
    void FillBufRandDupes(uint8_t* pBuf, size_t len)
    {
        std::array<uint8_t, N> vals;

        for (size_t i = 0; i < N; i++) {
            vals[i] = rand() % 256;
        }

        for (size_t i = 0; i < len; i++) {
            pBuf[i] = vals[rand() % N];
        }
    }

    struct PodType
    {
        uint32_t meow;
        uint32_t meow1;
    };

    typedef ::testing::Types<LZ4, LZ4HC> CompressorTypes;
    TYPED_TEST_CASE(LZ4Test, CompressorTypes);

    template<typename T>
    class LZ4Test : public ::testing::Test
    {
    public:
    };

} // namespace

TYPED_TEST(LZ4Test, Unbuffered)
{
    // none bufferd defate of single buffer.
    const size_t srcBufSize = 4096;
    size_t deflatedSize, DestbufSize;

    // create a buffer.
    uint8_t* pUncompressed = X_NEW_ARRAY(uint8_t, srcBufSize, g_arena, "LZ4Uncompressed");

    FillBufRand(pUncompressed, srcBufSize);

    DestbufSize = TypeParam::requiredDeflateDestBuf(srcBufSize);

    uint8_t* pCompressed = X_NEW_ARRAY(uint8_t, DestbufSize, g_arena, "LZ4Compressed");
    memset(pCompressed, 0, DestbufSize);

    bool deflateOk = TypeParam::deflate(g_arena, pUncompressed, srcBufSize, pCompressed, DestbufSize, deflatedSize);
    EXPECT_TRUE(deflateOk);

    if (deflateOk) {
        uint8_t* pUncompressed2 = X_NEW_ARRAY(uint8_t, srcBufSize, g_arena, "LZ4bUncompressed2");
        memset(pUncompressed2, 0, srcBufSize);

        bool inflateOk = TypeParam::inflate(g_arena, pCompressed, deflatedSize,
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

TYPED_TEST(LZ4Test, Array)
{
    core::Array<uint8_t> data(g_arena);
    core::Array<uint8_t> deflated(g_arena);

    data.resize(4096);
    FillBufRand(data.ptr(), data.size());

    bool deflateOk = TypeParam::deflate(g_arena, data, deflated);
    ASSERT_TRUE(deflateOk);

    core::Array<uint8_t> inflated(g_arena, data.size());

    bool inflateOk = TypeParam::inflate(g_arena, deflated, inflated);
    ASSERT_TRUE(inflateOk);

    // check it's the same
    ASSERT_EQ(data.size(), inflated.size());
    EXPECT_EQ(0, memcmp(data.ptr(), inflated.ptr(), inflated.size()));
}

TYPED_TEST(LZ4Test, ArrayCustomType)
{
    core::Array<PodType> data(g_arena);
    core::Array<uint8_t> deflated(g_arena);

    data.resize(4096);
    FillBufRand(data.ptr(), data.size());

    bool deflateOk = TypeParam::deflate(g_arena, data, deflated);
    ASSERT_TRUE(deflateOk);

    core::Array<PodType> inflated(g_arena, data.size());

    bool inflateOk = TypeParam::inflate(g_arena, deflated, inflated);
    ASSERT_TRUE(inflateOk);

    // check it's the same
    ASSERT_EQ(data.size(), inflated.size());
    EXPECT_EQ(0, memcmp(data.ptr(), inflated.ptr(), inflated.size() * sizeof(PodType)));
}

TEST(LZ4, Stream)
{
    const size_t BUF_SIZE = 4096;
    const size_t NUM = 64;

    core::Array<uint8_t> data(g_arena);
    core::Array<uint8_t> dataOut(g_arena);

    std::vector<uint8_t> compData;
    std::vector<uint8_t> deCompData;

    {
        char buf[2][BUF_SIZE];
        char cmpBuf[LZ4Stream::requiredDeflateDestBuf(BUF_SIZE)];

        int32_t bufIdx = 0;

        // the input data.
        data.resize(BUF_SIZE * NUM);
        FillBufRandDupes<4>(data.ptr(), data.size());

        LZ4Stream stream(g_arena);

        for (size_t i = 0; i < NUM; i++) {
            // simulate streaming it in.
            ::memcpy(buf[bufIdx], &data[i * BUF_SIZE], BUF_SIZE);

            size_t compSize = stream.compressContinue(buf[bufIdx], BUF_SIZE, cmpBuf, sizeof(cmpBuf),
                core::Compression::CompressLevel::NORMAL);

            // write the size.
            uint32_t size = safe_static_cast<uint32_t, size_t>(compSize);
            uint8_t* pSize = reinterpret_cast<uint8_t*>(&size);

            compData.insert(compData.end(), pSize, pSize + sizeof(size));
            compData.insert(compData.end(), cmpBuf, cmpBuf + compSize);

            bufIdx = (bufIdx + 1) % 2;
        }
    }

    {
        char cmpbuf[LZ4Stream::requiredDeflateDestBuf(BUF_SIZE)];
        char decBuf[2][BUF_SIZE];

        int32_t bufIdx = 0;

        LZ4StreamDecode streamDe;

        // we read the sizes and decompress.
        core::MemCursor memCur(compData.data(), compData.size());

        for (size_t i = 0; i < NUM; i++) {
            uint32_t compSize = memCur.getSeek<uint32_t>();
            ::memcpy(cmpbuf, memCur.postSeekPtr<uint8_t>(compSize), compSize);

            char* const pDec = decBuf[bufIdx];

            size_t decSize = streamDe.decompressContinue(cmpbuf, pDec, compSize, BUF_SIZE);

            deCompData.insert(deCompData.end(), pDec, pDec + decSize);

            bufIdx = (bufIdx + 1) % 2;
        }
    }

    // check same.
    ASSERT_EQ(deCompData.size(), data.size());
    EXPECT_TRUE(memcmp(deCompData.data(), data.ptr(), data.size()) == 0);
}