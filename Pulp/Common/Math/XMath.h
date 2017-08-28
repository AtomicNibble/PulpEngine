#pragma once


#ifndef _X_MATH_H_
#define _X_MATH_H_


#include <cmath>
#include <climits>
#include <float.h>


#ifdef INFINITY
#undef INFINITY
#endif

X_DISABLE_WARNING(4244)

template<typename T>
struct math
{
	X_INLINE static  constexpr T  square(T x)		{ return x * x; }

	X_INLINE static  T	acos(T x)		{ return ::acos(double(x)); }
	X_INLINE static  T	asin(T x)		{ return ::asin(double(x)); }
	X_INLINE static  T	atan(T x)		{ return ::atan(double(x)); }
	X_INLINE static  T	atan2(T y, T x)	{ return ::atan2(double(y), double(x)); }
	X_INLINE static  T	cos(T x)		{ return ::cos(double(x)); }
	X_INLINE static  T	sin(T x)		{ return ::sin(double(x)); }
	X_INLINE static  T	tan(T x)		{ return ::tan(double(x)); }
	X_INLINE static  T	cosh(T x)		{ return ::cosh(double(x)); }
	X_INLINE static  T	sinh(T x)		{ return ::sinh(double(x)); }
	X_INLINE static  T	tanh(T x)		{ return ::tanh(double(x)); }
	X_INLINE static  T	exp(T x)		{ return ::exp(double(x)); }
	X_INLINE static  T	log(T x)		{ return ::log(double(x)); }
	X_INLINE static  T	log10(T x)		{ return ::log10(double(x)); }
	X_INLINE static  T	modf(T x, T *iptr)
	{
		double ival;
		T rval(::modf(double(x), &ival));
		*iptr = ival;
		return rval;
	}
	X_INLINE static  T	pow(T x, T y)				{ return ::pow(double(x), double(y)); }
	X_INLINE static  T	sqrt(T x)					{ return ::sqrt(double(x)); }
	X_INLINE static  T	cbrt(T x)					{ return (x > 0) ? (::pow(x, 1.0 / 3.0)) : (-::pow(-x, 1.0 / 3.0)); }
	X_INLINE static  T	ceil(T x)					{ return ::ceil(double(x)); }
	X_INLINE static  T	abs(T x)					{ return ::fabs(double(x)); }
	X_INLINE static  T	floor(T x)					{ return ::floor(double(x)); }
	X_INLINE static  T	round(T x)					{ return ::floor(double(x) + T(0.5)); }
	X_INLINE static  T	fmod(T x, T y)				{ return ::fmod(double(x), double(y)); }
	X_INLINE static  T	hypot(T x, T y)				{ return ::hypot(double(x), double(y)); }
	X_INLINE static  T	signum(T x)					{ return (x >0.0) ? 1.0 : ((x < 0.0) ? -1.0 : 0.0); }
	X_INLINE static  constexpr T	min(T x, T y)	{ return (x < y) ? x : y; }
	X_INLINE static  constexpr T	max(T x, T y)	{ return (x > y) ? x : y; }
	X_INLINE static  constexpr T	clamp(T x, T min = 0, T max = 1)	{ return (x < min) ? min : ((x > max) ? max : x); }

	X_INLINE static  void sincos(T x, T& s, T& c) { s = ::sin(double(x)); c = ::cos(double(x)); }

	X_INLINE static int32 isneg(T x)
	{
		union { float32_t f; uint32 i; } u;
		u.f = (float32_t)x;
		return (int32_t)(u.i >> 31);
	}
};


template<>
struct math<float>
{
	X_INLINE static  float  square(float x)		{ return x * x; }

