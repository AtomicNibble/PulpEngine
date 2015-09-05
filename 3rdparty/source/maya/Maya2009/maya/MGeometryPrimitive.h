#ifndef _MGeometryPrimitive
#define _MGeometryPrimitive
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
// CLASS:    MGeometryPrimitive
//
// ****************************************************************************

#if defined __cplusplus

// ****************************************************************************
// INCLUDED HEADER FILES


#include <maya/MTypes.h>
#include <maya/MGeometryData.h>

class MHardwareRenderer;
class MGeometry;

// ****************************************************************************
// CLASS DECLARATION (MGeometryPrimitive)

//! \ingroup OpenMayaRender
//! \brief Geometric index container
/*!
  MGeometryPrimitive is a class describes the topology used for
  accessing MGeometryData.

  Topology is specified as a set of index values which references into
  data elements in an MGeometryData. Index values can be assumed to be
  stored in contiguous memory.

  A "draw primitive type" indicates the format of the indexing as follows:

	\li kPoint : individual unconnected points.
	\li kLine : individual unconnected line segments.
	\li kLineStrip : connected line strip.
	\li kLineLoop : closed line loop.
	\li kTriangle : filled triangle.
	\li kTriangleStrip : filled triangle strip.
	\li kTriangleFan : filled triangle fan
	\li kQuad : filled quadrilateral (quad).
	\li kQuadStrip : filled quad strip.
	\li kPolygon : filled N-sided polygon


  Internal Maya data which is passed to the user via this class is
  always assumed to be non-modifiable. <b>If modified, stability
  cannot be ensured.</b>
*/
class OPENMAYARENDER_EXPORT MGeometryPrimitive
{
public:
	//! Specifies the data primitive type constructed by the indexing array
	enum DrawPrimitiveType {
		//! Default value is not valid
		kInvalidIndexType = 0,
		//! Corresponds to GL_POINTS in OpenGL
		kPoints,
		//! Corresponds to GL_LINES in OpenGL (individual unconnected line segments)
		kLines,
		//! Corresponds to GL_LINE_STRIP in OpenGL
		kLineStrip,
		//! Corresponds to GL_LINE_LOOP	in OpenGL (non-filled, connected line segments)
		kLineLoop,
		//! Corresponds to GL_TRIANGLES In OpenGL
		kTriangles,
		//! Corresponds to GL_TRIANGLE_STRIP in OpenGL
		kTriangleStrip,
		//! Corresponds to GL_TRIANGLE_FAN in OpenGL
		kTriangleFan,
		//! Corresponds to GL_QUADS in OpenGL
		kQuads,
		//! Corresponds to GL_QUAD_STRIP in OpenGL
		kQuadStrip,
		//! Corresponds to GL_POLYGON in OpenGL
		kPolygon,
		//! Number of primitive types.
		kMaxDrawPrimitiveTypeIndex	// Valid entries are < kMaxDrawModeIndex
	};

	//! Destructor
	~MGeometryPrimitive();

	int				uniqueID() const;
	DrawPrimitiveType drawPrimitiveType() const;
    unsigned int	elementCount() const;
	MGeometryData::ElementType dataType() const;
    const void *	data() const;

protected:
	// Default constructor is protected
	MGeometryPrimitive();
	MGeometryPrimitive(void *);
	//
	// Data set and release is protected
	void set( void* );
	void release();

	friend class MGeometry;
	friend class MVaryingParameter;
	friend class MHardwareRenderer;

	void *_pGeometryIndex;

private:
// No private members

};

#endif
#endif // _MGeometryPrimitive
