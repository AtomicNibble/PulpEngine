#include "stdafx.h"
#include "Filters.h"

X_NAMESPACE_BEGIN(texture)

namespace Converter
{
    namespace
    {
        // Sinc function.
        X_INLINE static float sincf(const float x)
        {
            if (math<float>::abs(x) < EPSILON_VALUEf) {
                //return 1.0;
                return 1.0f + x * x * (-1.0f / 6.0f + x * x * 1.0f / 120.0f);
            }
            else {
                return math<float>::sin(x) / x;
            }
        }

        // Bessel function of the first kind from Jon Blow's article.
        // http://mathworld.wolfram.com/BesselFunctionoftheFirstKind.html
        // http://en.wikipedia.org/wiki/Bessel_function
        X_INLINE static float bessel0(float x)
        {
            const float EPSILON_RATIO = 1e-6f;
            float xh, sum, pow, ds;
            int32_t k;

            xh = 0.5f * x;
            sum = 1.0f;
            pow = 1.0f;
            k = 0;
            ds = 1.0;
            while (ds > sum * EPSILON_RATIO) {
                ++k;
                pow = pow * (xh / k);
                ds = pow * pow;
                sum = sum + ds;
            }

            return sum;
        }

    } // namespace

    Filter::Filter(float width) :
        width_(width)
    {
    }

    Filter::~Filter()
    {
    }

    float Filter::sampleDelta(float x, float scale) const
    {
        return evaluate((x + 0.5f) * scale);
    }

    float Filter::sampleBox(float x, float scale, int32_t samples) const
    {
        double sum = 0;
        float isamples = 1.0f / float(samples);

        for (int32_t s = 0; s < samples; s++) {
            float p = (x + (float(s) + 0.5f) * isamples) * scale;
            float value = evaluate(p);

            sum += value;
        }

        return float(sum * isamples);
    }

    float Filter::sampleTriangle(float x, float scale, int32_t samples) const
    {
        double sum = 0;
        float isamples = 1.0f / float(samples);

        for (int32_t s = 0; s < samples; s++) {
            float offset = (2 * float(s) + 1.0f) * isamples;
            float p = (x + offset - 0.5f) * scale;
            float value = evaluate(p);

            float weight = offset;
            if (weight > 1.0f) {
                weight = 2.0f - weight;
            }

            sum += value * weight;
        }

        return float(2 * sum * isamples);
    }

    // -------------------------------------------------------------------

    BoxFilter::BoxFilter() :
        Filter(0.5f)
    {
    }

    BoxFilter::BoxFilter(float width) :
        Filter(width)
    {
    }

    float BoxFilter::evaluate(float x) const
    {
        if (math<float>::abs(x) <= width_) {
            return 1.0f;
        }
        return 0.0f;
    }

    // -------------------------------------------------------------------

    TriangleFilter::TriangleFilter() :
        Filter(1.0f)
    {
    }

    TriangleFilter::TriangleFilter(float width) :
        Filter(width)
    {
    }

    float TriangleFilter::evaluate(float x) const
    {
        x = math<float>::abs(x);
        if (x < width_) {
            return width_ - x;
        }
        return 0.0f;
    }

    // -------------------------------------------------------------------

    QuadraticFilter::QuadraticFilter() :
        Filter(1.5f)
    {
    }

    float QuadraticFilter::evaluate(float x) const
    {
        x = math<float>::abs(x);
        if (x < 0.5f) {
            return 0.75f - x * x;
        }
        if (x < 1.5f) {
            float t = x - 1.5f;
            return 0.5f * t * t;
        }
        return 0.0f;
    }

    // -------------------------------------------------------------------

    CubicFilter::CubicFilter() :
        Filter(1.0f)
    {
    }

    float CubicFilter::evaluate(float x) const
    {
        // f(t) = 2|t|^3 - 3|t|^2 + 1, -1 <= t <= 1
        x = math<float>::abs(x);
        if (x < 1.0f) {
            return ((2.0f * x - 3.0f) * x * x + 1.0f);
        }
        return 0.0f;
    }
    // -------------------------------------------------------------------

    BSplineFilter::BSplineFilter() :
        Filter(2.0f)
    {
    }

    float BSplineFilter::evaluate(float x) const
    {
        x = math<float>::abs(x);
        if (x < 1.0f) {
            return (4.0f + x * x * (-6.0f + x * 3.0f)) / 6.0f;
        }
        if (x < 2.0f) {
            float t = 2.0f - x;
            return t * t * t / 6.0f;
        }
        return 0.0f;
    }

    // -------------------------------------------------------------------

    MitchellFilter::MitchellFilter() :
        Filter(2.0f)
    {
        setParameters(1.0f / 3.0f, 1.0f / 3.0f);
    }

