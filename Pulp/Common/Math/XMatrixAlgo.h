#pragma once

#ifndef _X_MATH_MATRIX_ALGO_H_
#define _X_MATH_MATRIX_ALGO_H_


#include "XMatrix22.h"
#include "XMatrix33.h"
#include "XMatrix44.h"


#if defined( X_PLATFORM_WIN32 )

#if defined( X_PLATFORM_WIN32 )
	#define X_ALIGN16_MATRIX44F( VAR ) \
		X_ALIGNED_SYMBOL(Matrix44f,16) VAR
#endif

inline Matrix44f SseMultiply( const Matrix44f& mat0, const Matrix44f& mat1 ) 
{
	X_ALIGN16_MATRIX44F( ret );

	X_ASSERT_ALIGNMENT(&mat0, 16, 0);
	X_ASSERT_ALIGNMENT(&mat1, 16, 0);

	const float* a = mat0.m;
	const float* b = mat1.m;
	float* res = ret.m;

	__m128 b_line, r_line, ab;
	__m128 a_line_0, a_line_4, a_line_8, a_line_12;

	// NOTE: Keep the usage of i in. Removed it and
	//       used constants and it ran slower. I'm not
	//       sure why - so I'm just going with it at 
	//       the moment.
	int i;

	// Load in the rows for b
	a_line_0  = _mm_load_ps( a +  0 );
	a_line_4  = _mm_load_ps( a +  4 );
	a_line_8  = _mm_load_ps( a +  8 );
	a_line_12 = _mm_load_ps( a + 12 );

	i = 0;
	// 
	b_line = _mm_set1_ps( *(b + i + 0) );
	r_line = _mm_mul_ps( b_line, a_line_0 );
	//
	b_line = _mm_set1_ps( *(b + i + 1) );
	ab     = _mm_mul_ps( b_line, a_line_4 );
	r_line = _mm_add_ps( ab, r_line );
	//
	b_line = _mm_set1_ps( *(b + i + 2) );
	ab     = _mm_mul_ps( b_line, a_line_8 );
	r_line = _mm_add_ps( ab, r_line );
	//
	b_line = _mm_set1_ps( *(b + i + 3) );
	ab     = _mm_mul_ps( b_line, a_line_12 );
	r_line = _mm_add_ps( ab, r_line );
	//
	_mm_store_ps( res + i, r_line );

	i = 4;
	// 
	b_line = _mm_set1_ps( *(b + i + 0) );
	r_line = _mm_mul_ps( b_line, a_line_0 );
	//
	b_line = _mm_set1_ps( *(b + i + 1) );
	ab     = _mm_mul_ps( b_line, a_line_4 );
	r_line = _mm_add_ps( ab, r_line );
	//
	b_line = _mm_set1_ps( *(b + i + 2) );
	ab     = _mm_mul_ps( b_line, a_line_8 );
	r_line = _mm_add_ps( ab, r_line );
	//
	b_line = _mm_set1_ps( *(b + i + 3) );
	ab     = _mm_mul_ps( b_line, a_line_12 );
	r_line = _mm_add_ps( ab, r_line );
	//
	_mm_store_ps( res + i, r_line );

	i = 8;
	// 
	b_line = _mm_set1_ps( *(b + i + 0) );
	r_line = _mm_mul_ps( b_line, a_line_0 );
	//
	b_line = _mm_set1_ps( *(b + i + 1) );
	ab     = _mm_mul_ps( b_line, a_line_4 );
	r_line = _mm_add_ps( ab, r_line );
	//
	b_line = _mm_set1_ps( *(b + i + 2) );
	ab     = _mm_mul_ps( b_line, a_line_8 );
	r_line = _mm_add_ps( ab, r_line );
	//
	b_line = _mm_set1_ps( *(b + i + 3) );
	ab     = _mm_mul_ps( b_line, a_line_12 );
	r_line = _mm_add_ps( ab, r_line );
	//
	_mm_store_ps( res + i, r_line );

	i = 12;
	// 
	b_line = _mm_set1_ps( *(b + i + 0) );
	r_line = _mm_mul_ps( b_line, a_line_0 );
	//
	b_line = _mm_set1_ps( *(b + i + 1) );
	ab     = _mm_mul_ps( b_line, a_line_4 );
	r_line = _mm_add_ps( ab, r_line );
	//
	b_line = _mm_set1_ps( *(b + i + 2) );
	ab     = _mm_mul_ps( b_line, a_line_8 );
	r_line = _mm_add_ps( ab, r_line );
	//
	b_line = _mm_set1_ps( *(b + i + 3) );
	ab     = _mm_mul_ps( b_line, a_line_12 );
	r_line = _mm_add_ps( ab, r_line );
	//
	_mm_store_ps( res + i, r_line );

	return ret;
}