	X_INLINE static  float	acos(float x)			{ return ::acosf(x); }
	X_INLINE static  float	asin(float x)			{ return ::asinf(x); }
	X_INLINE static  float	atan(float x)			{ return ::atanf(x); }
	X_INLINE static  float	atan2(float y, float x)	{ return ::atan2f(y, x); }
	X_INLINE static  float	cos(float x)			{ return ::cosf(x); }
	X_INLINE static  float	sin(float x)			{ return ::sinf(x); }
	X_INLINE static  float	tan(float x)			{ return ::tanf(x); }
	X_INLINE static  float	cosh(float x)			{ return ::coshf(x); }
	X_INLINE static  float	sinh(float x)			{ return ::sinhf(x); }
	X_INLINE static  float	tanh(float x)			{ return ::tanhf(x); }
	X_INLINE static  float	exp(float x)			{ return ::expf(x); }
	X_INLINE static  float	log(float x)			{ return ::logf(x); }
	X_INLINE static  float	log10(float x)			{ return ::log10f(x); }
	X_INLINE static  float	modf(float x, float *y)	{ return ::modff(x, y); }
	X_INLINE static  float	pow(float x, float y)	{ return ::powf(x, y); }
	X_DISABLE_WARNING(4756)
	X_INLINE static  float	sqrt(float x)			{ return ::sqrtf(x); }
	X_ENABLE_WARNING(4756)
	X_INLINE static  float	cbrt(float x)			{ return (x > 0) ? (::powf(x, 1.0f / 3.0f)) : (-::powf(-x, 1.0f / 3.0f)); }
	X_INLINE static  float	ceil(float x)			{ return ::ceilf(x); }
	X_INLINE static  float	abs(float x)			{ return ::fabsf(x); }
	X_INLINE static  float	floor(float x)			{ return ::floorf(x); }
	X_INLINE static  float	round(float x)			{ return ::floorf(float(x) + 0.5f); }
	X_INLINE static  float	fmod(float x, float y)	{ return ::fmodf(x, y); }
	X_INLINE static  float	hypot(float x, float y)	{ return ::sqrtf(x*x + y*y); }
	X_INLINE static  float	signum(float x)			{ return (x > 0.0f) ? 1.0f : ((x < 0.0f) ? -1.0f : 0.0f); }
	X_INLINE static  constexpr float	max(float x, float y)	{ return (x > y) ? x : y; }
	X_INLINE static  constexpr float	min(float x, float y)	{ return (x < y) ? x : y; }
	X_INLINE static  constexpr float	clamp(float x, float min = 0, float max = 1)	{ return (x < min) ? min : ((x > max) ? max : x); }

	X_INLINE static  void sincos(float x, float& s, float& c) { s = ::sinf(x); c = ::cosf(x); }

	X_INLINE static int32 isneg(float x) {
		union { float32_t f; uint32 i; } u;
		u.f = x;
		return (int32_t)(u.i >> 31);
	}

};


template<>
struct math<int>
{
	X_INLINE static  int	pow(int x, int y)			{ return ::pow(double(x), double(y)); }
	X_INLINE static  int	sqrt(int x)					{ return ::sqrt(double(x)); }
	X_INLINE static  int	cbrt(int x)					{ return (x > 0) ? (::pow(x, 1.0 / 3.0)) : (-::pow(-x, 1.0 / 3.0)); }
	X_INLINE static  int	ceil(int x)					{ return ::ceil(double(x)); }
	X_INLINE static  int	abs(int x)					{ return ::fabs(double(x)); }
	X_INLINE static  int	floor(int x)				{ return ::floor(double(x)); }
	X_INLINE static  constexpr int	  clamp(int x, int min = 0, int max = 1) { return (x < min) ? min : ((x > max) ? max : x); }

	X_INLINE static int32 isneg(int32 x) 
	{
		return (int32)((uint32)x >> 31);
	}

	X_INLINE static int min(int x, int y)
	{
		int diff = x - y;
		int mask = diff >> 31; // sign extend
		return (y & (~mask)) | (x & mask);
	}

	X_INLINE static constexpr int max(int x, int y)
	{
		return (x > y) ? x : y;
	}
};

X_ENABLE_WARNING(4244)

#ifndef X_PI
#define X_PI           3.14159265358979323846
#endif

