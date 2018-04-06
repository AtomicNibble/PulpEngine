#include "stdafx.h"

#include <Math\XHalf.h>

X_USING_NAMESPACE;

using namespace core;

#define EXPECT_NEAR_VEC4(expected, actual, angle_error) \
    {                                                   \
        Vec4f _exp = expected;                          \
        Vec4f _act = actual;                            \
        EXPECT_NEAR(_exp.x, _act.x, angle_error);       \
        EXPECT_NEAR(_exp.y, _act.y, angle_error);       \
        EXPECT_NEAR(_exp.z, _act.z, angle_error);       \
        EXPECT_NEAR(_exp.w, _act.w, angle_error);       \
    }

TEST(Half, Single)
{
    XHalf out;

    out = XHalfCompressor::compress(0.5f);

    EXPECT_EQ(14336, out);
    EXPECT_FLOAT_EQ(0.5f, XHalfCompressor::decompress(out));

    out = XHalfCompressor::compress(0.12345678f);

    EXPECT_NEAR(12262, out, 1);
    EXPECT_NEAR(0.12345678f, XHalfCompressor::decompress(out), 0.0001f);
}

TEST(Half, Quad)
{
    Vec4f vec = Vec4f(0.5454f, 0.4535f, 0.24162f, 0.8622f);

    XHalf4 compressed((float*)&vec);

    EXPECT_NEAR_VEC4(vec, compressed.decompress(), 0.001f);
}