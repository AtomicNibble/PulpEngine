#pragma once

#ifndef _X_MATH_RECT_H_
#define _X_MATH_RECT_H_

#include "XVector.h"
#include "XAlignment.h"

template<typename T>
class RectT
{
public:
	T	x1, y1, x2, y2;

	RectT() : x1(0), y1(0), x2(0), y2(0) {}

	RectT(T aX1, T aY1, T aX2, T aY2) {
		set(aX1, aY1, aX2, aY2);
	}
	RectT(const Vec2<T> &v1, const Vec2<T> &v2) {
		set(v1.x, v1.y, v2.x, v2.y);
	}

	void		set(T aX1, T aY1, T aX2, T aY2);

	T			getWidth() const { return x2 - x1; }
	T			getHeight() const { return y2 - y1; }
	T			getAspectRatio() const { return getWidth() / getHeight(); }
	T			calcArea() const { return getWidth() * getHeight(); }

	void		canonicalize(); // return rect w/ properly ordered coordinates
	RectT		canonicalized() const; // return rect w/ properly ordered coordinates

	void		clipBy(const RectT &clip);
	RectT		getClipBy(const RectT &clip) const;
	//	Area		getInteriorArea() const;
	void		offset(const Vec2<T> &offset);
	RectT		getOffset(const Vec2<T> &off) const { RectT result(*this); result.offset(off); return result; }
	void		inflate(const Vec2<T> &amount);
	RectT		inflated(const Vec2<T> &amount) const;
	//! Translates the rectangle so that its center is at \a center
	void		offsetCenterTo(const Vec2<T> &center) { offset(center - getCenter()); }
	void		scaleCentered(const Vec2<T> &scale);
	void		scaleCentered(T scale);
	RectT		scaledCentered(T scale) const;
	void		scale(T scale);
	void		scale(const Vec2<T> &scale);
	RectT		scaled(T scale) const;
	RectT		scaled(const Vec2<T> &scale) const;


	/** \brief Is a point \a pt inside the rectangle **/
	template<typename Y>
	bool		contains(const Vec2<Y> &pt) const { return (pt.x >= x1) && (pt.x <= x2) && (pt.y >= y1) && (pt.y <= y2); }
	/** \brief Is a point \a pt inside the rectangle **/
	bool		contains(const RectT &rect) const;

	//! Returns whether \a rect intersects with this
	bool		intersects(const RectT &rect) const;

	//! Returns the distance between the point \a pt and the rectangle. Points inside the rectangle return \c 0.
	T			distance(const Vec2<T> &pt) const;
	//! Returns the squared distance between the point \a pt and the rectangle. Points inside the rectangle return \c 0.
	T			distanceSquared(const Vec2<T> &pt) const;

	//! Returns the nearest point on the Rect \a rect. Points inside the rectangle return \a pt.
	Vec2<T>		closestPoint(const Vec2<T> &pt) const;

	T			getX1() const { return x1; }
	T			getY1() const { return y1; }
	T			getX2() const { return x2; }
	T			getY2() const { return y2; }

	Vec2<T>		getUpperLeft() const { return Vec2<T>(x1, y1); };
	Vec2<T>		getUpperRight() const { return Vec2<T>(x2, y1); };
	Vec2<T>		getLowerRight() const { return Vec2<T>(x2, y2); };
	Vec2<T>		getLowerLeft() const { return Vec2<T>(x1, y2); };
	Vec2<T>		getCenter() const { return Vec2<T>((x1 + x2) / 2, (y1 + y2) / 2); }
	Vec2<T>		getSize() const { return Vec2<T>(x2 - x1, y2 - y1); }

	/** \return Scaled copy with the same aspect ratio centered relative to and scaled to fit inside \a other. If \a expand then the rectangle is expanded if it is smaller than \a other */
	RectT		getCenteredFit(const RectT &other, bool expand) const;

	/** Expands the Rect to include \a point in its interior **/
	void		include(const Vec2<T> &point);
	/** Expands the Rect to include \a rect in its interior **/
	void		include(const RectT &rect);

	// align
	RectT&		Align(const RectT& other, AlignmentFlags alignment);


	const RectT<T>		operator+(const Vec2<T> &o) const { return this->getOffset(o); }
	const RectT<T>		operator-(const Vec2<T> &o) const { return this->getOffset(-o); }
	const RectT<T>		operator*(T s) const { return this->scaled(s); }
	const RectT<T>		operator/(T s) const { return this->scaled(((T)1) / s); }

	const RectT<T>		operator+(const RectT<T>& rhs) const { return RectT<T>(x1 + rhs.x1, y1 + rhs.y1, x2 + rhs.x2, y2 + rhs.y2); }
	const RectT<T>		operator-(const RectT<T>& rhs) const { return RectT<T>(x1 - rhs.x1, y1 - rhs.y1, x2 - rhs.x2, y2 - rhs.y2); }

	RectT<T>&		operator+=(const Vec2<T> &o) { offset(o); return *this; }
	RectT<T>&		operator-=(const Vec2<T> &o) { offset(-o); return *this; }
	RectT<T>&		operator*=(T s) { scale(s); return *this; }
	RectT<T>&		operator/=(T s) { scale(((T)1) / s); return *this; }

};


typedef RectT<int32_t>		Recti;
typedef RectT<float32_t>	Rectf;
typedef RectT<float64_t>	Rectd;


#include "XRect.inl"


#endif // !_X_MATH_RECT_H_
