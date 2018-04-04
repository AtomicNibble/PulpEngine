#pragma once


#ifndef _X_MATH_MATRIX22_H_
#define _X_MATH_MATRIX22_H_



#include "XMath.h"
#include "XVector.h"


//////////////////////////////////////////////////////////////////////////////////////////////////////
// Matrix22
template< typename T >
class Matrix22
{
public:
	typedef T	TYPE;
	typedef T	value_type;
	//
	static const size_t DIM		= 2;
	static const size_t DIM_SQ	= DIM*DIM;
	static const size_t MEM_LEN	= sizeof(T)*DIM_SQ;

	//
	// This class is OpenGL friendly and stores the m as how OpenGL would expect it.
	// m[i,j]:
	// | m[0,0] m[0,1] |
	// | m[1,0] m[1,1] |
	//
	// m[idx]
	// | m[0] m[2] |
	// | m[1] m[3] |
	//
	X_PUSH_WARNING_LEVEL(3)
	union {
		T m[4];
		struct {
			// This looks like it's transposed from the above, but it's really not.
			// It just has to be written this way so it follows the right ordering
			// in the memory layout as well as being mathematically correct.
			T m00, m10;
			T m01, m11;
		};
		// [Cols][Rows]
		T mcols[2][2];
	};
	X_POP_WARNING_LEVEL

	Matrix22();

	explicit Matrix22(T s);

	// OpenGL layout - unless srcIsRowMajor is true
	explicit Matrix22( const T *dt, bool srcIsRowMajor = false );

	// OpenGL layout: m[0]=d0, m[1]=d1, m[2]=d2, m[3]=d3 - unless srcIsRowMajor is true
	Matrix22( T d0, T d1, T d2, T d3, bool srcIsRowMajor = false );

	// Creates matrix with column vectors vx and vy
	Matrix22( const Vec2<T> &vx, const Vec2<T> &vy );

	template< typename FromT >
	explicit Matrix22( const Matrix22<FromT>& src );

	Matrix22( const Matrix22<T>& src );

	operator T*() { return (T*)m; }
	operator const T*() const { return (const T*)m; }

	Matrix22<T>&		operator=( const Matrix22<T>& rhs );
	Matrix22<T>&		operator=( const T &rhs );

	template< typename FromT >
	Matrix22<T>&		operator=( const Matrix22<FromT>& rhs );

	bool				equalCompare( const Matrix22<T>& rhs, T epsilon ) const;
	bool				operator==( const Matrix22<T> &rhs ) const { return equalCompare( rhs, (T)EPSILON ); }
	bool				operator!=( const Matrix22<T> &rhs ) const { return ! ( *this == rhs ); }

	Matrix22<T>&		operator*=( const Matrix22<T> &rhs );
	Matrix22<T>&		operator+=( const Matrix22<T> &rhs );
	Matrix22<T>&		operator-=( const Matrix22<T> &rhs );

	Matrix22<T>&		operator*=( T s );
	Matrix22<T>&		operator/=( T s );
	Matrix22<T>&		operator+=( T s );
	Matrix22<T>&		operator-=( T s );

	const Matrix22<T>	operator*( const Matrix22<T> &rhs ) const;
	const Matrix22<T>	operator+( const Matrix22<T> &rhs ) const;
	const Matrix22<T>	operator-( const Matrix22<T> &rhs ) const;

	// post-multiplies column vector [rhs.x rhs.y]
	const Vec2<T>		operator*( const Vec2<T> &rhs ) const;

	const Matrix22<T>	operator*( T rhs ) const;
	const Matrix22<T>	operator/( T rhs ) const;
	const Matrix22<T>	operator+( T rhs ) const;
	const Matrix22<T>	operator-( T rhs ) const;

	// Accessors
	T&					at( int row, int col );
	const T&			at( int row, int col ) const;

	// OpenGL layout - unless srcIsRowMajor is true
	void				set( const T *dt, bool srcIsRowMajor = false );
	// OpenGL layout: m[0]=d0, m[1]=d1, m[2]=d2, m[3]=d3 - unless srcIsRowMajor is true
	void				set( T d0, T d1, T d2, T d3, bool srcIsRowMajor = false );

	Vec2<T>				getColumn( int col ) const;
	void				setColumn( int col, const Vec2<T> &v );

	Vec2<T>				getRow( int row ) const;
	void				setRow( int row, const Vec2<T> &v );

	void				getColumns( Vec2<T> *c0, Vec2<T> *c1 ) const;
	void				setColumns( const Vec2<T> &c0, const Vec2<T> &c1 );

	void				getRows( Vec2<T> *r0, Vec2<T> *r1 ) const;
	void				setRows( const Vec2<T> &r0, const Vec2<T> &r1 );

	void				setToNull();
	void				setToIdentity();

	T					determinant() const;
	T					trace() const;

	Matrix22<T>         diagonal() const;

	Matrix22<T>			lowerTriangular() const;
	Matrix22<T>			upperTriangular() const;

	void				transpose();
	Matrix22<T>			transposed() const;

	void				invert (T epsilon = EPSILON ) { *this = inverted( epsilon ); }
	Matrix22<T>			inverted( T epsilon = EPSILON ) const;

	// pre-multiplies row vector v - no divide by w
	Vec2<T>				preMultiply( const Vec2<T> &v ) const;

	// post-multiplies column vector v - no divide by w
	Vec2<T>				postMultiply( const Vec2<T> &v ) const;

	// post-multiplies column vector [rhs.x rhs.y]
	Vec2<T>				transformVec( const Vec2<T> &v ) const { return postMultiply( v ); }

	// rotate by radians (conceptually, rotate is before 'this')
	void				rotate( T radians ) { Matrix22 rot = createRotation( radians ); Matrix22 mat = *this; *this = rot*mat; }

	// concatenate scale (conceptually, scale is before 'this')
	void				scale( T s ) { Matrix22 sc = createScale( s ); Matrix22 mat = *this; *this = sc*mat; }
	void				scale( const Vec2<T> &v ) { Matrix22 sc = createScale( v ); Matrix22 mat = *this; *this = sc*mat; }

	// transposes rotation sub-matrix and inverts translation
	Matrix22<T>			invertTransform() const;

	// returns an identity matrix
	static Matrix22<T>	identity() { return Matrix22( 1, 0, 0, 1 ); }
	// returns 1 filled matrix
	static Matrix22<T>  one() { return Matrix22( (T)1 ); }
	// returns 0 filled matrix
	static Matrix22<T>  zero() { return Matrix22( (T)0 ); }

	// creates rotation matrix
	static Matrix22<T>	createRotation( T radians );

	// creates scale matrix
	static Matrix22<T>	createScale( T s );
	static Matrix22<T>	createScale( const Vec2<T> &v );

};

typedef Matrix22<float32_t>	 Matrix22f;
typedef Matrix22<float64_t> Matrix22d;


#include "XMatrix22.inl"

#endif // _X_MATH_MATRIX22_H_