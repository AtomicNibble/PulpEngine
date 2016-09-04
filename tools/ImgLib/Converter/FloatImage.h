#pragma once

#include <Containers\Array.h>
#include "Types.h"

X_NAMESPACE_BEGIN(texture)

namespace Converter
{
	class Filter;

	class FloatImage
	{
			
	public:
		FloatImage(core::MemoryArenaBase* arena);
		~FloatImage();


		void fastDownSample(void) const;
		void downSample(const Filter& filter, WrapMode::Enum wm) const;
		void downSample(const Filter& filter, WrapMode::Enum wm, uint32_t alphaChannel) const;
		void resize(const Filter& filter, uint32_t w, uint32_t h, WrapMode::Enum wm) const;
		void resize(const Filter& filter, uint32_t w, uint32_t h, uint32_t d, WrapMode::Enum wm) const;
		void resize(const Filter& filter, uint32_t w, uint32_t h, WrapMode::Enum wm, uint32_t alphaChannel) const;
		void resize(const Filter& filter, uint32_t w, uint32_t h, uint32_t d, WrapMode::Enum wm, uint32_t alphaChannel) const;

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

	private:
		uint16_t componentCount_; // R, G, B, A ..
		uint16_t width_;
		uint16_t height_;
		uint16_t depth_;
		uint32_t pixelCount_;
		core::Array<float> data_;
	};


} // namespace Converters

X_NAMESPACE_END

#include "FloatImage.inl"