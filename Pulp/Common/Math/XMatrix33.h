#pragma once


#ifndef _X_MATH_MATRIX33_H_
#define _X_MATH_MATRIX33_H_


#include "XMath.h"
#include "XVector.h"

#include "XMatrix34.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Matrix33
template< typename T >
class Matrix33 
{
public:
	typedef T	TYPE;
	typedef T	value_type;
	//
	static const size_t DIM		= 3;
	static const size_t DIM_SQ	= DIM*DIM;
	static const size_t MEM_LEN	= sizeof(T)*DIM_SQ;

	//
	// This class is OpenGL friendly and stores the m as how OpenGL would expect it.
	// m[i,j]:
	// | m[0,0] m[0,1] m[0,2] |
	// | m[1,0] m[1,1] m[1,2] |
	// | m[2,0] m[2,1] m[2,2] |
	//
	// m[idx]
	// | m[0] m[3] m[6] |
	// | m[1] m[4] m[7] |
	// | m[2] m[5] m[8] |
	//
	X_PUSH_WARNING_LEVEL(3)

	union {
		T m[9];
		struct {
			// This looks like it's transposed from the above, but it's really not.
			// It just has to be written this way so it follows the right ordering
			// in the memory layout as well as being mathematically correct.
			T m00, m10, m20;
			T m01, m11, m21;
			T m02, m12, m22;
		};
		// [Cols][Rows]
		T mcols[3][3];
	};
	X_POP_WARNING_LEVEL

	Matrix33();

	explicit Matrix33( T s );

	// OpenGL layout - unless srcIsRowMajor is true
	explicit Matrix33( const T *dt, bool srcIsRowMajor = false );

	// OpenGL layout: m[0]=d0, m[1]=d1, m[2]=d2 ... m[6]=d6, m[7]=d7, m[8]=d8 - unless srcIsRowMajor is true
	Matrix33( T d0, T d1, T d2, T d3, T d4, T d5, T d6, T d7, T d8, bool srcIsRowMajor = false );

	// Creates matrix with column vectors vx, vy and z
	Matrix33(const Vec3<T> &vx, const Vec3<T> &vy, const Vec3<T> &vz);

	template< typename FromT >
	explicit Matrix33(const Matrix33<FromT>& src);

	explicit Matrix33(const Matrix22<T>& src);
	Matrix33(const Matrix33<T>& src);
	explicit Matrix33(const Matrix34<T>& src);

	operator T*() { return (T*)m; }
	operator const T*() const { return (const T*)m; }

	Matrix33<T>&		operator=( const Matrix33<T>& rhs );
	Matrix33<T>&		operator=( T rhs );

	template< typename FromT >
	Matrix33<T>&		operator=( const Matrix33<FromT>& rhs );

	// remaining columns and rows will be filled with identity values
	Matrix33<T>&		operator=( const Matrix22<T>& rhs );

	bool				equalCompare( const Matrix33<T>& rhs, T epsilon ) const;
	bool				operator==( const Matrix33<T> &rhs ) const { return equalCompare( rhs, (T)EPSILON ); }
	bool				operator!=( const Matrix33<T> &rhs ) const { return ! ( *this == rhs ); }

	Matrix33<T>&		operator*=( const Matrix33<T> &rhs );
	Matrix33<T>&		operator+=( const Matrix33<T> &rhs );
	Matrix33<T>&		operator-=( const Matrix33<T> &rhs );

	Matrix33<T>&		operator*=( T s );
	Matrix33<T>&		operator/=( T s );
	Matrix33<T>&		operator+=( T s );
	Matrix33<T>&		operator-=( T s );

	const Matrix33<T>	operator*( const Matrix33<T> &rhs ) const;
	const Matrix33<T>	operator+( const Matrix33<T> &rhs ) const;
	const Matrix33<T>	operator-( const Matrix33<T> &rhs ) const;

	// post-multiplies column vector [rhs.x rhs.y rhs.z]
	const Vec3<T>		operator*( const Vec3<T> &rhs ) const;

	const Matrix33<T>	operator*( T rhs ) const;
	const Matrix33<T>	operator/( T rhs ) const;
	const Matrix33<T>	operator+( T rhs ) const;
	const Matrix33<T>	operator-( T rhs ) const;

	// Accessors
	T&					at( int row, int col );
	const T&			at( int row, int col ) const;