#if defined( X_PLATFORM_WIN32 )
#   pragma runtime_checks( "", off )
#   pragma warning( push )
#   pragma warning( disable:4700 )
#endif 

inline Matrix44f SseInvert( const Matrix44f& mat )
{
	X_ALIGN16_MATRIX44F( ret );

	X_ASSERT_ALIGNMENT(&mat, 16, 0);

	const float* src = mat.m;
	float* dst = ret.m;

	__m128 minor0, minor1, minor2, minor3;
	__m128 row0, row1, row2, row3;
	__m128 det, tmp1;
	tmp1   = _mm_loadh_pi( _mm_loadl_pi( tmp1, ( __m64* )( src ) ), ( __m64* )( src + 4 ) );
	row1   = _mm_loadh_pi( _mm_loadl_pi( row1, ( __m64* )( src + 8 ) ), ( __m64* )( src + 12 ) );
	row0   = _mm_shuffle_ps( tmp1, row1, 0x88 );
	row1   = _mm_shuffle_ps( row1, tmp1, 0xDD );
	tmp1   = _mm_loadh_pi( _mm_loadl_pi( tmp1, ( __m64* )( src + 2 ) ), ( __m64* )( src + 6 ) );
	row3   = _mm_loadh_pi( _mm_loadl_pi( row3, ( __m64* )( src + 10 ) ), ( __m64* )( src + 14 ) );
	row2   = _mm_shuffle_ps( tmp1, row3, 0x88 );
	row3   = _mm_shuffle_ps( row3, tmp1, 0xDD );
	// ----------------------------------------------- 
	tmp1   = _mm_mul_ps( row2, row3 );
	tmp1   = _mm_shuffle_ps( tmp1, tmp1, 0xB1 );
	minor0 = _mm_mul_ps( row1, tmp1 );
	minor1 = _mm_mul_ps( row0, tmp1 );
	tmp1   = _mm_shuffle_ps( tmp1, tmp1, 0x4E );
	minor0 = _mm_sub_ps( _mm_mul_ps( row1, tmp1 ), minor0 );
	minor1 = _mm_sub_ps( _mm_mul_ps( row0, tmp1 ), minor1 );
	minor1 = _mm_shuffle_ps( minor1, minor1, 0x4E );
	// -----------------------------------------------
	tmp1   = _mm_mul_ps( row1, row2 );
	tmp1   = _mm_shuffle_ps( tmp1, tmp1, 0xB1 );
	minor0 = _mm_add_ps( _mm_mul_ps( row3, tmp1 ), minor0 );
	minor3 = _mm_mul_ps( row0, tmp1 );
	tmp1   = _mm_shuffle_ps( tmp1, tmp1, 0x4E );
	minor0 = _mm_sub_ps( minor0, _mm_mul_ps( row3, tmp1 ) );
	minor3 = _mm_sub_ps( _mm_mul_ps( row0, tmp1 ), minor3 );
	minor3 = _mm_shuffle_ps( minor3, minor3, 0x4E );
	// -----------------------------------------------
	tmp1   = _mm_mul_ps( _mm_shuffle_ps( row1, row1, 0x4E ), row3 );
	tmp1   = _mm_shuffle_ps( tmp1, tmp1, 0xB1 );
	row2   = _mm_shuffle_ps( row2, row2, 0x4E );
	minor0 = _mm_add_ps( _mm_mul_ps( row2, tmp1 ), minor0 );
	minor2 = _mm_mul_ps( row0, tmp1 );
	tmp1   = _mm_shuffle_ps( tmp1, tmp1, 0x4E );
	minor0 = _mm_sub_ps( minor0, _mm_mul_ps( row2, tmp1 ) );
	minor2 = _mm_sub_ps( _mm_mul_ps( row0, tmp1 ), minor2 );
	minor2 = _mm_shuffle_ps( minor2, minor2, 0x4E );
	// -----------------------------------------------
	tmp1   = _mm_mul_ps( row0, row1 );
	tmp1   = _mm_shuffle_ps( tmp1, tmp1, 0xB1 );
	minor2 = _mm_add_ps( _mm_mul_ps( row3, tmp1 ), minor2 );
	minor3 = _mm_sub_ps( _mm_mul_ps( row2, tmp1 ), minor3 );
	tmp1   = _mm_shuffle_ps( tmp1, tmp1, 0x4E );
	minor2 = _mm_sub_ps( _mm_mul_ps( row3, tmp1 ), minor2 );
	minor3 = _mm_sub_ps( minor3, _mm_mul_ps( row2, tmp1 ) );
	// -----------------------------------------------
	tmp1   = _mm_mul_ps( row0, row3 );
	tmp1   = _mm_shuffle_ps( tmp1, tmp1, 0xB1 );
	minor1 = _mm_sub_ps( minor1, _mm_mul_ps( row2, tmp1 ) );
	minor2 = _mm_add_ps( _mm_mul_ps( row1, tmp1 ), minor2 );
	tmp1   = _mm_shuffle_ps( tmp1, tmp1, 0x4E );
	minor1 = _mm_add_ps( _mm_mul_ps( row2, tmp1 ), minor1 );
	minor2 = _mm_sub_ps( minor2, _mm_mul_ps( row1, tmp1 ) );
	// -----------------------------------------------
	tmp1   = _mm_mul_ps( row0, row2 );
	tmp1   = _mm_shuffle_ps( tmp1, tmp1, 0xB1 );
	minor1 = _mm_add_ps( _mm_mul_ps( row3, tmp1 ), minor1 );
	minor3 = _mm_sub_ps( minor3, _mm_mul_ps( row1, tmp1 ) );
	tmp1   = _mm_shuffle_ps( tmp1, tmp1, 0x4E );
	minor1 = _mm_sub_ps( minor1, _mm_mul_ps( row3, tmp1 ) );
	minor3 = _mm_add_ps( _mm_mul_ps( row1, tmp1 ), minor3 );
	// -----------------------------------------------
	det  = _mm_mul_ps( row0, minor0 );
	det  = _mm_add_ps( _mm_shuffle_ps( det, det, 0x4E ), det );
	det  = _mm_add_ss( _mm_shuffle_ps( det, det, 0xB1 ), det );
	tmp1 = _mm_rcp_ss( det );
	det  = _mm_sub_ss( _mm_add_ss( tmp1, tmp1 ), _mm_mul_ss( det, _mm_mul_ss( tmp1, tmp1 ) ) );
	det  = _mm_shuffle_ps( det, det, 0x00 );

	minor0 = _mm_mul_ps( det, minor0 );
	_mm_storel_pi( ( __m64* )( dst ), minor0 );
	_mm_storeh_pi( ( __m64* )( dst + 2 ), minor0 );

	minor1 = _mm_mul_ps( det, minor1 );
	_mm_storel_pi( ( __m64* )( dst + 4 ), minor1 );
	_mm_storeh_pi( ( __m64* )( dst + 6 ), minor1 );

	minor2 = _mm_mul_ps( det, minor2 );
	_mm_storel_pi( ( __m64* )( dst +  8 ), minor2 );
	_mm_storeh_pi( ( __m64* )( dst + 10 ), minor2 );

	minor3 = _mm_mul_ps( det, minor3 );
	_mm_storel_pi( ( __m64* )( dst + 12 ), minor3 );
	_mm_storeh_pi( ( __m64* )( dst + 14 ), minor3 );	

	return ret;
}

