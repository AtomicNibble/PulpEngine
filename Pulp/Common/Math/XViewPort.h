#pragma once

#ifndef _X_MATH_VIEWPORT_H_
#define _X_MATH_VIEWPORT_H_

#include <Math\XRect.h>

#ifdef near
#undef near
#endif

#ifdef far
#undef far
#endif

class XViewPort
{
public:
	XViewPort() = default;

	X_INLINE void setZ(float32_t near, float32_t far);
	X_INLINE void set(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom);
	X_INLINE void set(uint32_t width, uint32_t height);
	X_INLINE void set(const Vec2<uint32_t>& wh);
	X_INLINE uint32_t getX(void) const;
	X_INLINE uint32_t getY(void) const;
	X_INLINE uint32_t getWidth(void) const;
	X_INLINE uint32_t getHeight(void) const;
	X_INLINE float getXf(void) const;
	X_INLINE float getYf(void) const;
	X_INLINE float getWidthf(void) const;
	X_INLINE float getHeightf(void) const;
	X_INLINE Recti getRect(void);
	X_INLINE const Recti& getRect(void) const;
	X_INLINE float getZNear(void) const;
	X_INLINE float getZFar(void) const;

private:
	Recti view_;
	Rectf viewf_;
	Vec2f z_;
};


#include "XViewPort.inl"


#endif // !_X_MATH_VIEWPORT_H_