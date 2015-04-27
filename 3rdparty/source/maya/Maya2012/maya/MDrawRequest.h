#ifndef _MDrawRequest
#define _MDrawRequest
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
// CLASS:    MDrawRequest
//
// ****************************************************************************
//
#if defined __cplusplus

// ****************************************************************************
// INCLUDED HEADER FILES


#include <maya/MStatus.h>
#include <maya/MTypes.h>
#include <maya/MObject.h>
#include <maya/M3dView.h>

// ****************************************************************************
// DECLARATIONS

class MSelectionMask;
class MPoint;
class MPointArray;
class MVector;
class MSelectionList;
class MMatrix;
class MPxSurfaceShapeUI;
class MMaterial;
class MDrawData;

// ****************************************************************************
// CLASS DECLARATION (MDrawRequest)

//! \ingroup OpenMayaUI
//! \brief A draw reqeust used in the draw methods of MPxSurfaceShapeUI 
/*!
This class encapsulates all the information needed to fulfill
a request to draw an object or part of an object.
This class is used by the draw methods of MPxSurfaceShapeUI
derived objects.

The draw request should be created in the overridden
MPxSurfaceShapeUI::getDrawRequests method. Once created the
appropriate "set" methods of this class should be used to define
what is being requested. Then the request should be placed on the
draw reqeust queue using MDrawRequestQueue::add.

When your request gets processed by maya, your overriden
MPxSurfaceShape::draw method will get called with your request.
Use the query methods of this class to determine what to draw.

You create a draw request using the method MDrawInfo::getPrototype.
A draw request automatically picks up certain information (listed
below) upon its creation. So you don't have to set any of this
information unless you want to change it.

Information automatically set by MDrawInfo::getPrototype :

    \li <b>path</b>                path to object to be drawn
    \li <b>view</b>                view to draw in
    \li <b>matrix</b>              world matrix for object
    \li <b>display appearance</b>  how object should be drawn
    \li <b>display status</b>      active, dormant etc.


The draw token is an integer value which you can use to specify
what is to be drawn. This is object specific and so you should
define an enum with the information you require to decide what
is being drawn in your MPxSurfaceShapeUI::draw method.

Here is an example of draw token values for a polygonal mesh
object as defined in an MPxSurfaceShapeUI derived class.

\code
    // Draw Tokens
    //
    enum {
        kDrawVertices, // component token
        kDrawWireframe,
        kDrawWireframeOnShaded,
        kDrawSmoothShaded,
        kDrawFlatShaded,
        kLastToken
    };
\endcode
*/
class OPENMAYAUI_EXPORT MDrawRequest
{
public:
	MDrawRequest();
	MDrawRequest( const MDrawRequest& in );
	~MDrawRequest();

public:

	// the view to draw to
	M3dView					view() const;
	void					setView( M3dView & );
	const MDagPath			multiPath() const;
	void					setMultiPath( const MDagPath & );
	MObject 				component() const;
	void					setComponent( MObject & );
	MDrawData 				drawData() const;
	void					setDrawData( MDrawData & );
	M3dView::DisplayStatus	displayStatus() const;
	void					setDisplayStatus( M3dView::DisplayStatus );
	bool					displayCulling() const;
	void					setDisplayCulling( bool );
	bool					displayCullOpposite() const;
	void					setDisplayCullOpposite( bool );
	M3dView::DisplayStyle	displayStyle() const;
	void					setDisplayStyle( M3dView::DisplayStyle );
	int						color( M3dView::ColorTable table ) const;
	void					setColor( int, M3dView::ColorTable table );
	MMaterial 				material() const;
	void					setMaterial( MMaterial& );
	bool					isTransparent() const;
	void					setIsTransparent( bool );
	bool					drawLast() const;
	void					setDrawLast( bool );
	int						token() const;
	void					setToken( int );
	const MMatrix &			matrix() const;
	void					setMatrix( const MMatrix & );

	MDrawRequest&	operator = ( const MDrawRequest& other );

	static const char*		className();

protected:
// No protected members

private:
    MDrawRequest( void* in, bool own );
	void*	fDrawRequest;
	bool    fOwn;
};

#endif /* __cplusplus */
#endif /* _MDrawRequest */
