#include "stdafx.h"
#include "FloatImage.h"
#include "TextureFile.h"
#include "Filters.h"

X_NAMESPACE_BEGIN(texture)

namespace Converter
{

	namespace
	{

		X_INLINE int32_t wrapClamp(int32_t x, int32_t w)
		{
			return math<int32_t>::clamp(x, 0, w - 1);
		}
		
		X_INLINE int32_t wrapRepeat(int32_t x, int32_t w)
		{
			if (x >= 0) {
				return x % w;
			}
			return (x + 1) % w + w - 1;
		}

		X_INLINE int32_t wrapMirror(int32_t x, int32_t w)
		{
			if (w == 1) {
				x = 0;
			}

			x = math<int32_t>::abs(x);
			
			while (x >= w) {
				x = math<int32_t>::abs(w + w - x - 2);
			}

			return x;
		}
	
	} // namespace
	
	
	FloatImage::FloatImage(core::MemoryArenaBase* arena) :
		data_(arena)
	{

	}

	FloatImage::~FloatImage()
	{

	}

	void FloatImage::initFrom(const XTextureFile& img)
	{
		// init from rgb8 for now.
		if (img.getFormat() != Texturefmt::R8G8B8A8 || img.getFormat() != Texturefmt::A8R8G8B8) {
			return;
		}

		// one face.

		allocate(4, img.getWidth(), img.getHeight(), 1);

		float* pRed_channel = channel(0);
		float* pGreen_channel = channel(1);
		float* pBlue_channel = channel(2);
		float* pAlpha_channel = channel(3);

		// lets do each channel one by one.
		const uint8_t* pData = img.getLevel(0,0);

		const uint32_t count = 0;
		for (uint32_t i = 0; i < count; i++)
		{
			uint8_t red = pData[i * 4];
			pRed_channel[i] = static_cast<float>(red) / 255.f;
		}
		for (uint32_t i = 0; i < count; i++)
		{
			uint8_t green = pData[(i * 4) + 1];
			pGreen_channel[i] = static_cast<float>(green) / 255.f;
		}
		for (uint32_t i = 0; i < count; i++)
		{
			uint8_t blue = pData[(i * 4) + 2];
			pBlue_channel[i] = static_cast<float>(blue) / 255.f;
		}
		for (uint32_t i = 0; i < count; i++)
		{
			uint8_t alpha = pData[(i * 4) + 3];
			pAlpha_channel[i] = static_cast<float>(alpha) / 255.f;
		}
	}

	void FloatImage::allocate(uint32_t channels, uint32_t width, uint32_t height, uint32_t depth )
	{
		if (componentCount_!= channels || width_ != width || height_ != height || depth_ != depth)
		{
			free();

			width_ = width;
			height_ = height;
			depth_ = depth;
			componentCount_ = channels;
			pixelCount_ = width * height * depth;
			data_.resize(pixelCount_);
		}
	}

	void FloatImage::free(void)
	{
		data_.free();
	}

	void FloatImage::fastDownSample(void) const
	{

	}

	void FloatImage::downSample(const Filter& filter, WrapMode::Enum wm) const
	{
		const uint32_t w = core::Max(1, width_ / 2);
		const uint32_t h = core::Max(1, height_ / 2);
		const uint32_t d = core::Max(1, depth_ / 2);

		return resize(filter, w, h, d, wm);
	}

	void FloatImage::downSample(const Filter& filter, WrapMode::Enum wm, uint32_t alphaChannel) const
	{
		const uint32_t w = core::Max(1, width_ / 2);
		const uint32_t h = core::Max(1, height_ / 2);
		const uint32_t d = core::Max(1, depth_ / 2);

		return resize(filter, w, h, d, wm, alphaChannel);
	}

	void FloatImage::resize(const Filter& filter, uint32_t w, uint32_t h, WrapMode::Enum wm) const
	{

	}

	void FloatImage::resize(const Filter& filter, uint32_t w, uint32_t h, uint32_t d, WrapMode::Enum wm) const
	{

	}

	void FloatImage::resize(const Filter& filter, uint32_t w, uint32_t h, WrapMode::Enum wm, uint32_t alphaChannel) const
	{

	}

	void FloatImage::resize(const Filter& filter, uint32_t w, uint32_t h, uint32_t d, WrapMode::Enum wm, uint32_t alphaChannel) const
	{

	}



