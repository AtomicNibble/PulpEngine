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

	void FloatImage::initFrom(const XTextureFile& img, int32_t face, int32_t mip)
	{
		// init from rgb8 for now.
		if (img.getFormat() != Texturefmt::R8G8B8A8 && img.getFormat() != Texturefmt::B8G8R8A8) {
			return;
		}

		// one face.
		if (face != 0 && mip != 0) {
			// alloc logic needs doing.
			X_ASSERT_NOT_IMPLEMENTED();
			return;
		}

		allocate(4, img.getWidth(), img.getHeight(), 1);

		float* pRed_channel = channel(0);
		float* pGreen_channel = channel(1);
		float* pBlue_channel = channel(2);
		float* pAlpha_channel = channel(3);

		// lets do each channel one by one.
		const uint8_t* pData = img.getLevel(face,mip);

		const uint32_t count = pixelCount_;
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

	void FloatImage::allocate(uint32_t channels, uint32_t width, uint32_t height, uint32_t depth)
	{
		if (componentCount_!= channels || width_ != width || height_ != height || depth_ != depth)
		{
			free();

			width_ = width;
			height_ = height;
			depth_ = depth;
			componentCount_ = channels;
			pixelCount_ = width * height * depth;
			data_.resize(getAllocationSize(channels, width, height, depth));
		}
	}

	void FloatImage::free(void)
	{
		data_.free();
	}

	void FloatImage::fastDownSample(FloatImage& dst) const
	{
		const uint32_t newWidth = core::Max(1, width_ / 2);
		const uint32_t newHeight = core::Max(1, height_ / 2);
		dst.allocate(componentCount_, newWidth, newHeight);


		// 1D box filter.
		if (width_ == 1 || height_ == 1)
		{
			const uint32_t num = newWidth * newHeight;

			if ((width_ * height_) & 1)
			{
				const float scale = 1.0f / (2 * num + 1);

				for (uint32_t c = 0; c < componentCount_; c++)
				{
					const float* pSrc = channel(c);
					float* pDst = dst.channel(c);

					for (uint32_t x = 0; x < num; x++)
					{
						const float w1 = float(num - 0);
						const float w0 = float(num - x);
						const float w2 = float(1 + x);

						*pDst++ = scale * (w0 * pSrc[0] + w1 * pSrc[1] + w2 * pSrc[2]);
						pSrc += 2;
					}
				}
			}
			else
			{
				for (uint32_t c = 0; c < componentCount_; c++)
				{
					const float* pSrc = channel(c);
					float* pDst = dst.channel(c);

					for (uint32_t x = 0; x < num; x++)
					{
						*pDst = 0.5f * (pSrc[0] + pSrc[1]);
						pDst++;
						pSrc += 2;
					}
				}
			}
		}
		// Regular box filter.
		else if ((width_ & 1) == 0 && (height_ & 1) == 0)
		{
			for (uint32_t c = 0; c < componentCount_; c++)
			{
				const float* pSrc = channel(c);
				float * pDst = dst.channel(c);

				for (uint32_t y = 0; y < newHeight; y++)
				{
					for (uint32_t x = 0; x < newWidth; x++)
					{
						*pDst = 0.25f * (pSrc[0] + pSrc[1] + pSrc[width_] + pSrc[width_ + 1]);
						pDst++;
						pSrc += 2;
					}

					pSrc += width_;
				}
			}
		}
		// Polyphase filters.
		else if (width_ & 1 && height_ & 1)
		{
			X_ASSERT(width_ == 2 * newWidth + 1, "")(width_, newWidth);
			X_ASSERT(height_ == 2 * newHeight + 1, "")(height_, newHeight);

			const float scale = 1.0f / (width_ * height_);

			for (uint32_t c = 0; c < componentCount_; c++)
			{
				const float* pSrc = channel(c);
				float* pDst = dst.channel(c);

				for (uint32_t y = 0; y < newHeight; y++)
				{
					const float v0 = float(newHeight - y);
					const float v1 = float(newHeight - 0);
					const float v2 = float(1 + y);

					for (uint32_t x = 0; x < newWidth; x++)
					{
						const float w0 = float(newWidth - x);
						const float w1 = float(newWidth - 0);
						const float w2 = float(1 + x);

						float f = 0.0f;
						f += v0 * (w0 * pSrc[0 * width_ + 2 * x] + w1 * pSrc[0 * width_ + 2 * x + 1] + w2 * pSrc[0 * width_ + 2 * x + 2]);
						f += v1 * (w0 * pSrc[1 * width_ + 2 * x] + w1 * pSrc[1 * width_ + 2 * x + 1] + w2 * pSrc[1 * width_ + 2 * x + 2]);
						f += v2 * (w0 * pSrc[2 * width_ + 2 * x] + w1 * pSrc[2 * width_ + 2 * x + 1] + w2 * pSrc[2 * width_ + 2 * x + 2]);

						*pDst = f * scale;
						pDst++;
					}

					pSrc += 2 * width_;
				}
			}
		}
		else if (width_ & 1)
		{
			X_ASSERT(width_ == 2 * newWidth + 1, "")(width_, newWidth);
			const float scale = 1.0f / (2 * width_);

			for (uint32_t c = 0; c < componentCount_; c++)
			{
				const float* pSrc = channel(c);
				float* pDst = dst.channel(c);

				for (uint32_t y = 0; y < newHeight; y++)
				{
					for (uint32_t x = 0; x < newWidth; x++)
					{
						const float w0 = float(newWidth - x);
						const float w1 = float(newWidth - 0);
						const float w2 = float(1 + x);

						float f = 0.0f;
						f += w0 * (pSrc[2 * x + 0] + pSrc[width_ + 2 * x + 0]);
						f += w1 * (pSrc[2 * x + 1] + pSrc[width_ + 2 * x + 1]);
						f += w2 * (pSrc[2 * x + 2] + pSrc[width_ + 2 * x + 2]);

						*pDst = f * scale;
						pDst++;
					}

					pSrc += 2 * width_;
				}
			}
		}
		else if (height_ & 1)
		{
			X_ASSERT(height_ == 2 * newHeight + 1, "")(height_, newHeight);

			const float scale = 1.0f / (2 * height_);

			for (uint32_t c = 0; c < componentCount_; c++)
			{
				const float* pSrc = channel(c);
				float* pDst = dst.channel(c);

				for (uint32_t y = 0; y < newHeight; y++)
				{
					const float v0 = float(newHeight - y);
					const float v1 = float(newHeight - 0);
					const float v2 = float(1 + y);

					for (uint32_t x = 0; x < newWidth; x++)
					{
						float f = 0.0f;
						f += v0 * (pSrc[0 * width_ + 2 * x] + pSrc[0 * width_ + 2 * x + 1]);
						f += v1 * (pSrc[1 * width_ + 2 * x] + pSrc[1 * width_ + 2 * x + 1]);
						f += v2 * (pSrc[2 * width_ + 2 * x] + pSrc[2 * width_ + 2 * x + 1]);

						*pDst = f * scale;
						pDst++;
					}

					pSrc += 2 * width_;
				}
			}
		}
		else
		{
			X_ASSERT_UNREACHABLE();
		}
	}

	void FloatImage::downSample(FloatImage& dst, core::MemoryArenaBase* arena, const Filter& filter, WrapMode::Enum wm) const
	{
		const uint32_t w = core::Max(1, width_ / 2);
		const uint32_t h = core::Max(1, height_ / 2);
		const uint32_t d = core::Max(1, depth_ / 2);

		return resize(dst, arena, filter, w, h, d, wm);
	}

	void FloatImage::downSample(FloatImage& dst, core::MemoryArenaBase* arena, const Filter& filter, WrapMode::Enum wm, uint32_t alphaChannel) const
	{
		const uint32_t w = core::Max(1, width_ / 2);
		const uint32_t h = core::Max(1, height_ / 2);
		const uint32_t d = core::Max(1, depth_ / 2);

		return resize(dst, arena, filter, w, h, d, wm, alphaChannel);
	}

	void FloatImage::resize(FloatImage& dst, core::MemoryArenaBase* arena, const Filter& filter, uint32_t w, uint32_t h, WrapMode::Enum wm) const
	{
		FloatImage tmpImage(arena);

		PolyphaseKernel xkernel(arena, filter, width_, w, 32);
		PolyphaseKernel ykernel(arena, filter, height_, h, 32);

		{
			tmpImage.allocate(componentCount_, w, height_);
			dst.allocate(componentCount_, w, h);

			// @@ We could avoid this allocation, write directly to dst_plane.
			core::Array<float> tmpColumn(arena, h);
			tmpColumn.resize(h);

			for (uint32_t c = 0; c < componentCount_; c++)
			{
				for (uint32_t z = 0; z < depth_; z++)
				{
					float* pTmpPlane = tmpImage.plane(c, z);

					for (uint32_t y = 0; y < height_; y++) {
						this->applyKernelX(xkernel, y, z, c, wm, pTmpPlane + y * w);
					}

					float* pDstPlane = dst.plane(c, z);

					for (uint32_t x = 0; x < w; x++) {
						tmpImage.applyKernelY(ykernel, x, z, c, wm, tmpColumn.data());

						// @@ We could avoid this copy, write directly to dst_plane.
						for (uint32_t y = 0; y < h; y++) {
							pDstPlane[y * w + x] = tmpColumn[y];
						}
					}
				}
			}
		}
	}

	void FloatImage::resize(FloatImage& dst, core::MemoryArenaBase* arena, const Filter& filter, uint32_t w, uint32_t h, uint32_t d, WrapMode::Enum wm) const
	{
		// @@ Use monophase filters when frac(m_width / w) == 0

		// Use the existing 2d version if we are not resizing in the Z axis:
		if (depth_ == d) {
			return resize(dst, arena, filter, w, h, wm);
		}

		FloatImage tmpImage(arena);
		FloatImage tmpImage2(arena);

		PolyphaseKernel xkernel(arena, filter, width_, w, 32);
		PolyphaseKernel ykernel(arena, filter, height_, h, 32);
		PolyphaseKernel zkernel(arena, filter, depth_, d, 32);

		tmpImage.allocate(componentCount_, w, height_, depth_);
		tmpImage.allocate(componentCount_, w, height_, d);
		dst.allocate(componentCount_, w, h, d);

		core::Array<float> tmpColumn(arena, h);
		tmpColumn.resize(h);

		for (uint32_t c = 0; c < componentCount_; c++)
		{
			float* pTmpChannel = tmpImage.channel(c);

			// split width in half
			for (uint32_t z = 0; z < depth_; z++) {
				for (uint32_t y = 0; y < height_; y++) {
					this->applyKernelX(xkernel, y, z, c, wm, pTmpChannel + z * height_ * w + y * w);
				}
			}

			// split depth in half
			float* pTmp2Channel = tmpImage2.channel(c);
			for (uint32_t y = 0; y < height_; y++) {
				for (uint32_t x = 0; x < w; x++) {
					tmpImage.applyKernelZ(zkernel, x, y, c, wm, tmpColumn.data());

					for (uint32_t z = 0; z < d; z++) {
						pTmp2Channel[z * height_ * w + y * w + x] = tmpColumn[z];
					}
				}
			}

			// split height in half
			float* pDstChannel = dst.channel(c);

			for (uint32_t z = 0; z < d; z++) {
				for (uint32_t x = 0; x < w; x++) {
					tmpImage2.applyKernelY(ykernel, x, z, c, wm, tmpColumn.data());

					for (uint32_t y = 0; y < h; y++) {
						pDstChannel[z * h * w + y * w + x] = tmpColumn[y];
					}
				}
			}
		}
	}

	void FloatImage::resize(FloatImage& dst, core::MemoryArenaBase* arena, const Filter& filter, uint32_t w, uint32_t h, WrapMode::Enum wm, uint32_t alphaChannel) const
	{
		X_ASSERT(alphaChannel < componentCount_, "Invalid alpha channel index")(alphaChannel, componentCount_);

		FloatImage tmpImage(arena);

		PolyphaseKernel xkernel(arena, filter, width_, w, 32);
		PolyphaseKernel ykernel(arena, filter, height_, h, 32);

		{
			tmpImage.allocate(componentCount_, w, height_);
			dst.allocate(componentCount_, w, h);

			core::Array<float> tmpColumn(arena, h);
			tmpColumn.resize(h);

			for (uint32_t i = 0; i < componentCount_; i++)
			{
				// Process alpha channel first.
				uint32_t c;
				if (i == 0) {
					c = alphaChannel;
				}
				else if (i > alphaChannel) {
					c = i;
				}
				else {
					c = i - 1;
				}

				for (uint32_t z = 0; z < depth_; z++)
				{
					float * tmp_plane = tmpImage.plane(c, z);

					for (uint32_t y = 0; y < height_; y++) {
						this->applyKernelX(xkernel, y, z, c, wm, tmp_plane + y * w);
					}

					float * dst_plane = tmpImage.plane(c, z);

					for (uint32_t x = 0; x < w; x++) {
						tmpImage.applyKernelY(ykernel, x, z, c, wm, tmpColumn.data());

						// @@ Avoid this copy, write directly to dst_plane.
						for (uint32_t y = 0; y < h; y++) {
							dst_plane[y * w + x] = tmpColumn[y];
						}
					}
				}
			}
		}
	}

	void FloatImage::resize(FloatImage& dst, core::MemoryArenaBase* arena, const Filter& filter, 
		uint32_t w, uint32_t h, uint32_t d, WrapMode::Enum wm, uint32_t alphaChannel) const
	{
		X_ASSERT(alphaChannel < componentCount_, "Invalid alpha channel index")(alphaChannel, componentCount_);

		// use the existing 2d version if we are a 2d image:
		if (depth_ == d) {
			return resize(dst, arena, filter, w, h, wm, alphaChannel);
		}

		FloatImage tmpImage(arena);
		FloatImage tmpImage2(arena);

		PolyphaseKernel xkernel(arena, filter, width_, w, 32);
		PolyphaseKernel ykernel(arena, filter, height_, h, 32);
		PolyphaseKernel zkernel(arena, filter, depth_, d, 32);

		tmpImage.allocate(componentCount_, w, height_, depth_);
		tmpImage2.allocate(componentCount_, w, height_, d);
		dst.allocate(componentCount_, w, h, d);

		core::Array<float> tmpColumn(arena, h);
		tmpColumn.resize(h);

		for (uint32_t i = 0; i < componentCount_; i++)
		{
			// Process alpha channel first.
			uint32_t c;
			if (i == 0) {
				c = alphaChannel;
			}
			else if (i > alphaChannel) {
				c = i;
			}
			else {
				c = i - 1;
			}

			float* pTmpChannel = tmpImage.channel(c);

			for (uint32_t z = 0; z < depth_; z++) {
				for (uint32_t y = 0; y < height_; y++) {
					this->applyKernelX(xkernel, y, z, c, wm, pTmpChannel + z * height_ * w + y * w);
				}
			}

			float* pTmp2Channel = tmpImage2.channel(c);
			for (uint32_t y = 0; y < height_; y++) {
				for (uint32_t x = 0; x < w; x++) {
					tmpImage.applyKernelZ(zkernel, x, y, c, wm, tmpColumn.data());

					for (uint32_t z = 0; z < d; z++) {
						pTmp2Channel[z * height_ * w + y * w + x] = tmpColumn[z];
					}
				}
			}

			float* pDstChannel = dst.channel(c);

			for (uint32_t z = 0; z < d; z++) {
				for (uint32_t x = 0; x < w; x++) {
					tmpImage2.applyKernelY(ykernel, x, z, c, wm, tmpColumn.data());

					for (uint32_t y = 0; y < h; y++) {
						pDstChannel[z * h * w + y * w + x] = tmpColumn[y];
					}
				}
			}
		}
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

	void FloatImage::swap(FloatImage& oth)
	{
		core::Swap(componentCount_, oth.componentCount_);
		core::Swap(width_, oth.width_);
		core::Swap(height_, oth.height_);
		core::Swap(depth_, oth.depth_);
		core::Swap(pixelCount_, oth.pixelCount_);
		data_.swap(oth.data_);
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

	size_t FloatImage::getAllocationSize(uint32_t channels, uint32_t width, uint32_t height, uint32_t depth)
	{
		return ((width * height) * depth) * channels;
	}

} // namespace Converter

X_NAMESPACE_END