#if defined( X_PLATFORM_WIN32 )
#   pragma warning( pop )
#   pragma runtime_checks( "", restore )
#endif

#endif // #if defined( X_PLATFORM_WIN32 ) 

#if 0
X_INLINE void MatrixOrthoOffCenterLH(Matrix44f* pMat, float32_t left, float32_t right,
	float32_t bottom, float32_t top, float32_t zn, float32_t zf)
{

	pMat->m00 = 2.0f / (right - left);
	pMat->m01 = 0;
	pMat->m02 = 0;
	pMat->m03 = 0;

	pMat->m10 = 0;
	pMat->m11 = 2.0f / (top - bottom);
	pMat->m12 = 0;
	pMat->m13 = 0;

	pMat->m20 = 0;
	pMat->m21 = 0;
	pMat->m22 = 1.0f / (zf - zn);
	pMat->m23 = 0;

	pMat->m30 = (left + right) / (left - right);
	pMat->m31 = (top + bottom) / (bottom - top);
	pMat->m32 = zn / (zn - zf);
	pMat->m33 = 1.0f;
}

X_INLINE void MatrixPerspectivOffCenterLH(Matrix44f* pMat, float32_t left, float32_t right,
	float32_t bottom, float32_t top, float32_t zn, float32_t zf)
{

	pMat->m00 = 2.0f * zn / (right - left);
	pMat->m01 = 0;
	pMat->m02 = 0;
	pMat->m03 = 0;

	pMat->m10 = 0;
	pMat->m11 = 2.0f * zn / (top - bottom);
	pMat->m12 = 0;
	pMat->m13 = 0;

	pMat->m20 = (left + right) / (left - right);
	pMat->m21 = (top + bottom) / (bottom - top);
	pMat->m22 = zf / (zf - zn);
	pMat->m23 = 1.f;

	pMat->m30 = 0;
	pMat->m31 = 0;
	pMat->m32 = zn * zf / (zn - zf);
	pMat->m33 = 0;
}