const float64_t PI		= 3.1415926535897932384626433832;
const float64_t PI2		= 3.1415926535897932384626433832 * 2.0;
const float32_t PIf		= (float32_t)3.1415926535897932384626433832f;
const float32_t PI2f	= (float32_t)3.1415926535897932384626433832 * 2.0;
const float32_t PIHalff = (float32_t)3.1415926535897932384626433832 * 0.5f;
const float64_t PIHalf = (float64_t)3.1415926535897932384626433832 * 0.5;

const float32_t	Sqrt_1OVER2 = 0.70710678118654752440f;


const float64_t EPSILON_VALUE = 4.37114e-05;
const float32_t EPSILON_VALUEf = 4.37114e-05f;
#define EPSILON EPSILON_VALUE

static const float	INFINITY = 1e30f;

inline constexpr float fsel(float a, float b, float c)
{
	return a >= 0 ? b : c;
}

inline constexpr float toRadians(float x)
{
	return x * 0.017453292519943295769f; // ( x * PI / 180 )
}

inline constexpr double toRadians(double x)
{
	return x * 0.017453292519943295769; // ( x * PI / 180 )
}

inline constexpr float toDegrees(float x)
{
	return x * 57.295779513082321f; // ( x * 180 / PI )
}

inline constexpr double toDegrees(double x)
{
	return x * 57.295779513082321; // ( x * 180 / PI )
}


template <typename T> 
inline constexpr T divideByMultiple(T value, size_t alignment)
{
	return static_cast<T>((value + alignment - 1) / alignment);
}

template<typename T, typename L>
constexpr T lerp(const T &a, const T &b, L factor)
{
	return a + (b - a) * factor;
}

template<typename T>
constexpr T lmap(T val, T inMin, T inMax, T outMin, T outMax)
{
	return outMin + (outMax - outMin) * ((val - inMin) / (inMax - inMin));
}

template<typename T, typename L>
T bezierInterp(T a, T b, T c, T d, L t)
{
	L t1 = static_cast<L>(1.0) - t;
	return a*(t1*t1*t1) + b*(3 * t*t1*t1) + c*(3 * t*t*t1) + d*(t*t*t);
}

template<typename T, typename L>
T bezierInterpRef(const T &a, const T &b, const T &c, const T &d, L t)
{
	L t1 = static_cast<L>(1.0) - t;
	return a*(t1*t1*t1) + b*(3 * t*t1*t1) + c*(3 * t*t*t1) + d*(t*t*t);
}

template<typename T>
T constrain(T val, T minVal, T maxVal)
{
	if (val < minVal) return minVal;
	else if (val > maxVal) return maxVal;
	else return val;
}

// Don Hatch's version of sin(x)/x, which is accurate for very small x.
// Returns 1 for x == 0.
template <class T>
T sinx_over_x(T x)
{
	if (x * x < 1.19209290E-07F) {
		return T(1);
	}
	
	return math<T>::sin(x) / x;
}

// There are faster techniques for this, but this is portable
inline uint32_t log2floor(uint32_t x)
{
	uint32_t result = 0;
	while (x >>= 1) {
		++result;
	}

	return result;
}

inline uint32_t log2ceil(uint32_t x)
{
	uint32_t isNotPowerOf2 = (x & (x - 1));
	return (isNotPowerOf2) ? (log2floor(x) + 1) : log2floor(x);
}

inline uint32_t nextPowerOf2(uint32_t x)
{
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return(x + 1);
}

template<typename T>
inline int solveLinear(T a, T b, T result[1])
{
	if (a == 0) {
		return (b == 0 ? -1 : 0);
	}
	result[0] = -b / a;
	return 1;
}

template<typename T>
inline int solveQuadratic(T a, T b, T c, T result[2])
{
	if (a == 0) return solveLinear(b, c, result);

	T radical = b * b - 4 * a * c;
	if (radical < 0) return 0;

	if (radical == 0) {
		result[0] = -b / (2 * a);
		return 1;
	}

	T srad = math<T>::sqrt(radical);
	result[0] = (-b - srad) / (2 * a);
	result[1] = (-b + srad) / (2 * a);
	if (a < 0) {
		std::swap(result[0], result[1]);
	}
	return 2;
}

#endif // !_X_MATH_H_