	float FloatImage::applyKernelXY(const Kernel2* pKernel, int32_t x, int32_t y, int32_t z, uint32_t c, WrapMode::Enum wm) const
	{
		X_ASSERT_NOT_NULL(pKernel);

		const uint32_t kernelWindow = pKernel->windowSize();
		const int32_t kernelOffset = int32_t(kernelWindow / 2);

		const float* pChannel = plane(c, z);

		float sum = 0.0f;
		for (uint32_t i = 0; i < kernelWindow; i++)
		{
			int32_t src_y = int32_t(y + i) - kernelOffset;

			for (uint32_t e = 0; e < kernelWindow; e++)
			{
				int32_t src_x = int32_t(x + e) - kernelOffset;
				int32_t idx = index(src_x, src_y, z, wm);

				sum += pKernel->valueAt(e, i) * pChannel[idx];
			}
		}

		return sum;
	}

	float FloatImage::applyKernelX(const Kernel1* pKernel, int32_t x, int32_t y, int32_t z, uint32_t c, WrapMode::Enum wm) const
	{
		X_ASSERT_NOT_NULL(pKernel);

		const uint32_t kernelWindow = pKernel->windowSize();
		const int32_t kernelOffset = int32_t(kernelWindow / 2);

		const float* pChannel = channel(c);

		float sum = 0.0f;
		for (uint32_t i = 0; i < kernelWindow; i++)
		{
			const int32_t src_x = int32_t(x + i) - kernelOffset;
			const int32_t idx = index(src_x, y, z, wm);

			sum += pKernel->valueAt(i) * pChannel[idx];
		}

		return sum;
	}

	float FloatImage::applyKernelY(const Kernel1* pKernel, int32_t x, int32_t y, int32_t z, uint32_t c, WrapMode::Enum wm) const
	{
		X_ASSERT_NOT_NULL(pKernel);

		const uint32_t kernelWindow = pKernel->windowSize();
		const int32_t kernelOffset = int32_t(kernelWindow / 2);

		const float* pChannel = channel(c);

		float sum = 0.0f;
		for (uint32_t i = 0; i < kernelWindow; i++)
		{
			const int32_t src_y = int32_t(y + i) - kernelOffset;
			const int32_t idx = index(x, src_y, z, wm);

			sum += pKernel->valueAt(i) * pChannel[idx];
		}

		return sum;
	}

	float FloatImage::applyKernelZ(const Kernel1* pKernel, int32_t x, int32_t y, int32_t z, uint32_t c, WrapMode::Enum wm) const
	{
		X_ASSERT_NOT_NULL(pKernel);

		const uint32_t kernelWindow = pKernel->windowSize();
		const int32_t kernelOffset = int32_t(kernelWindow / 2);

		const float* pChannel = channel(c);

		float sum = 0.0f;
		for (uint32_t i = 0; i < kernelWindow; i++)
		{
			const int32_t src_z = int32_t(z + i) - kernelOffset;
			const int32_t idx = index(x, y, src_z, wm);

			sum += pKernel->valueAt(i) * pChannel[idx];
		}

		return sum;
	}

	void FloatImage::applyKernelX(const PolyphaseKernel& k, int32_t y, int32_t z, uint32_t c, WrapMode::Enum wm, float* pOutput) const
	{
		const uint32_t length = k.length();
		const float scale = float(length) / float(width_);
		const float iscale = 1.0f / scale;

		const float width = k.width();
		const int32_t windowSize = k.windowSize();

		const float* pChannel = channel(c);

		for (uint32_t i = 0; i < length; i++)
		{
			const float center = (0.5f + i) * iscale;

			const int32_t left = static_cast<int32_t>(math<float>::floor(center - width));
			const int32_t right = static_cast<int32_t>(math<float>::ceil(center + width));
			X_ASSERT(right - left <= windowSize, "")(right, left, right - left, windowSize);

			float sum = 0;
			for (int32_t j = 0; j < windowSize; ++j)
			{
				const int32_t idx = index(left + j, y, z, wm);

				sum += k.valueAt(i, j) * pChannel[idx];
			}

			pOutput[i] = sum;
		}
	}

