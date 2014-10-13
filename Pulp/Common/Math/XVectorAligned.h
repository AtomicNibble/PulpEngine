#pragma once


#ifndef X_MATH_VECTOR_ALIGHNED_H_
#define X_MATH_VECTOR_ALIGHNED_H_

// a 16bytes aligned Vec3f & Vec4f
// that uses SSE :)
// only supports floats

#include <xmmintrin.h>

X_ALIGNED_SYMBOL(class Vec4fA, 16)
{
public:
	typedef float value_type;
	typedef VECTRAIT<float>::DIST	DIST;

	static const unsigned int	FABS_MASK = 0x7fffffff;
	static const unsigned int	INV_MASK = 0x80000000;
//	static const float			EPSILON = 0.001f;

	X_INLINE Vec4fA();
	X_INLINE Vec4fA(float X, float Y, float Z, float W = 0);

	// all the goats
	X_PUSH_WARNING_LEVEL(3)
	union {
		struct {
			float x;
			float y;
			float z;
			float w;
		};
		__m128 sse;
		float data[4];
	};
	X_POP_WARNING_LEVEL

	// kinky operators
	X_INLINE int operator!() const;
	X_INLINE Vec4fA& operator=(const Vec4fA& oth);

	X_INLINE const float& operator[](int i) const;
	X_INLINE float& operator[](int i);

	X_INLINE const Vec4fA	operator+(const Vec4fA& rhs) const;
	X_INLINE const Vec4fA	operator-(const Vec4fA& rhs) const;
	X_INLINE const Vec4fA	operator*(const Vec4fA& rhs) const;
	X_INLINE const Vec4fA	operator/(const Vec4fA& rhs) const;

	X_INLINE Vec4fA&	operator+=(const Vec4fA& rhs);
	X_INLINE Vec4fA&	operator-=(const Vec4fA& rhs);
	X_INLINE Vec4fA&	operator*=(const Vec4fA& rhs);
	X_INLINE Vec4fA&	operator/=(const Vec4fA& rhs);
	X_INLINE const Vec4fA	operator/(float rhs) const;
	X_INLINE Vec4fA&	operator+=(float rhs);
	X_INLINE Vec4fA&	operator-=(float rhs);
	X_INLINE Vec4fA&	operator*=(float rhs);
	X_INLINE Vec4fA&	operator/=(float rhs);

	X_INLINE Vec4fA	operator-();

	X_INLINE bool operator==(const Vec4fA& oth) const;
	X_INLINE bool operator!=(const Vec4fA& oth) const;

	X_INLINE float dot(const Vec4fA& oth) const;
	X_INLINE Vec4fA cross(const Vec4fA &rhs) const;
	X_INLINE float distance(const Vec4fA &rhs) const;
	X_INLINE float distanceSquared(const Vec4fA &rhs) const;

	X_INLINE DIST length() const;

	X_INLINE float lengthSquared() const;

	Vec4fA& normalize();
	Vec4fA normalized() const;

	// Tests for zero-length (yolo)
	X_INLINE Vec4fA& normalizeSafe();

	X_INLINE Vec4fA& invert();
	X_INLINE Vec4fA inverse() const;

	X_INLINE bool compare(const Vec4fA& oth, float elipson = 0.0001f) const;


	static Vec4fA zero() {
		return Vec4fA(0.f,0.f,0.f,0.f);
	}

	static Vec4fA one() {
		return Vec4fA(1.f,1.f,1.f,1.f);
	}

};

// ----------------------------------------------------

Vec4fA::Vec4fA()
{
	sse = _mm_setzero_ps();
}

Vec4fA::Vec4fA(float X, float Y, float Z, float W) : x(X), y(Y), z(Z), w(W)
{

}


// ----------------------------------------------------

X_INLINE Vec4fA& Vec4fA::operator=(const Vec4fA& oth)
{
	_mm_store_ps(data, _mm_load_ps((float*)&oth));
	return *this;
}