    float MitchellFilter::evaluate(float x) const
    {
        x = math<float>::abs(x);
        if (x < 1.0f) {
            return p0_ + x * x * (p2_ + x * p3_);
        }
        if (x < 2.0f) {
            return q0_ + x * (q1_ + x * (q2_ + x * q3_));
        }
        return 0.0f;
    }

    void MitchellFilter::setParameters(float b, float c)
    {
        p0_ = (6.0f - 2.0f * b) / 6.0f;
        p2_ = (-18.0f + 12.0f * b + 6.0f * c) / 6.0f;
        p3_ = (12.0f - 9.0f * b - 6.0f * c) / 6.0f;
        q0_ = (8.0f * b + 24.0f * c) / 6.0f;
        q1_ = (-12.0f * b - 48.0f * c) / 6.0f;
        q2_ = (6.0f * b + 30.0f * c) / 6.0f;
        q3_ = (-b - 6.0f * c) / 6.0f;
    }

    // -------------------------------------------------------------------

    LanczosFilter::LanczosFilter() :
        Filter(3.0f)
    {
    }

    float LanczosFilter::evaluate(float x) const
    {
        x = math<float>::abs(x);
        if (x < 3.0f) {
            return sincf(PIf * x) * sincf(PIf * x / 3.0f);
        }
        return 0.0f;
    }
    // -------------------------------------------------------------------

    SincFilter::SincFilter(float w) :
        Filter(w)
    {
    }

    float SincFilter::evaluate(float x) const
    {
        return sincf(PIf * x);
    }

    // -------------------------------------------------------------------

    KaiserFilter::KaiserFilter(float w) :
        Filter(w)
    {
        setParameters(4.0f, 1.0f);
    }

    float KaiserFilter::evaluate(float x) const
    {
        const float sinc_value = sincf(PIf * x * stretch_);
        const float t = x / width_;

        if ((1.f - t * t) >= 0) {
            return sinc_value * bessel0(alpha_ * math<float>::sqrt(1.f - t * t)) / bessel0(alpha_);
        }

        return 0.f;
    }

    void KaiserFilter::setParameters(float alpha, float stretch)
    {
        alpha_ = alpha;
        stretch_ = stretch;
    }

    // -------------------------------------------------------------------

    GaussianFilter::GaussianFilter(float w) :
        Filter(w)
    {
        setParameters(1);
    }

    float GaussianFilter::evaluate(float x) const
    {
        // variance = sigma^2
        return (1.0f / sqrtf(2.f * PIf * variance_)) * expf(-x * x / (2.f * variance_));
    }

    void GaussianFilter::setParameters(float variance)
    {
        variance_ = variance;
    }

    // -------------------------------------------------------------------

    Kernel1::Kernel1(core::MemoryArenaBase* arena, const Filter& f, int32_t iscale, int32_t samples /*= 32*/) :
        data_(arena)
    {
        X_ASSERT(iscale > 1, "Scale must be positive")(iscale); 
        X_ASSERT(samples > 0, "Samples must be greater than zero")(samples); 

        const float scale = 1.0f / iscale;

        width_ = f.width() * iscale;
        windowSize_ = static_cast<int32_t>(math<float>::ceil(2 * width_));
        data_.resize(windowSize_);

        const float offset = float(windowSize_) / 2;

        float total = 0.0f;
        for (int32_t i = 0; i < windowSize_; i++) {
            const float sample = f.sampleBox(i - offset, scale, samples);
            data_[i] = sample;
            total += sample;
        }

        const float inv = 1.0f / total;
        for (int32_t i = 0; i < windowSize_; i++) {
            data_[i] *= inv;
        }
    }

    Kernel1::~Kernel1()
    {
    }

    // Print the kernel for debugging purposes.
    void Kernel1::debugPrint(void) const
    {
        for (int32_t i = 0; i < windowSize_; i++) {
            X_LOG0("FilterKern", "%d: %f\n", i, data_[i]);
        }
    }

    // -------------------------------------------------------------------

    Kernel2::Kernel2(core::MemoryArenaBase* arena, uint32_t ws) :
        windowSize_(ws),
        data_(arena, windowSize_ * windowSize_)
    {
    }

    Kernel2::Kernel2(core::MemoryArenaBase* arena, uint32_t ws, const float* pData) :
        windowSize_(ws),
        data_(arena, windowSize_ * windowSize_)
    {
        std::memcpy(data_.data(), pData, sizeof(float) * windowSize_ * windowSize_);
    }

    Kernel2::Kernel2(const Kernel2& oth) :
        windowSize_(oth.windowSize_),
        data_(oth.data_)
    {
    }

    Kernel2::~Kernel2()
    {
    }

