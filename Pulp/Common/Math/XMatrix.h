#pragma once

#ifndef _X_MATH_MATRIX_H_
#define _X_MATH_MATRIX_H_

template<class T>
class Matrix22;

template<class T>
class Matrix33;

template<class T>
class Matrix34;

template<class T>
class Matrix44;

#define X_ALIGN16_MATRIX44F(VAR) X_ALIGNED_SYMBOL(Matrix44f, 16) \
VAR

#include "XMath.h"
#include "XMatrix22.h"
#include "XMatrixAffine2.h"
#include "XMatrix33.h"
#include "XMatrix44.h"
#include "XMatrix34.h" // must be last since it uses 33 & 44

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Parallel Transport Frames
//
//  These methods compute a set of reference frames, defined by their
//  transformation matrix, along a curve. It is designed so that the
//  array of points and the array of matrices used to fetch these routines
//  don't need to be ordered as the curve.
//
//  A typical usage would be :
//
//      m[0] = Imath::firstFrame( p[0], p[1], p[2] );
//      for( int i = 1; i < n - 1; i++ )
//      {
//          m[i] = Imath::nextFrame( m[i-1], p[i-1], p[i], t[i-1], t[i] );
//      }
//      m[n-1] = Imath::lastFrame( m[n-2], p[n-2], p[n-1] );
//
//  See Graphics Gems I for the underlying algorithm.
//  These are also called Parallel Transport Frames
//    see Game Programming Gems 2, Section 2.5

// Vec3
template<typename T>
Matrix44<T> firstFrame(
    const Vec3<T>& firstPoint,
    const Vec3<T>& secondPoint,
    const Vec3<T>& thirdPoint);

template<typename T>
Matrix44<T> nextFrame(
    const Matrix44<T>& prevMatrix,
    const Vec3<T>& prevPoint,
    const Vec3<T>& curPoint,
    Vec3<T>& prevTangent,
    Vec3<T>& curTangent);

template<typename T>
Matrix44<T> lastFrame(
    const Matrix44<T>& prevMatrix,
    const Vec3<T>& prevPoint,
    const Vec3<T>& lastPoint);

// Vec4
template<typename T>
Matrix44<T> firstFrame(
    const Vec4<T>& firstPoint,
    const Vec4<T>& secondPoint,
    const Vec4<T>& thirdPoint)
{
    return firstFrame(firstPoint.xyz(), secondPoint.xyz(), thirdPoint.xyz());
}

template<typename T>
Matrix44<T> nextFrame(
    const Matrix44<T>& prevMatrix,
    const Vec4<T>& prevPoint,
    const Vec4<T>& curPoint,
    Vec4<T>& prevTangent,
    Vec4<T>& curTangent)
{
    Vec3<T> aPrevTangent = prevTangent.xyz();
    Vec3<T> aCurTangent = curTangent.xyz();
    return nextFrame(prevMatrix, prevPoint.xyz(), curPoint.xyz(), aPrevTangent, aCurTangent);
}

template<typename T>
Matrix44<T> lastFrame(
    const Matrix44<T>& prevMatrix,
    const Vec4<T>& prevPoint,
    const Vec4<T>& lastPoint)
{
    return lastFrame(prevMatrix, prevPoint.xyz(), lastPoint.xyz());
}

#endif // !_X_MATH_MATRIX_H_