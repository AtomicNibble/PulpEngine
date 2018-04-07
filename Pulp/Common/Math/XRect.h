#pragma once

#ifndef _X_MATH_RECT_H_
#define _X_MATH_RECT_H_

#include "XVector.h"
#include "XAlignment.h"

template<typename T>
class RectT
{
public:
    T x1, y1, x2, y2;

    RectT();
    RectT(T aX1, T aY1, T aX2, T aY2);
    RectT(const Vec2<T>& v1, const Vec2<T>& v2);

    void set(T aX1, T aY1, T aX2, T aY2);

    T getWidth(void) const;
    T getHeight(void) const;
    T getAspectRatio(void) const;
    T calcArea(void) const;

    void canonicalize(void);         // return rect w/ properly ordered coordinates
    RectT canonicalized(void) const; // return rect w/ properly ordered coordinates

    void clipBy(const RectT& clip);
    RectT getClipBy(const RectT& clip) const;
    
    void offset(const Vec2<T>& offset);
    RectT getOffset(const Vec2<T>& off) const;

    void inflate(const Vec2<T>& amount);
    RectT inflated(const Vec2<T>& amount) const;

    //! Translates the rectangle so that its center is at \a center
    void offsetCenterTo(const Vec2<T>& center);
    
    void scaleCentered(const Vec2<T>& scale);
    void scaleCentered(T scale);
    RectT scaledCentered(T scale) const;
    
    void scale(T scale);
    void scale(const Vec2<T>& scale);
    
    RectT scaled(T scale) const;
    RectT scaled(const Vec2<T>& scale) const;

    /** \brief Is a point \a pt inside the rectangle **/
    template<typename Y>
    bool contains(const Vec2<Y>& pt) const;

    /** \brief Is a point \a pt inside the rectangle **/
    bool contains(const RectT& rect) const;

    //! Returns whether \a rect intersects with this
    bool intersects(const RectT& rect) const;

    //! Returns the distance between the point \a pt and the rectangle. Points inside the rectangle return \c 0.
    T distance(const Vec2<T>& pt) const;
    //! Returns the squared distance between the point \a pt and the rectangle. Points inside the rectangle return \c 0.
    T distanceSquared(const Vec2<T>& pt) const;

    //! Returns the nearest point on the Rect \a rect. Points inside the rectangle return \a pt.
    Vec2<T> closestPoint(const Vec2<T>& pt) const;

    T getX1(void) const;
    T getY1(void) const;
    T getX2(void) const;
    T getY2(void) const;

    Vec2<T> getUpperLeft(void) const;
    Vec2<T> getUpperRight(void) const;
    Vec2<T> getLowerRight(void) const;
    Vec2<T> getLowerLeft(void) const;
    Vec2<T> getCenter(void) const;
    Vec2<T> getSize(void) const;

    /** \return Scaled copy with the same aspect ratio centered relative to and scaled to fit inside \a other. If \a expand then the rectangle is expanded if it is smaller than \a other */
    RectT getCenteredFit(const RectT& other, bool expand) const;

    /** Expands the Rect to include \a point in its interior **/
    void include(const Vec2<T>& point);
    /** Expands the Rect to include \a rect in its interior **/
    void include(const RectT& rect);

    // align
    RectT& Align(const RectT& other, AlignmentFlags alignment);

    const RectT<T> operator+(const Vec2<T>& o) const;
    const RectT<T> operator-(const Vec2<T>& o) const;
    const RectT<T> operator*(T s) const;
    const RectT<T> operator/(T s) const;

    const RectT<T> operator+(const RectT<T>& rhs) const;
    const RectT<T> operator-(const RectT<T>& rhs) const;

    RectT<T>& operator+=(const Vec2<T>& o);
    RectT<T>& operator-=(const Vec2<T>& o);
    RectT<T>& operator*=(T s);
    RectT<T>& operator/=(T s);
};

typedef RectT<int32_t> Recti;
typedef RectT<float32_t> Rectf;
typedef RectT<float64_t> Rectd;

#include "XRect.inl"

#endif // !_X_MATH_RECT_H_