X_INLINE const float& Vec4fA::operator[](int i) const 
{
	X_ASSERT(i >= 0 && i < 4, "out of range")(i);
	return data[i];
}

X_INLINE float& Vec4fA::operator[](int i) 
{
	X_ASSERT(i >= 0 && i < 4, "out of range")(i);
	return data[i];
}


// ----------------------------------------------------

X_INLINE const Vec4fA Vec4fA::operator+(const Vec4fA& rhs) const
{
	Vec4fA r;
	r.sse = _mm_add_ps(sse, rhs.sse);
	return r;
}

X_INLINE const Vec4fA Vec4fA::operator-(const Vec4fA& rhs) const
{
	Vec4fA r;
	r.sse = _mm_sub_ps(sse, rhs.sse);
	return r;
}

X_INLINE const Vec4fA Vec4fA::operator*(const Vec4fA& rhs) const
{
	Vec4fA r;
	r.sse = _mm_mul_ps(sse, rhs.sse);
	return r;
}

X_INLINE const Vec4fA Vec4fA::operator/(const Vec4fA& rhs) const
{
	Vec4fA r;
	r.sse = _mm_div_ps(sse, rhs.sse);
	return r;
}

// ----------------------------------------------------

X_INLINE Vec4fA& Vec4fA::operator+=(const Vec4fA& rhs)
{
	sse = _mm_add_ps(sse, rhs.sse);
	return *this;
}

X_INLINE Vec4fA& Vec4fA::operator-=(const Vec4fA& rhs)
{
	sse = _mm_sub_ps(sse, rhs.sse);
	return *this;
}

X_INLINE Vec4fA& Vec4fA::operator*=(const Vec4fA& rhs)
{
	sse = _mm_mul_ps(sse, rhs.sse);
	return *this;
}

X_INLINE Vec4fA& Vec4fA::operator/=(const Vec4fA& rhs)
{
	sse = _mm_div_ps(sse, rhs.sse);
	return *this;
}

// --------- Single ----------

X_INLINE const Vec4fA Vec4fA::operator/(float rhs) const
{
	Vec4fA r;
	__m128 f4 = _mm_load_ss(&rhs);
	f4 = _mm_shuffle_ps(f4, f4, 0x00);
	r.sse = _mm_div_ps(sse, f4);
	return r;
}

X_INLINE Vec4fA& Vec4fA::operator+=(float rhs)
{
	__m128 f4 = _mm_load_ss(&rhs);
	f4 = _mm_shuffle_ps(f4, f4, 0x00);
	sse = _mm_add_ps(sse, f4);
	return *this;
}

X_INLINE Vec4fA& Vec4fA::operator-=(float rhs)
{
	__m128 f4 = _mm_load_ss(&rhs);
	f4 = _mm_shuffle_ps(f4, f4, 0x00);
	sse = _mm_sub_ps(sse, f4);
	return *this;
}

X_INLINE Vec4fA& Vec4fA::operator*=(float rhs)
{
	__m128 f4 = _mm_load_ss(&rhs);
	f4 = _mm_shuffle_ps(f4, f4, 0x00);
	sse = _mm_mul_ps(sse, f4);
	return *this;
}

X_INLINE Vec4fA& Vec4fA::operator/=(float rhs)
{
	__m128 f4 = _mm_load_ss(&rhs);
	f4 = _mm_shuffle_ps(f4, f4, 0x00);
	sse = _mm_div_ps(sse, f4);
	return *this;
}

// ----------------------------------------------------


X_INLINE Vec4fA	Vec4fA::operator-()
{
	Vec4fA r;
	r.sse = _mm_xor_ps(sse, _mm_set1_ps(-0.f));
	return r;
}

// ----------------------------------------------------


X_INLINE bool Vec4fA::operator==(const Vec4fA& oth) const
{
	return compare(oth);
}

