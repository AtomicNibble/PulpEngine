#pragma once

X_NAMESPACE_BEGIN(texture)

namespace Converter
{

	class Filter
	{
	public:
		explicit Filter(float width);
		virtual ~Filter();

		X_INLINE float width(void) const;
		float sampleDelta(float x, float scale) const;
		float sampleBox(float x, float scale, int samples) const;
		float sampleTriangle(float x, float scale, int samples) const;

		virtual float evaluate(float x) const X_ABSTRACT;

	protected:
		const float width_;
	};


	// Box filter.
	class BoxFilter : public Filter
	{
	public:
		BoxFilter();
		explicit BoxFilter(float width);

		virtual float evaluate(float x) const X_OVERRIDE;
	};

	// Triangle (bilinear/tent) filter.
	class TriangleFilter : public Filter
	{
	public:
		TriangleFilter();
		explicit TriangleFilter(float width);

		virtual float evaluate(float x) const X_OVERRIDE;
	};

	// Quadratic (bell) filter.
	class QuadraticFilter : public Filter
	{
	public:
		QuadraticFilter();

		virtual float evaluate(float x) const X_OVERRIDE;
	};

	// Cubic filter from Thatcher Ulrich.
	class CubicFilter : public Filter
	{
	public:
		CubicFilter();

		virtual float evaluate(float x) const X_OVERRIDE;
	};

	// Cubic b-spline filter from Paul Heckbert.
	class BSplineFilter : public Filter
	{
	public:
		BSplineFilter();

		virtual float evaluate(float x) const X_OVERRIDE;
	};

	/// Mitchell & Netravali's two-param cubic
	/// @see "Reconstruction Filters in Computer Graphics", SIGGRAPH 88
	class MitchellFilter : public Filter
	{
	public:
		MitchellFilter();

		virtual float evaluate(float x) const X_OVERRIDE;

		void setParameters(float b, float c);

	private:
		float p0_, p2_, p3_;
		float q0_, q1_, q2_, q3_;
	};

	// Lanczos3 filter.
	class LanczosFilter : public Filter
	{
	public:
		LanczosFilter();

		virtual float evaluate(float x) const X_OVERRIDE;
	};

	// Sinc filter.
	class SincFilter : public Filter
	{
	public:
		explicit SincFilter(float w);

		virtual float evaluate(float x) const X_OVERRIDE;
	};

	// Kaiser filter.
	class KaiserFilter : public Filter
	{
	public:
		explicit KaiserFilter(float w);

		virtual float evaluate(float x) const X_OVERRIDE;

		void setParameters(float a, float stretch);

	private:
		float alpha_;
		float stretch_;
	};

	// Gaussian filter.
	class GaussianFilter : public Filter
	{
	public:
		explicit GaussianFilter(float w);

		virtual float evaluate(float x) const X_OVERRIDE;

		void setParameters(float variance);

	private:
		float variance_;
	};




} // namespace Converter

X_NAMESPACE_END

#include "Filters.inl"