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
    static constexpr T EPSILON = T(0);

    X_INLINE static constexpr T square(T x)
    {
        return x * x;
    }
    X_INLINE static constexpr T min(T x, T y)
    {
        return (x < y) ? x : y;
    }
    X_INLINE static constexpr T max(T x, T y)
    {
        return (x > y) ? x : y;
    }
    X_INLINE static constexpr T clamp(T x, T min = 0, T max = 1)
    {
        return (x < min) ? min : ((x > max) ? max : x);
    }
};

template<>
struct math<double>
{
    // clang-format off
    static constexpr float64_t CMP_EPSILON = 4.37114e-05; // used for comparisions, not reliable but faster than dynamic.
    static constexpr float64_t EPSILON = 2.2204460492503131e-016;
    static constexpr float64_t INFINITY = std::numeric_limits<double>::infinity();

    static constexpr float64_t PI = 3.1415926535897932384626433832;
    static constexpr float64_t TWO_PI = 2.0 * PI;
    static constexpr float64_t HALF_PI = 0.5 * PI;
    static constexpr float64_t ONE_FOURTH_PI = 0.25 * PI;
    static constexpr float64_t ONE_OVER_PI = 1 / PI;
    static constexpr float64_t ONE_OVER_TWO_PI = 1 / TWO_PI;
    static constexpr float64_t SQRT_TWO = 1.41421356237309504880;
    static constexpr float64_t SQRT_THREE = 1.73205080756887729352;
    static constexpr float64_t SQRT_1OVER2 = 0.70710678118654752440;
    static constexpr float64_t SQRT_1OVER3 = 0.57735026918962576450;
    static constexpr float64_t MUL_DEG2RAD = PI / 180;
    static constexpr float64_t MUL_RAD2DEG = 180 / PI;

    // clang-format on
    X_INLINE static double square(double x)
    {
        return x * x;
    }

    X_INLINE static double acos(double x)
    {
        return ::acos(x);
    }
    X_INLINE static double asin(double x)
    {
        return ::asin(x);
    }
    X_INLINE static double atan(double x)
    {
        return ::atan(x);
    }
    X_INLINE static double atan2(double y, double x)
    {
        return ::atan2(y, x);
    }
    X_INLINE static double cos(double x)
    {
        return ::cos(x);
    }
    X_INLINE static double sin(double x)
    {
        return ::sin(x);
    }
    X_INLINE static double tan(double x)
    {
        return ::tan(x);
    }
    X_INLINE static double cosh(double x)
    {
        return ::cosh(x);
    }
    X_INLINE static double sinh(double x)
    {
        return ::sinh(x);
    }
    X_INLINE static double tanh(double x)
    {
        return ::tanh(x);
    }
    X_INLINE static double exp(double x)
    {
        return ::exp(x);
    }
    X_INLINE static double log(double x)
    {
        return ::log(x);
    }
    X_INLINE static double log10(double x)
    {
        return ::log10(x);
    }
    X_INLINE static double modf(double x, double* y)
    {
        return ::modf(x, y);
    }
    X_INLINE static double pow(double x, double y)
    {
        return ::pow(x, y);
    }
    X_DISABLE_WARNING(4756)
    X_INLINE static double sqrt(double x)
    {
        return ::sqrt(x);
    }
    X_ENABLE_WARNING(4756)
    X_INLINE static double ceil(double x)
    {
        return ::ceil(x);
    }
    X_INLINE static double abs(double x)
    {
        return ::abs(x);
    }
    X_INLINE static double frac(double x)
    {
        return x - floor(x);
    }
    X_INLINE static double floor(double x)
    {
        return ::floor(x);
    }
    X_INLINE static double round(double x)
    {
        return ::floor(double(x) + 0.5);
    }
    X_INLINE static double fmod(double x, double y)
    {
        return ::fmod(x, y);
    }
    X_INLINE static double hypot(double x, double y)
    {
        return ::sqrt(x * x + y * y);
    }
    X_INLINE static double signum(double x)
    {
        return (x > 0.0) ? 1.0 : ((x < 0.0) ? -1.0 : 0.0);
    }
    X_INLINE static constexpr double max(double x, double y)
    {
        return (x > y) ? x : y;
    }
    X_INLINE static constexpr double min(double x, double y)
    {
        return (x < y) ? x : y;
    }
    X_INLINE static constexpr double clamp(double x, double min = 0, double max = 1)
    {
        return (x < min) ? min : ((x > max) ? max : x);
    }
    X_INLINE static constexpr double saturate(double val)
    {
        return (val < 0.0) ? 0.0 : (val > 1.0) ? 1.0 : val;
    }