X_INLINE void MatrixLookAtLH(Matrix44f* pMat, const Vec3f& Eye, const Vec3f& At, const Vec3f& Up)
{
	Vec3f vLightDir = (At - Eye);
	Vec3f zaxis = vLightDir.normalized();
	Vec3f xaxis = (Up.cross(zaxis)).normalized();
	Vec3f yaxis = zaxis.cross(xaxis);


	pMat->m00 = xaxis.x;
	pMat->m01 = xaxis.y;
	pMat->m02 = xaxis.z;
	pMat->m03 = 0;

	pMat->m10 = yaxis.x;
	pMat->m11 = yaxis.y;
	pMat->m12 = yaxis.z;
	pMat->m13 = 0;

	pMat->m20 = zaxis.x;
	pMat->m21 = zaxis.y;
	pMat->m22 = zaxis.z;
	pMat->m23 = 0;

	pMat->m03 = -xaxis.dot(Eye);
	pMat->m13 = -yaxis.dot(Eye);
	pMat->m23 = -zaxis.dot(Eye);
	pMat->m33 = 1;
}
#endif

X_INLINE void MatrixPerspectivOffCenterRH(Matrix44f* pMat, float32_t left, float32_t right,
	float32_t bottom, float32_t top, float32_t zn, float32_t zf)
{

	pMat->m00 = 2.0f * zn / (right - left);
	pMat->m10 = 0;
	pMat->m20 = 0;
	pMat->m30 = 0;

	pMat->m01 = 0;
	pMat->m11 = 2.0f * zn / (top - bottom);
	pMat->m21 = 0;
	pMat->m31 = 0;

	pMat->m02 = (left + right) / (right - left);
	pMat->m12 = (top + bottom) / (top - bottom);
	pMat->m22 = zf / (zn - zf);
	pMat->m32 = -1.f;

	pMat->m03 = 0;
	pMat->m13 = 0;
	pMat->m23 = zn * zf / (zn - zf);
	pMat->m33 = 0;
}

// todo
X_INLINE void MatrixOrthoOffCenterRH(Matrix44f* pMat, float32_t left, float32_t right,
	float32_t bottom, float32_t top, float32_t zn, float32_t zf)
{

	pMat->m00 = 2.0f / (right - left);
	pMat->m10 = 0;
	pMat->m20 = 0;
	pMat->m30 = 0;

	pMat->m01 = 0;
	pMat->m11 = 2.0f / (top - bottom);
	pMat->m21 = 0;
	pMat->m31 = 0;

	pMat->m02 = 0;
	pMat->m12 = 0;
	pMat->m22 = 1.0f / (zf - zn);
	pMat->m32 = 0;

	pMat->m03 = (left + right) / (left - right);
	pMat->m13 = (top + bottom) / (bottom - top);
	pMat->m23 = zn / (zn - zf);
	pMat->m33 = 1.0f;
}