	// OpenGL layout - unless srcIsRowMajor is true
	void				set( const T *dt, bool srcIsRowMajor = false );
	// OpenGL layout: m[0]=d0, m[1]=d1, m[2]=d2 ... m[6]=d6, m[7]=d7, m[8]=d8 - unless srcIsRowMajor is true
	void				set( T d0, T d1, T d2, T d3, T d4, T d5, T d6, T d7, T d8, bool srcIsRowMajor = false );

	Vec3<T>				getColumn( int col ) const;
	void				setColumn( int col, const Vec3<T> &v );

	Vec3<T>				getRow( int row ) const;
	void				setRow( int row, const Vec3<T> &v );

	void				getColumns( Vec3<T> *c0, Vec3<T> *c1, Vec3<T> *c2 ) const;
	void				setColumns( const Vec3<T> &c0, const Vec3<T> &c1, const Vec3<T> &c2 );

	void				getRows( Vec3<T> *r0, Vec3<T> *r1, Vec3<T> *r2 ) const;
	void				setRows( const Vec3<T> &r0, const Vec3<T> &r1, const Vec3<T> &r2 );

	// returns a sub-matrix starting at row, col
	Matrix22<T>			subMatrix22( int row, int col ) const;

	void				setToNull();
	void				setToIdentity();

	T					determinant() const;
	T					trace() const;

	Matrix33<T>         diagonal() const;

	Matrix33<T>			lowerTriangular() const;
	Matrix33<T>			upperTriangular() const;

	void				transpose();
	Matrix33<T>			transposed() const;

	void				invert (T epsilon = EPSILON ) { *this = inverted( epsilon ); }
	Matrix33<T>			inverse(T epsilon = EPSILON) const { return inverted(epsilon);  }
	Matrix33<T>			inverted( T epsilon = EPSILON ) const;

	// pre-multiplies row vector v - no divide by w
	Vec3<T>				preMultiply( const Vec3<T> &v ) const;

	// post-multiplies column vector v - no divide by w
	Vec3<T>				postMultiply( const Vec3<T> &v ) const;

	// post-multiplies column vector [rhs.x rhs.y rhs.z]
	Vec3<T>				transformVec( const Vec3<T> &v ) const { return postMultiply( v ); }

	// rotate by radians on axis (conceptually, rotate is before 'this')
	template <template <typename> class VecT>
	void				rotate( const VecT<T> &axis, T radians ) { *this *= Matrix33<T>::createRotation( axis, radians ); }
	// rotate by eulerRadians - Euler angles in radians (conceptually, rotate is before 'this')
	template <template <typename> class VecT>
	void				rotate( const VecT<T> &eulerRadians ) { *this *= Matrix33<T>::createRotation( eulerRadians ); }
	// rotate by matrix derives rotation matrix using from, to, worldUp	(conceptually, rotate is before 'this')
	template <template <typename> class VecT> 
	void				rotate( const VecT<T> &from, const VecT<T> &to, const VecT<T> &worldUp ) { *this *= Matrix33<T>::createRotation( from, to, worldUp ); }

	// transposes rotation sub-matrix and inverts translation
	Matrix33<T>			invertTransform() const;

	// returns an identity matrix
	static Matrix33<T>	identity() { return Matrix33( 1, 0, 0, 0, 1, 0, 0, 0, 1 ); }
	// returns 1 filled matrix
	static Matrix33<T>  one() { return Matrix33( (T)1 ); }
	// returns 0 filled matrix
	static Matrix33<T>  zero() { return Matrix33( (T)0 ); }

	// creates rotation matrix
	static Matrix33<T>	createRotation( const Vec3<T> &axis, T radians );
	static Matrix33<T>	createRotation( const Vec3<T> &from, const Vec3<T> &to, const Vec3<T> &worldUp );
	// equivalent to rotate( zAxis, z ), then rotate( yAxis, y ) then rotate( xAxis, x )
	static Matrix33<T>	createRotation(const Vec3<T> &eulerRadians);
	// diffrnet name as i might use it instead of the one above.
	// if i'm drunk :Z
	static Matrix33<T>	createRotationV01(const Vec3<T> &v0, const Vec3<T> &v1);


	// creates scale matrix
	static Matrix33<T>	createScale( T s );
	static Matrix33<T>	createScale( const Vec2<T> &v );
	static Matrix33<T>	createScale( const Vec3<T> &v );

	// creates a rotation matrix with z-axis aligned to targetDir	
	static Matrix33<T>	alignZAxisWithTarget( Vec3<T> targetDir, Vec3<T> upDir );

};


typedef Matrix33<float32_t>	 Matrix33f;
typedef Matrix33<float64_t>  Matrix33d;


#include "XMatrix33.inl"


#endif // _X_MATH_MATRIX33_H_