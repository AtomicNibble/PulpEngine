#pragma once

#include <Containers\Array.h>
#include "Types.h"

X_NAMESPACE_BEGIN(texture)

class XTextureFile;

namespace Converter
{
	class Filter;
	class Kernel1;
	class Kernel2;
	class PolyphaseKernel;

	class FloatImage
	{
	public:
		typedef core::Array<float> ImgData;
			
	public:
		FloatImage(core::MemoryArenaBase* arena);
		~FloatImage();

		void initFrom(const XTextureFile& img);

		void allocate(uint32_t channels, uint32_t width, uint32_t heigth, uint32_t depth = 1);
		void free(void);

		void fastDownSample(FloatImage& dst) const;
		void downSample(FloatImage& dst, core::MemoryArenaBase* arena, const Filter& filter, WrapMode::Enum wm) const;
		void downSample(FloatImage& dst, core::MemoryArenaBase* arena, const Filter& filter, WrapMode::Enum wm, uint32_t alphaChannel) const;
		void resize(FloatImage& dst, core::MemoryArenaBase* arena, const Filter& filter, uint32_t w, uint32_t h, WrapMode::Enum wm) const;
		void resize(FloatImage& dst, core::MemoryArenaBase* arena, const Filter& filter, uint32_t w, uint32_t h, uint32_t d, WrapMode::Enum wm) const;
		void resize(FloatImage& dst, core::MemoryArenaBase* arena, const Filter& filter, uint32_t w, uint32_t h, WrapMode::Enum wm, uint32_t alphaChannel) const;
		void resize(FloatImage& dst, core::MemoryArenaBase* arena, const Filter& filter, uint32_t w, uint32_t h, uint32_t d, WrapMode::Enum wm, uint32_t alphaChannel) const;

		float applyKernelXY(const Kernel2* k, int32_t x, int32_t y, int32_t z, uint32_t c, WrapMode::Enum wm) const;
		float applyKernelX(const Kernel1* k, int32_t x, int32_t y, int32_t z, uint32_t c, WrapMode::Enum wm) const;
		float applyKernelY(const Kernel1* k, int32_t x, int32_t y, int32_t z, uint32_t c, WrapMode::Enum wm) const;
		float applyKernelZ(const Kernel1* k, int32_t x, int32_t y, int32_t z, uint32_t c, WrapMode::Enum wm) const;
		void applyKernelX(const PolyphaseKernel& k, int32_t y, int32_t z, uint32_t c, WrapMode::Enum wm, float* pOutput) const;
		void applyKernelY(const PolyphaseKernel& k, int32_t x, int32_t z, uint32_t c, WrapMode::Enum wm, float* pOutput) const;
		void applyKernelZ(const PolyphaseKernel& k, int32_t x, int32_t y, uint32_t c, WrapMode::Enum wm, float* pOutput) const;
		void applyKernelX(const PolyphaseKernel& k, int32_t y, int32_t z, uint32_t c, uint32_t a, WrapMode::Enum wm, float* pOutput) const;
		void applyKernelY(const PolyphaseKernel& k, int32_t x, int32_t z, uint32_t c, uint32_t a, WrapMode::Enum wm, float* pOutput) const;
		void applyKernelZ(const PolyphaseKernel& k, int32_t x, int32_t y, uint32_t c, uint32_t a, WrapMode::Enum wm, float* pOutput) const;

		void flipX(void);
		void flipY(void);
		void flipZ(void);

		X_INLINE uint32_t width(void) const;
		X_INLINE uint32_t height(void) const;
		X_INLINE uint32_t depth(void) const;
		X_INLINE uint32_t componentCount(void) const;
		X_INLINE uint32_t pixelCount(void) const;

	private:
		X_INLINE const float* channel(uint32_t component) const;
		X_INLINE float* channel(uint32_t component);

		X_INLINE const float* plane(uint32_t component, uint32_t z) const;
		X_INLINE float* plane(uint32_t component, uint32_t z);

		X_INLINE const float* scanline(uint32_t component, uint32_t y, uint32_t z) const;
		X_INLINE float* scanline(uint32_t component, uint32_t y, uint32_t z);

		X_INLINE uint32_t index(uint32_t x, uint32_t y, uint32_t z) const;
		uint32_t index(int32_t x, int32_t y, int32_t z, WrapMode::Enum wm) const;
		uint32_t indexClamp(int32_t x, int32_t y, int32_t z) const;
		uint32_t indexRepeat(int32_t x, int32_t y, int32_t z) const;
		uint32_t indexMirror(int32_t x, int32_t y, int32_t z) const;

		static size_t getAllocationSize(uint32_t channels, uint32_t width, uint32_t height, uint32_t depth);

	private:
		uint16_t componentCount_; // R, G, B, A ..
		uint16_t width_;
		uint16_t height_;
		uint16_t depth_;
		uint32_t pixelCount_;
		ImgData data_;
	};


} // namespace Converters

X_NAMESPACE_END

#include "FloatImage.inl"