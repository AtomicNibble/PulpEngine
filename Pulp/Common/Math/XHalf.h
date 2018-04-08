#pragma once

#ifndef _H_MATH_HALF_H_
#define _H_MATH_HALF_H_

#include <Math\XVector.h>

X_NAMESPACE_BEGIN(core)

typedef uint16_t XHalf;
typedef Vec2<XHalf> Vec2Half;
typedef Vec4<XHalf> Vec4Half;

class XHalfCompressor
{
    union Bits
    {
        float f;
        int32_t si;
        uint32_t ui;
    };

    static int const shift = 13;
    static int const shiftSign = 16;

    static int32_t const infN = 0x7F800000;  // flt32 infinity
    static int32_t const maxN = 0x477FE000;  // max flt16 normal as a flt32
    static int32_t const minN = 0x38800000;  // min flt16 normal as a flt32
    static int32_t const signN = 0x80000000; // flt32 sign bit

    static int32_t const infC = infN >> shift;
    static int32_t const nanN = (infC + 1) << shift; // minimum flt16 nan as a flt32
    static int32_t const maxC = maxN >> shift;
    static int32_t const minC = minN >> shift;
    static int32_t const signC = signN >> shiftSign; // flt16 sign bit

    static int32_t const mulN = 0x52000000; // (1 << 23) / minN
    static int32_t const mulC = 0x33800000; // minN / (1 << (23 - shift))

    static int32_t const subC = 0x003FF; // max flt32 subnormal down shifted
    static int32_t const norC = 0x00400; // min flt32 normal down shifted

    static int32_t const maxD = infC - maxC - 1;
    static int32_t const minD = minC - subC - 1;

public:
    static XHalf compress(float v);
    static Vec4Half compress(float x, float y, float z, float w);
    static Vec4Half compress(Vec4f v);

    static float decompress(XHalf v);
    static Vec4f decompress(Vec4Half v);
};

struct XHalf2
{
    XHalf2() = default;
    XHalf2(XHalf x, XHalf y) :
        v(x, y)
    {
    }

    XHalf2(float x, float y)
    {
        v.x = XHalfCompressor::compress(x);
        v.y = XHalfCompressor::compress(y);
    }
    explicit XHalf2(const float* const __restrict pArray)
    {
        v.x = XHalfCompressor::compress(pArray[0]);
        v.y = XHalfCompressor::compress(pArray[1]);
    }

    XHalf2& operator=(const XHalf2& rhs) = default;

    static XHalf2 compress(float x, float y)
    {
        return XHalf2(
            XHalfCompressor::compress(x),
            XHalfCompressor::compress(y));
    }

    static XHalf2 zero(void)
    {
        return XHalf2(0_ui16, 0_ui16);
    }

    Vec2Half v;
};

struct XHalf4
{
    XHalf4() = default;
    XHalf4(XHalf x, XHalf y, XHalf z, XHalf w) :
        v(x,y,z,w)
    {
    }

    XHalf4(float x, float y, float z, float w)
    {
        v = XHalfCompressor::compress(x, y, z, w);
    }
    explicit XHalf4(const float* const __restrict pArray)
    {
        v = XHalfCompressor::compress(pArray[0], pArray[1], pArray[2], pArray[3]);
    }

    XHalf4& operator=(const XHalf4& rhs)  = default;

    Vec4f decompress(void)
    {
        return Vec4f(
            XHalfCompressor::decompress(v.x),
            XHalfCompressor::decompress(v.y),
            XHalfCompressor::decompress(v.z),
            XHalfCompressor::decompress(v.w));
    }

    static XHalf4 zero(void)
    {
        return XHalf4(0_ui16, 0_ui16, 0_ui16, 0_ui16);
    }

    Vec4Half v;
};

X_NAMESPACE_END

#endif // !_H_MATH_HALF_H_
