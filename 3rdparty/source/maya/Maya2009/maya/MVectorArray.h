#ifndef _MVectorArray
#define _MVectorArray
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
// CLASS:    MVectorArray
//
// ****************************************************************************
//
// CLASS DESCRIPTION (MVectorArray)
//
//  This class implements an array of integers.  Common convenience functions
//  are available, and the implementation is compatible with the internal
//  Maya implementation so that it can be passed efficiently between plugins
//  and internal maya data structures.
//
// ****************************************************************************

#if defined __cplusplus

// ****************************************************************************
// INCLUDED HEADER FILES


#include <maya/MTypes.h>
#include <maya/MStatus.h>

// ****************************************************************************
// DECLARATIONS

#include <maya/MVector.h>
#include <maya/MStatus.h>

// ****************************************************************************
// CLASS DECLARATION (MVectorArray)

//! \ingroup OpenMaya
//! \brief Array of MVectors data type. 
/*!
This class implements an array of MVectors.  Common convenience functions
are available, and the implementation is compatible with the internal
Maya implementation so that it can be passed efficiently between plugins
and internal maya data structures.
*/
class OPENMAYA_EXPORT MVectorArray
{

public:
					MVectorArray();
					MVectorArray( const MVectorArray& other );
BEGIN_NO_SCRIPT_SUPPORT:
	//!	NO SCRIPT SUPPORT
					MVectorArray( const MVector vectors[], unsigned int count );
END_NO_SCRIPT_SUPPORT:
					MVectorArray( const double vectors[][3], unsigned int count );
					MVectorArray( const float vectors[][3], unsigned int count );
					MVectorArray( unsigned int initialSize,
								  const MVector &initialValue
								  = MVector::zero );
					~MVectorArray();
 	const MVector&	operator[]( unsigned int index ) const;
 	MVectorArray &  operator=( const MVectorArray & other );
	MStatus			set( const MVector& element, unsigned int index );
	MStatus			set( double element[3], unsigned int index );
	MStatus			set( float element[3], unsigned int index );
	MStatus			setLength( unsigned int length );
	unsigned int		length() const;
	MStatus			remove( unsigned int index );
	MStatus			insert( const MVector & element, unsigned int index );
	MStatus			append( const MVector & element );
 	MStatus         copy( const MVectorArray& source );
	MStatus			clear();
	MStatus			get( double [][3] ) const;
	MStatus			get( float [][3] ) const;
	void			setSizeIncrement ( unsigned int newIncrement );
	unsigned int		sizeIncrement () const;

BEGIN_NO_SCRIPT_SUPPORT:

	//!	NO SCRIPT SUPPORT
 	MVector &		operator[]( unsigned int index );

	//!	NO SCRIPT SUPPORT
	friend OPENMAYA_EXPORT std::ostream &operator<<(std::ostream &os,
											   const MVectorArray &array);

END_NO_SCRIPT_SUPPORT:

protected:
// No protected members

private:
	MVectorArray( void* );
	void * arr;
	struct api_data
	{
		double  x;
		double  y;
		double  z;
	};
	const api_data* debugPeekValue ;
    bool   own;
	void syncDebugPeekValue();
	static const char* className();
};

#endif /* __cplusplus */
#endif /* _MVectorArray */