X_INLINE bool Vec4fA::operator!=(const Vec4fA& oth) const
{
	return !(*this == oth);
}

// ----------------------------------------------------


X_INLINE float Vec4fA::dot(const Vec4fA& oth) const
{
	// mul then add.
	__m128 sq = _mm_mul_ps(this->sse, oth.sse);
	return sq.m128_f32[0] + sq.m128_f32[1] + sq.m128_f32[2] + sq.m128_f32[3];
}

Vec4fA Vec4fA::cross(const Vec4fA &rhs) const
{
	Vec4fA r;
	__declspec(align(16)) __m128 a1 = _mm_shuffle_ps(sse, sse, _MM_SHUFFLE(3, 0, 2, 1));
	__m128 b1 = _mm_shuffle_ps(rhs.sse, rhs.sse, _MM_SHUFFLE(3, 1, 0, 2));
	__m128 a2 = _mm_shuffle_ps(sse, sse, _MM_SHUFFLE(3, 1, 0, 2));
	__m128 b2 = _mm_shuffle_ps(rhs.sse, rhs.sse, _MM_SHUFFLE(3, 0, 2, 1));
	a1 = _mm_mul_ps(a1, b1);
	a2 = _mm_mul_ps(a2, b2);
	r.sse = _mm_sub_ps(a1, a2);
	return r;
}

float Vec4fA::distance(const Vec4fA &rhs) const
{
	return (float)(*this - rhs).length();
}

float Vec4fA::distanceSquared(const Vec4fA &rhs) const
{
	return (*this - rhs).lengthSquared();
}

Vec4fA::DIST Vec4fA::length() const
{
	// For most vector operations, this assumes w to be zero.
#if 1
	__m128 sq = _mm_mul_ps(sse, sse);
	float len = sq.m128_f32[0] + sq.m128_f32[1] + sq.m128_f32[2] + sq.m128_f32[3];
	return math<DIST>::sqrt((DIST)len);
#else
	return math<DIST>::sqrt((DIST)(x*x + y*y + z*z + w*w));
#endif
}

float Vec4fA::lengthSquared() const
{
	// For most vector operations, this assumes w to be zero.
	__m128 sq = _mm_mul_ps(sse, sse);
	return sq.m128_f32[0] + sq.m128_f32[1] + sq.m128_f32[2] + sq.m128_f32[3];
}


Vec4fA& Vec4fA::invert()
{
#if 1
	const __m128 mask = _mm_set1_ps(*reinterpret_cast<float*>((unsigned int*)&INV_MASK));

	sse = _mm_xor_ps(sse, mask);
#else
	x = -x; y = -y; z = -z; w = -w;
#endif
	return *this;
}

Vec4fA Vec4fA::inverse() const
{
	const __m128 mask = _mm_set1_ps(*reinterpret_cast<float*>((unsigned int*)&INV_MASK));

	Vec4fA r;
	r.sse = _mm_xor_ps(sse, mask);
	return r;
}


bool Vec4fA::compare(const Vec4fA& oth, float elipson) const
{
	__m128 dif = _mm_sub_ps(sse, oth.sse);
	__m128 fabs_mask4 = _mm_set1_ps(*reinterpret_cast<float*>((unsigned int*)&FABS_MASK));
	__m128 epsilon4 = _mm_set1_ps(elipson);
	dif = _mm_and_ps(dif, fabs_mask4);
	dif = _mm_sub_ps(epsilon4, dif);

	return !_mm_movemask_ps(dif);
}


// >>>>>>>>>>>>>>>>>> 0.0 <<<<<<<<<<<<<<<<<<<<<<<

typedef Vec4fA Vec3fA;


X_ENSURE_SIZE(Vec3fA, 16);
X_ENSURE_SIZE(Vec4fA, 16);

#endif // !X_MATH_VECTOR_ALIGHNED_H_