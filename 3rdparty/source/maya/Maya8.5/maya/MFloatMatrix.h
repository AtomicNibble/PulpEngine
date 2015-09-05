#ifndef _MFloatMatrix
#define _MFloatMatrix
//
//-
// ==========================================================================
// Copyright (C) 1995 - 2006 Autodesk, Inc., and/or its licensors.  All 
// rights reserved.
// 
// The coded instructions, statements, computer programs, and/or related 
// material (collectively the "Data") in these files contain unpublished 
// information proprietary to Autodesk, Inc. ("Autodesk") and/or its 
// licensors,  which is protected by U.S. and Canadian federal copyright law 
// and by international treaties.
// 
// The Data may not be disclosed or distributed to third parties or be 
// copied or duplicated, in whole or in part, without the prior written 
// consent of Autodesk.
// 
// The copyright notices in the Software and this entire statement, 
// including the above license grant, this restriction and the following 
// disclaimer, must be included in all copies of the Software, in whole 
// or in part, and all derivative works of the Software, unless such copies 
// or derivative works are solely in the form of machine-executable object 
// code generated by a source language processor.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND. 
// AUTODESK DOES NOT MAKE AND HEREBY DISCLAIMS ANY EXPRESS OR IMPLIED 
// WARRANTIES INCLUDING, BUT NOT LIMITED TO, THE WARRANTIES OF 
// NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, 
// OR ARISING FROM A COURSE OF DEALING, USAGE, OR TRADE PRACTICE. IN NO 
// EVENT WILL AUTODESK AND/OR ITS LICENSORS BE LIABLE FOR ANY LOST 
// REVENUES, DATA, OR PROFITS, OR SPECIAL, DIRECT, INDIRECT, OR 
// CONSEQUENTIAL DAMAGES, EVEN IF AUTODESK AND/OR ITS LICENSORS HAS 
// BEEN ADVISED OF THE POSSIBILITY OR PROBABILITY OF SUCH DAMAGES. 
// ==========================================================================
//+
//
// CLASS:    MFloatMatrix
//
// *****************************************************************************
//
// CLASS DESCRIPTION (MFloatMatrix)
//
//  This class provides access to Maya's internal matrix math library allowing
//  matrices to be handled easily, and in a manner compatible with internal
//  Maya data structures.
//
//
// *****************************************************************************

#if defined __cplusplus

// *****************************************************************************

// INCLUDED HEADER FILES



#include <maya/MStatus.h>
#include <maya/MTypes.h>

// *****************************************************************************

// DECLARATIONS

#define MFloatMatrix_kTol	1.0e-5F

// *****************************************************************************

// CLASS DECLARATION (MFloatMatrix)

/// A matrix math class for 4x4 matrices of floats. (OpenMaya) (OpenMaya.py)
/**
  This class provides access to Maya's matrix math library
*/
#if defined(_WIN32)
#pragma warning(disable: 4522)
#endif // _WIN32

class OPENMAYA_EXPORT MFloatMatrix  
{

public:
	///
						MFloatMatrix();
	///
						MFloatMatrix( const MFloatMatrix & src );
	///
						MFloatMatrix( const double m[4][4] );
	///
						MFloatMatrix( const float m[4][4] );
	///
						~MFloatMatrix();
	///
 	MFloatMatrix&		operator = (const MFloatMatrix &);
	///
	float				operator()(unsigned int row, unsigned int col ) const;
	///
	const float* 		operator[]( unsigned int row ) const;
	///
	MStatus				get( double dest[4][4] ) const;
	///
	MStatus				get( float dest[4][4] ) const;
	///
 	MFloatMatrix     	transpose() const;
	///
 	MFloatMatrix &   	setToIdentity();
	///
 	MFloatMatrix &   	setToProduct( const MFloatMatrix & left,
									  const MFloatMatrix & right );
	///
 	MFloatMatrix &   	operator+=( const MFloatMatrix & right );
	///
 	MFloatMatrix    	operator+ ( const MFloatMatrix & right ) const;
	///
 	MFloatMatrix &   	operator-=( const MFloatMatrix & right );
	///
 	MFloatMatrix  		operator- ( const MFloatMatrix & right ) const;
	///
 	MFloatMatrix &   	operator*=( const MFloatMatrix & right );
	///
 	MFloatMatrix     	operator* ( const MFloatMatrix & right ) const;
	///
 	MFloatMatrix &   	operator*=( float );
	///
 	MFloatMatrix     	operator* ( float ) const;
	///
 	bool          		operator==( const MFloatMatrix & other ) const;
	///
 	bool           		operator!=( const MFloatMatrix & other ) const;
	///
 	MFloatMatrix     	inverse() const;
	///
 	MFloatMatrix     	adjoint() const;
	///
 	MFloatMatrix     	homogenize() const;
	///
 	float       		det4x4() const;
	///
 	float         		det3x3() const;
	///
 	bool           		isEquivalent( const MFloatMatrix & other,
									  float tolerance = MFloatMatrix_kTol )
 									  const;

BEGIN_NO_SCRIPT_SUPPORT:

	///	NO SCRIPT SUPPORT
	inline float& operator()(unsigned int row, unsigned int col )
	{
		return matrix[row][col];
	}

	///	NO SCRIPT SUPPORT
	inline float* operator[]( unsigned int row )
	{
		return matrix[row];
	}

	///	NO SCRIPT SUPPORT
 	friend OPENMAYA_EXPORT MFloatMatrix	operator* ( float,
												const MFloatMatrix & right );
	///	NO SCRIPT SUPPORT
	friend OPENMAYA_EXPORT std::ostream& operator<< ( std::ostream& os,
												const MFloatMatrix& m );

END_NO_SCRIPT_SUPPORT:

	/// the matrix data
 	float matrix[4][4];

protected:
// No protected members

private:
	static const char* className();
};

#ifdef WANT_GCC41_FRIEND
MFloatMatrix operator* ( float, const MFloatMatrix & right );
#endif


inline float MFloatMatrix::operator()(unsigned int row, unsigned int col ) const
{
	return matrix[row][col];
}


inline const float* MFloatMatrix::operator[]( unsigned int row ) const
{
	return matrix[row];
}

#if defined(_WIN32)
#pragma warning(default: 4522)
#endif // _WIN32


// *****************************************************************************
#endif /* __cplusplus */
#endif /* _MFloatMatrix */