	void FloatImage::applyKernelY(const PolyphaseKernel& k, int32_t x, int32_t z, uint32_t c, WrapMode::Enum wm, float* pOutput) const
	{
		const uint32_t length = k.length();
		const float scale = float(length) / float(height_);
		const float iscale = 1.0f / scale;

		const float width = k.width();
		const int32_t windowSize = k.windowSize();

		const float* pChannel = channel(c);

		for (uint32_t i = 0; i < length; i++)
		{
			const float center = (0.5f + i) * iscale;

			const int32_t left = static_cast<int32_t>(math<float>::floor(center - width));
			const int32_t right = static_cast<int32_t>(math<float>::ceil(center + width));
			X_ASSERT(right - left <= windowSize, "")(right, left, right - left, windowSize);

			float sum = 0;
			for (int32_t j = 0; j < windowSize; ++j)
			{
				const int32_t idx = index(x, j + left, z, wm);
				sum += k.valueAt(i, j) * pChannel[idx];
			}

			pOutput[i] = sum;
		}
	}

	void FloatImage::applyKernelZ(const PolyphaseKernel& k, int32_t x, int32_t y, uint32_t c, WrapMode::Enum wm, float* pOutput) const
	{
		const uint32_t length = k.length();
		const float scale = float(length) / float(height_);
		const float iscale = 1.0f / scale;

		const float width = k.width();
		const int32_t windowSize = k.windowSize();

		const float* pChannel = channel(c);

		for (uint32_t i = 0; i < length; i++)
		{
			const float center = (0.5f + i) * iscale;

			const int32_t left = static_cast<int32_t>(math<float>::floor(center - width));
			const int32_t right = static_cast<int32_t>(math<float>::ceil(center + width));
			X_ASSERT(right - left <= windowSize, "")(right, left, right - left, windowSize);

			float sum = 0;
			for (int32_t j = 0; j < windowSize; ++j)
			{
				const int32_t idx = index(x, y, j + left, wm);
				sum += k.valueAt(i, j) * pChannel[idx];
			}

			pOutput[i] = sum;
		}
	}

	void FloatImage::applyKernelX(const PolyphaseKernel& k, int32_t y, int32_t z, uint32_t c, uint32_t a, WrapMode::Enum wm, float* pOutput) const
	{
		const uint32_t length = k.length();
		const float scale = float(length) / float(width_);
		const float iscale = 1.0f / scale;

		const float width = k.width();
		const int32_t windowSize = k.windowSize();

		const float* pChannel = channel(c);
		const float* pAlpha = channel(a);

		for (uint32_t i = 0; i < length; i++)
		{
			const float center = (0.5f + i) * iscale;

			const int32_t left = static_cast<int32_t>(math<float>::floor(center - width));
			const int32_t right = static_cast<int32_t>(math<float>::ceil(center + width));
			X_ASSERT(right - left <= windowSize, "")(right, left, right - left, windowSize);

			float norm = 0.0f;
			float sum = 0;
			for (int32_t j = 0; j < windowSize; ++j)
			{
				const int32_t idx = index(left + j, y, z, wm);

				float w = k.valueAt(i, j) * (pAlpha[idx] + (1.0f / 256.0f));
				norm += w;
				sum += w * pChannel[idx];
			}

			pOutput[i] = sum / norm;
		}
	}

	void FloatImage::applyKernelY(const PolyphaseKernel& k, int32_t x, int32_t z, uint32_t c, uint32_t a, WrapMode::Enum wm, float* pOutput) const
	{
		const uint32_t length = k.length();
		const float scale = float(length) / float(height_);
		const float iscale = 1.0f / scale;

		const float width = k.width();
		const int32_t windowSize = k.windowSize();

		const float* pChannel = channel(c);
		const float* pAlpha = channel(a);

		for (uint32_t i = 0; i < length; i++)
		{
			const float center = (0.5f + i) * iscale;

			const int32_t left = static_cast<int32_t>(math<float>::floor(center - width));
			const int32_t right = static_cast<int32_t>(math<float>::ceil(center + width));
			X_ASSERT(right - left <= windowSize, "")(right, left, right - left, windowSize);

			float norm = 0;
			float sum = 0;
			for (int32_t j = 0; j < windowSize; ++j)
			{
				const int32_t idx = index(x, j + left, z, wm);

				float w = k.valueAt(i, j) * (pAlpha[idx] + (1.0f / 256.0f));
				norm += w;
				sum += w * pChannel[idx];
			}

			pOutput[i] = sum / norm;
		}
	}

