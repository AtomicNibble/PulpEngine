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
				return 1.0f + x*x*(-1.0f / 6.0f + x*x*1.0f / 120.0f);
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
			int k;

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
		return evaluate((x + 0.5f)* scale);
	}

	float Filter::sampleBox(float x, float scale, int samples) const
	{
		double sum = 0;
		float isamples = 1.0f / float(samples);

		for (int s = 0; s < samples; s++)
		{
			float p = (x + (float(s) + 0.5f) * isamples) * scale;
			float value = evaluate(p);

			sum += value;
		}

		return float(sum * isamples);
	}

	float Filter::sampleTriangle(float x, float scale, int samples) const
	{
		double sum = 0;
		float isamples = 1.0f / float(samples);

		for (int s = 0; s < samples; s++)
		{
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
			return((2.0f * x - 3.0f) * x * x + 1.0f);
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
		alpha = alpha;
		stretch = stretch;
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
		return (1.0f / sqrtf(2.f * PIf * variance_)) * expf(-x*x / (2.f * variance_));
	}

	void GaussianFilter::setParameters(float variance)
	{
		variance_ = variance;
	}




} // namespace

X_NAMESPACE_END