    // Normalize the filter.
    void Kernel2::normalize(void)
    {
        float total = 0.0f;
        for (uint32_t i = 0; i < windowSize_ * windowSize_; i++) {
            total += math<float>::abs(data_[i]);
        }

        const float inv = 1.0f / total;
        for (uint32_t i = 0; i < windowSize_ * windowSize_; i++) {
            data_[i] *= inv;
        }
    }

    // Transpose the kernel.
    void Kernel2::transpose(void)
    {
        for (uint32_t i = 0; i < windowSize_; i++) {
            for (uint32_t j = i + 1; j < windowSize_; j++) {
                core::Swap(data_[i * windowSize_ + j], data_[j * windowSize_ + i]);
            }
        }
    }

    // Init laplacian filter, usually used for sharpening.
    void Kernel2::initLaplacian(void)
    {
        X_ASSERT(windowSize_ == 3, "Window size must be 3")(windowSize_); 

        //	data_[0] = -1; data_[1] = -1; data_[2] = -1;
        //	data_[3] = -1; data_[4] = +8; data_[5] = -1;
        //	data_[6] = -1; data_[7] = -1; data_[8] = -1;

        data_[0] = +0;
        data_[1] = -1;
        data_[2] = +0;
        data_[3] = -1;
        data_[4] = +4;
        data_[5] = -1;
        data_[6] = +0;
        data_[7] = -1;
        data_[8] = +0;

        //	data_[0] = +1; data_[1] = -2; data_[2] = +1;
        //	data_[3] = -2; data_[4] = +4; data_[5] = -2;
        //	data_[6] = +1; data_[7] = -2; data_[8] = +1;
    }

    // Init simple edge detection filter.
    void Kernel2::initEdgeDetection(void)
    {
        X_ASSERT(windowSize_ == 3, "Window size must be 3")(windowSize_); 

        data_[0] = 0;
        data_[1] = 0;
        data_[2] = 0;
        data_[3] = -1;
        data_[4] = 0;
        data_[5] = 1;
        data_[6] = 0;
        data_[7] = 0;
        data_[8] = 0;
    }

    // Init sobel filter.
    void Kernel2::initSobel(void)
    {
        if (windowSize_ == 3) {
            data_[0] = -1;
            data_[1] = 0;
            data_[2] = 1;
            data_[3] = -2;
            data_[4] = 0;
            data_[5] = 2;
            data_[6] = -1;
            data_[7] = 0;
            data_[8] = 1;
        }
        else if (windowSize_ == 5) {
            float elements[] = {
                -1, -2, 0, 2, 1,
                -2, -3, 0, 3, 2,
                -3, -4, 0, 4, 3,
                -2, -3, 0, 3, 2,
                -1, -2, 0, 2, 1};

            for (int32_t i = 0; i < 5 * 5; i++) {
                data_[i] = elements[i];
            }
        }
        else if (windowSize_ == 7) {
            float elements[] = {
                -1, -2, -3, 0, 3, 2, 1,
                -2, -3, -4, 0, 4, 3, 2,
                -3, -4, -5, 0, 5, 4, 3,
                -4, -5, -6, 0, 6, 5, 4,
                -3, -4, -5, 0, 5, 4, 3,
                -2, -3, -4, 0, 4, 3, 2,
                -1, -2, -3, 0, 3, 2, 1};

            for (int32_t i = 0; i < 7 * 7; i++) {
                data_[i] = elements[i];
            }
        }
        else if (windowSize_ == 9) {
            float elements[] = {
                -1, -2, -3, -4, 0, 4, 3, 2, 1,
                -2, -3, -4, -5, 0, 5, 4, 3, 2,
                -3, -4, -5, -6, 0, 6, 5, 4, 3,
                -4, -5, -6, -7, 0, 7, 6, 5, 4,
                -5, -6, -7, -8, 0, 8, 7, 6, 5,
                -4, -5, -6, -7, 0, 7, 6, 5, 4,
                -3, -4, -5, -6, 0, 6, 5, 4, 3,
                -2, -3, -4, -5, 0, 5, 4, 3, 2,
                -1, -2, -3, -4, 0, 4, 3, 2, 1};

            for (int32_t i = 0; i < 9 * 9; i++) {
                data_[i] = elements[i];
            }
        }
    }

    // Init prewitt filter.
    void Kernel2::initPrewitt(void)
    {
        if (windowSize_ == 3) {
            data_[0] = -1;
            data_[1] = 0;
            data_[2] = -1;
            data_[3] = -1;
            data_[4] = 0;
            data_[5] = -1;
            data_[6] = -1;
            data_[7] = 0;
            data_[8] = -1;
        }
        else if (windowSize_ == 5) {
            // @@ Is this correct?
            float elements[] = {
                -2, -1, 0, 1, 2,
                -2, -1, 0, 1, 2,
                -2, -1, 0, 1, 2,
                -2, -1, 0, 1, 2,
                -2, -1, 0, 1, 2};

            for (int32_t i = 0; i < 5 * 5; i++) {
                data_[i] = elements[i];
            }
        }
    }