    X_INLINE static void sincos(double x, double& s, double& c)
    {
        s = ::sin(x);
        c = ::cos(x);
    }

};

template<>
struct math<float>
{
    // clang-format off
    static constexpr float32_t CMP_EPSILON = 4.37114e-05f; // used for comparisions, not reliable but faster than dynamic.
    static constexpr float32_t EPSILON = 1.192092896e-07f;
    static constexpr float32_t INFINITY = 1e30f;

    static constexpr float32_t PI = 3.1415926535897932384626433832f;
    static constexpr float32_t TWO_PI = 2.0f * PI;
    static constexpr float32_t HALF_PI = 0.5f * PI;
    static constexpr float32_t ONE_FOURTH_PI = 0.25f * PI;
    static constexpr float32_t ONE_OVER_PI = 1.f / PI;
    static constexpr float32_t ONE_OVER_TWO_PI = 1.f / TWO_PI;
    static constexpr float32_t SQRT_TWO = 1.41421356237309504880f;
    static constexpr float32_t SQRT_THREE = 1.73205080756887729352f;
    static constexpr float32_t SQRT_1OVER2 = 0.70710678118654752440f;
    static constexpr float32_t SQRT_1OVER3 = 0.57735026918962576450f;
    static constexpr float32_t MUL_DEG2RAD = PI / 180.f;
    static constexpr float32_t MUL_RAD2DEG = 180.f / PI;

    // clang-format on

    X_INLINE static float square(float x)
    {
        return x * x;
    }

    X_INLINE static float acos(float x)
    {
        return ::acosf(x);
    }
    X_INLINE static float asin(float x)
    {
        return ::asinf(x);
    }
    X_INLINE static float atan(float x)
    {
        return ::atanf(x);
    }
    X_INLINE static float atan2(float y, float x)
    {
        return ::atan2f(y, x);
    }
    X_INLINE static float cos(float x)
    {
        return ::cosf(x);
    }
    X_INLINE static float sin(float x)
    {
        return ::sinf(x);
    }
    X_INLINE static float tan(float x)
    {
        return ::tanf(x);
    }
    X_INLINE static float cosh(float x)
    {
        return ::coshf(x);
    }
    X_INLINE static float sinh(float x)
    {
        return ::sinhf(x);
    }
    X_INLINE static float tanh(float x)
    {
        return ::tanhf(x);
    }
    X_INLINE static float exp(float x)
    {
        return ::expf(x);
    }
    X_INLINE static float log(float x)
    {
        return ::logf(x);
    }
    X_INLINE static float log10(float x)
    {
        return ::log10f(x);
    }
    X_INLINE static float modf(float x, float* y)
    {
        return ::modff(x, y);
    }
    X_INLINE static float pow(float x, float y)
    {
        return ::powf(x, y);
    }
    X_DISABLE_WARNING(4756)
    X_INLINE static float sqrt(float x)
    {
        return ::sqrtf(x);
    }
    X_ENABLE_WARNING(4756)
    X_INLINE static float cbrt(float x)
    {
        return (x > 0) ? (::powf(x, 1.0f / 3.0f)) : (-::powf(-x, 1.0f / 3.0f));
    }
    X_INLINE static float ceil(float x)
    {
        return ::ceilf(x);
    }
    X_INLINE static float abs(float x)
    {
        return ::fabsf(x);
    }
    X_INLINE static float frac(float x)
    {
        return x - floor(x);
    }
    X_INLINE static float floor(float x)
    {
        return ::floorf(x);
    }
    X_INLINE static float round(float x)
    {
        return ::floorf(float(x) + 0.5f);
    }
    X_INLINE static float fmod(float x, float y)
    {
        return ::fmodf(x, y);
    }
    X_INLINE static float hypot(float x, float y)
    {
        return ::sqrtf(x * x + y * y);
    }
    X_INLINE static float signum(float x)
    {
        return (x > 0.0f) ? 1.0f : ((x < 0.0f) ? -1.0f : 0.0f);
    }
    X_INLINE static constexpr float max(float x, float y)
    {
        return (x > y) ? x : y;
    }
    X_INLINE static constexpr float min(float x, float y)
    {
        return (x < y) ? x : y;
    }
    X_INLINE static constexpr float clamp(float x, float min = 0, float max = 1)
    {
        return (x < min) ? min : ((x > max) ? max : x);
    }
    X_INLINE static constexpr float saturate(float val)
    {
        return (val < 0.0f) ? 0.0f : (val > 1.0f) ? 1.0f : val;
    }