	void FloatImage::applyKernelZ(const PolyphaseKernel& k, int32_t x, int32_t y, uint32_t c, uint32_t a, WrapMode::Enum wm, float* pOutput) const
	{
		const uint32_t length = k.length();
		const float scale = float(length) / float(width_);
		const float iscale = 1.0f / scale;

		const float width = k.width();
		const int32_t windowSize = k.windowSize();

		const float* pChannel = channel(c);
		const float* pAlpha = channel(a);

		for (uint32_t i = 0; i < length; i++)
		{
			const float center = (0.5f + i) * iscale;

			const int32_t left = static_cast<int32_t>(math<float>::floor(center - width));
			const int32_t right = static_cast<int32_t>(math<float>::ceil(center + width));
			X_ASSERT(right - left <= windowSize, "")(right, left, right - left, windowSize);

			float norm = 0.0f;
			float sum = 0;
			for (int32_t j = 0; j < windowSize; ++j)
			{
				const int32_t idx = index(x, y, left + j, wm);

				float w = k.valueAt(i, j) * (pAlpha[idx] + (1.0f / 256.0f));
				norm += w;
				sum += w * pChannel[idx];
			}

			pOutput[i] = sum / norm;
		}
	}




	void FloatImage::flipX(void)
	{
		const uint32_t w = width_;
		const uint32_t h = height_;
		const uint32_t d = depth_;
		const uint32_t w2 = w / 2;

		for (uint32_t c = 0; c < componentCount_; c++) {
			for (uint32_t z = 0; z < d; z++) {
				for (uint32_t y = 0; y < h; y++) {
					float* pLine = scanline(c, y, z);
					for (uint32_t x = 0; x < w2; x++) {
						core::Swap(pLine[x], pLine[w - 1 - x]);
					}
				}
			}
		}
	}

	void FloatImage::flipY(void)
	{
		const uint32_t w = width_;
		const uint32_t h = height_;
		const uint32_t d = depth_;
		const uint32_t h2 = h / 2;

		for (uint32_t c = 0; c < componentCount_; c++) {
			for (uint32_t z = 0; z < d; z++) {
				for (uint32_t y = 0; y < h2; y++) {
					float* pSrc = scanline(c, y, z);
					float* pDst = scanline(c, h - 1 - y, z);
					for (uint32_t x = 0; x < w; x++) {
						core::Swap(pSrc[x], pDst[x]);
					}
				}
			}
		}
	}

	void FloatImage::flipZ(void)
	{
		const uint32_t w = width_;
		const uint32_t h = height_;
		const uint32_t d = depth_;
		const uint32_t d2 = d / 2;

		for (uint32_t c = 0; c < componentCount_; c++) {
			for (uint32_t z = 0; z < d2; z++) {
				float * pSrc = plane(c, z);
				float * pDst = plane(c, d - 1 - z);
				for (uint32_t i = 0; i < w*h; i++) {
					core::Swap(pSrc[i], pDst[i]);
				}
			}
		}
	}


	uint32_t FloatImage::index(int32_t x, int32_t y, int32_t z, WrapMode::Enum wm) const
	{
		if (wm == WrapMode::Clamp) {
			return indexClamp(x, y, z);
		}
		if (wm == WrapMode::Repeat) {
			return indexRepeat(x, y, z);
		}
		if (wm != WrapMode::Mirror) {
			X_ASSERT_UNREACHABLE();
		}

		return indexMirror(x, y, z);
	}

	uint32_t FloatImage::indexClamp(int32_t x, int32_t y, int32_t z) const
	{
		x = wrapClamp(x, width_);
		y = wrapClamp(y, height_);
		z = wrapClamp(z, depth_);
		return index(x, y, z);
	}

	uint32_t FloatImage::indexRepeat(int32_t x, int32_t y, int32_t z) const
	{
		x = wrapRepeat(x, width_);
		y = wrapRepeat(y, height_);
		z = wrapRepeat(z, depth_);
		return index(x, y, z);
	}
	
	uint32_t FloatImage::indexMirror(int32_t x, int32_t y, int32_t z) const
	{
		x = wrapMirror(x, width_);
		y = wrapMirror(y, height_);
		z = wrapMirror(z, depth_);
		return index(x, y, z);
	}

} // namespace Converter

X_NAMESPACE_END