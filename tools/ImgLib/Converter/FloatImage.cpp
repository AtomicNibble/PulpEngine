#include "stdafx.h"
#include "FloatImage.h"


X_NAMESPACE_BEGIN(texture)

namespace Converter
{

	FloatImage::FloatImage(core::MemoryArenaBase* arena) :
		data_(arena)
	{

	}

	FloatImage::~FloatImage()
	{

	}


	void FloatImage::fastDownSample(void) const
	{

	}

	void FloatImage::downSample(const Filter& filter, WrapMode::Enum wm) const
	{

	}

	void FloatImage::downSample(const Filter& filter, WrapMode::Enum wm, uint32_t alphaChannel) const
	{

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


} // namespace Converter

X_NAMESPACE_END