    // Init blended sobel filter.
    void Kernel2::initBlendedSobel(const Vec4f& scale)
    {
        X_ASSERT(windowSize_ == 9, "Window size must be 9")(windowSize_); 

        {
            const float elements[] = {
                -1, -2, -3, -4, 0, 4, 3, 2, 1,
                -2, -3, -4, -5, 0, 5, 4, 3, 2,
                -3, -4, -5, -6, 0, 6, 5, 4, 3,
                -4, -5, -6, -7, 0, 7, 6, 5, 4,
                -5, -6, -7, -8, 0, 8, 7, 6, 5,
                -4, -5, -6, -7, 0, 7, 6, 5, 4,
                -3, -4, -5, -6, 0, 6, 5, 4, 3,
                -2, -3, -4, -5, 0, 5, 4, 3, 2,
                -1, -2, -3, -4, 0, 4, 3, 2, 1};

            for (int32_t i = 0; i < 9 * 9; i++) {
                data_[i] = elements[i] * scale.w;
            }
        }
        {
            const float elements[] = {
                -1,
                -2,
                -3,
                0,
                3,
                2,
                1,
                -2,
                -3,
                -4,
                0,
                4,
                3,
                2,
                -3,
                -4,
                -5,
                0,
                5,
                4,
                3,
                -4,
                -5,
                -6,
                0,
                6,
                5,
                4,
                -3,
                -4,
                -5,
                0,
                5,
                4,
                3,
                -2,
                -3,
                -4,
                0,
                4,
                3,
                2,
                -1,
                -2,
                -3,
                0,
                3,
                2,
                1,
            };

            for (int32_t i = 0; i < 7; i++) {
                for (int32_t e = 0; e < 7; e++) {
                    data_[(i + 1) * 9 + e + 1] += elements[i * 7 + e] * scale.z;
                }
            }
        }
        {
            const float elements[] = {
                -1, -2, 0, 2, 1,
                -2, -3, 0, 3, 2,
                -3, -4, 0, 4, 3,
                -2, -3, 0, 3, 2,
                -1, -2, 0, 2, 1};

            for (int32_t i = 0; i < 5; i++) {
                for (int32_t e = 0; e < 5; e++) {
                    data_[(i + 2) * 9 + e + 2] += elements[i * 5 + e] * scale.y;
                }
            }
        }
        {
            const float elements[] = {
                -1,
                0,
                1,
                -2,
                0,
                2,
                -1,
                0,
                1,
            };

            for (int32_t i = 0; i < 3; i++) {
                for (int32_t e = 0; e < 3; e++) {
                    data_[(i + 3) * 9 + e + 3] += elements[i * 3 + e] * scale.x;
                }
            }
        }
    }

    // -------------------------------------------------------------------

    PolyphaseKernel::PolyphaseKernel(core::MemoryArenaBase* arena, const Filter& f,
        uint32_t srcLength, uint32_t dstLength, int32_t samples /*= 32*/) :
        data_(arena)
    {
        X_ASSERT(samples > 0, "Samples must be greater than zero")(samples); 

        float scale = float(dstLength) / float(srcLength);
        const float iscale = 1.0f / scale;

        if (scale > 1) {
            // Upsampling.
            samples = 1;
            scale = 1;
        }

        length_ = dstLength;
        width_ = f.width() * iscale;
        windowSize_ = static_cast<int32_t>(math<float>::ceil(width_ * 2) + 1);

        data_.resize(windowSize_ * length_);

        for (uint32_t i = 0; i < length_; i++) {
            const float center = (0.5f + i) * iscale;

            const int32_t left = static_cast<int32_t>(math<float>::floor(center - width_));
            const int32_t right = static_cast<int32_t>(math<float>::ceil(center + width_));
            X_ASSERT(right - left <= windowSize_, "Range is bigger than window size")(right, left, right - left, windowSize_); 

            float total = 0.0f;
            for (int32_t j = 0; j < windowSize_; j++) {
                const float sample = f.sampleBox(left + j - center, scale, samples);

                data_[i * windowSize_ + j] = sample;
                total += sample;
            }

            // normalize weights.
            for (int32_t j = 0; j < windowSize_; j++) {
                data_[i * windowSize_ + j] /= total;
            }
        }
    }

    PolyphaseKernel::~PolyphaseKernel()
    {
    }

    void PolyphaseKernel::debugPrint(void) const
    {
        for (uint32_t i = 0; i < length_; i++) {
            X_LOG0("PolyKern", "%d: ", i);
            for (int32_t j = 0; j < windowSize_; j++) {
                X_LOG0("PolyKern", " %6.4f", data_[i * windowSize_ + j]);
            }
        }
    }

} // namespace Converter

X_NAMESPACE_END