X_INLINE void MatrixLookAtRH(Matrix44f* pMat, const Vec3f& Eye, const Vec3f& At, const Vec3f& Up)
{
	Vec3f vLightDir = (Eye - At);
	Vec3f zaxis = vLightDir.normalized();
	Vec3f xaxis = (Up.cross(zaxis)).normalized();
	Vec3f yaxis = zaxis.cross(xaxis);

	pMat->m00 = xaxis.x;
	pMat->m01 = xaxis.y;
	pMat->m02 = xaxis.z;

	pMat->m10 = yaxis.x;
	pMat->m11 = yaxis.y;
	pMat->m12 = yaxis.z;

	pMat->m20 = zaxis.x;
	pMat->m21 = zaxis.y;
	pMat->m22 = zaxis.z;

	// eye is bottom row.
	pMat->m03 = -xaxis.dot(Eye);
	pMat->m13 = -yaxis.dot(Eye);
	pMat->m23 = -zaxis.dot(Eye);

	// last colum.
	pMat->m30 = 0;
	pMat->m31 = 0;
	pMat->m32 = 0;
	pMat->m33 = 1;
}

X_INLINE void D3DXMatrixPerspectiveRH(Matrix44f* pMat, float32_t w, float32_t h,
	float32_t zn, float32_t zf)
{
	X_UNUSED(zf);


	float32_t xmin, xmax, ymin, ymax;
	float32_t width = w;
	float32_t height = h;

	const float fov_x = 90.0f;
	const float fov_y = 2 * toDegrees(atan(h / w));
//	const float fov_y1 = 2 * atan(h / w) * (180.0f / PIf);

	ymax = zn * tan(fov_y* PIf / 360.0f);
	ymin = -ymax;

	xmax = zn * tan(fov_x* PIf / 360.0f);
	xmin = -xmax;

	width = xmax - xmin;
	height = ymax - ymin;

	pMat->m00 = 2 * zn / width;
	pMat->m01 = 0;
	pMat->m02 = (xmax + xmin) / width;
	pMat->m03 = 0;

	pMat->m10 = 0;
	pMat->m11 = 2 * zn / height;
	pMat->m12 = (ymax + ymin) / height;
	pMat->m13 = 0;

	pMat->m20 = 0;
	pMat->m21 = 0;
	pMat->m22 = -0.999f; // make sure it dose not go over
	pMat->m23 = -2.0f * zn;

	pMat->m30 = 0;
	pMat->m31 = 0;
	pMat->m32 = -1;
	pMat->m33 = 0;
}


X_INLINE void MatrixPerspectiveFovRH(Matrix44f* pMat, float32_t fovY, float32_t aspect, 
	float32_t zn, float32_t zf, bool reverseZ)
{
	float32_t yScale = 1.0f / math<float32_t>::tan(fovY * 0.5f);
	float32_t xScale = yScale / aspect;

	float32_t q1, q2;
	float32_t depth;

	if (reverseZ)
	{
		depth = (zf - zn);

		q1 = zn / depth;
		q2 = zf * q1;
	}
	else
	{
		depth = (zn - zf);

		q1 = zf / depth;
		q2 = zn * q1;
	}

	pMat->m00 = xScale;
	pMat->m10 = 0;
	pMat->m20 = 0;
	pMat->m30 = 0;

	pMat->m01 = 0;
	pMat->m11 = yScale;
	pMat->m21 = 0;
	pMat->m31 = 0;

	pMat->m02 = 0;
	pMat->m12 = 0;
	pMat->m22 = q1;
	pMat->m32 = -1;

	pMat->m03 = 0;
	pMat->m13 = 0;
	pMat->m23 = q2;
	pMat->m33 = 0;
}

#endif // _X_MATH_MATRIX_ALGO_H_
