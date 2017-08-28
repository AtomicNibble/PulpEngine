#pragma once

#ifndef _H_MATH_HALF_H_
#define _H_MATH_HALF_H_

#include <Math\XVector.h>
#include <Util\CustomLiterals.h>

X_NAMESPACE_BEGIN(core)

typedef uint16_t XHalf;

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

	static int32_t const infN = 0x7F800000; // flt32 infinity
	static int32_t const maxN = 0x477FE000; // max flt16 normal as a flt32
	static int32_t const minN = 0x38800000; // min flt16 normal as a flt32
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

	static uint16_t compress(float value);
	static float decompress(uint16_t value);
};


struct XHalf2
{
	XHalf2() {}
	XHalf2(XHalf x, XHalf y) :
		x(x), y(y)
	{
	}

	XHalf2(float _x, float _y)
	{
		x = XHalfCompressor::compress(_x);
		y = XHalfCompressor::compress(_y);
	}
	explicit XHalf2(const float* const __restrict pArray)
	{
		x = XHalfCompressor::compress(pArray[0]);
		y = XHalfCompressor::compress(pArray[1]);
	}

	XHalf2& operator=(const XHalf2& rhs)
	{
		x = rhs.x;
		y = rhs.y;
		return *this;
	}


	static XHalf2 compress(float x, float y)
	{
		return XHalf2(
			XHalfCompressor::compress(x),
			XHalfCompressor::compress(y)
		);
	}

	static XHalf2 zero(void)
	{
		return XHalf2(0_u16, 0_u16);
	}

	XHalf x;
	XHalf y;
};

struct XHalf4
{
	XHalf4() {}
	XHalf4(XHalf x, XHalf y, XHalf z, XHalf w) :
		x(x), y(y), z(z), w(w) 
	{
	}

	XHalf4(float _x, float _y, float _z, float _w)
	{
		x = XHalfCompressor::compress(_x);
		y = XHalfCompressor::compress(_y);
		z = XHalfCompressor::compress(_z);
		w = XHalfCompressor::compress(_w);
	}
	explicit XHalf4(const float* const __restrict pArray)
	{
		x = XHalfCompressor::compress(pArray[0]);
		y = XHalfCompressor::compress(pArray[1]);
		z = XHalfCompressor::compress(pArray[2]);
		w = XHalfCompressor::compress(pArray[3]);
	}

	XHalf4& operator=(const XHalf4& rhs)
	{
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
		w = rhs.w;
		return *this;
	}

	Vec4f decompress(void) {
		return Vec4f(
			XHalfCompressor::decompress(x),
			XHalfCompressor::decompress(y),
			XHalfCompressor::decompress(z),
			XHalfCompressor::decompress(w)
		);
	}

	static XHalf4 zero(void)
	{
		return XHalf4(0_u16,0_u16,0_u16,0_u16);
	}

	XHalf x;
	XHalf y;
	XHalf z;
	XHalf w;
};


X_NAMESPACE_END

#endif // !_H_MATH_HALF_H_