    X_INLINE static void sincos(float x, float& s, float& c)
    {
        s = ::sinf(x);
        c = ::cosf(x);
    }

    X_INLINE static int32 isneg(float x)
    {
        union
        {
            float32_t f;
            uint32 i;
        } u;
        u.f = x;
        return (int32_t)(u.i >> 31);
    }
};

template<>
struct math<int>
{
    X_INLINE static int pow(int x, int y)
    {
        return std::pow(double(x), double(y));
    }
    X_INLINE static int sqrt(int x)
    {
        return std::sqrt(x);
    }
    X_INLINE static int cbrt(int x)
    {
        return (x > 0) ? (::pow(x, 1.0 / 3.0)) : (-::pow(-x, 1.0 / 3.0));
    }
    X_INLINE static int ceil(int x)
    {
        return std::ceil(x);
    }
    X_INLINE static int abs(int x)
    {
        return std::abs(x);
    }
    X_INLINE static int floor(int x)
    {
        return std::floor(x);
    }
    X_INLINE static constexpr int clamp(int x, int min = 0, int max = 1)
    {
        return (x < min) ? min : ((x > max) ? max : x);
    }
  
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

template<>
struct math<int16_t>
{
    X_INLINE static int16_t abs(int16_t x)
    {
        return std::abs(x);
    }
};

template<>
struct math<int64_t>
{
    X_INLINE static int64_t abs(int64_t x)
    {
        return std::abs(x);
    }
};

X_ENABLE_WARNING(4244)

#ifndef X_PI
#define X_PI 3.14159265358979323846
#endif

inline constexpr float fsel(float a, float b, float c)
{
    return a >= 0 ? b : c;
}

inline constexpr float toRadians(float x)
{
    return x * math<float>::MUL_DEG2RAD;
}

inline constexpr double toRadians(double x)
{
    return x * math<double>::MUL_DEG2RAD;
}

inline constexpr float toDegrees(float x)
{
    return x * math<float>::MUL_RAD2DEG;
}

inline constexpr double toDegrees(double x)
{
    return x * math<double>::MUL_RAD2DEG;
}

template<typename T>
inline constexpr T divideByMultiple(T value, size_t alignment)
{
    return static_cast<T>((value + alignment - 1) / alignment);
}

template<typename T, typename L>
constexpr T lerp(const T& a, const T& b, L factor)
{
    return a + (b - a) * factor;
}

template<>
constexpr int lerp(const int& a, const int& b, float factor)
{
    return static_cast<int>(static_cast<float>(a) + (static_cast<float>(b) - static_cast<float>(a)) * factor);
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
    return a * (t1 * t1 * t1) + b * (3 * t * t1 * t1) + c * (3 * t * t * t1) + d * (t * t * t);
}

template<typename T, typename L>
T bezierInterpRef(const T& a, const T& b, const T& c, const T& d, L t)
{
    L t1 = static_cast<L>(1.0) - t;
    return a * (t1 * t1 * t1) + b * (3 * t * t1 * t1) + c * (3 * t * t * t1) + d * (t * t * t);
}

template<typename T>
T constrain(T val, T minVal, T maxVal)
{
    if (val < minVal) {
        return minVal;
    }
    else if (val > maxVal) {
        return maxVal;
    }

    return val;
}

// Don Hatch's version of sin(x)/x, which is accurate for very small x.
// Returns 1 for x == 0.
template<class T>
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
    return (x + 1);
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
    if (a == 0) {
        return solveLinear(b, c, result);
    }

    T radical = b * b - 4 * a * c;
    if (radical < 0) {
        return 0;
    }

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
