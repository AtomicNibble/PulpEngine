#ifndef _MPxGeometryOverride
#define _MPxGeometryOverride
//-
// Copyright 2015 Autodesk, Inc. All rights reserved.
// 
// Use of this software is subject to the terms of the Autodesk
// license agreement provided at the time of installation or download,
// or which otherwise accompanies this software in either electronic
// or hard copy form.
//+
//
// CLASS:    MPxGeometryOverride
//
// ****************************************************************************
//
// CLASS DESCRIPTION (MPxGeometryOverride)
//
//  MPxGeometryOverride allows the user to create an override to prepare
//  vertex data for a Maya DAG object for drawing with an arbitrary shader in
//  Viewport 2.0.
//
// ****************************************************************************

#if defined __cplusplus

// ****************************************************************************
// INCLUDED HEADER FILES

#include <maya/MUintArray.h>
#include <maya/MFloatArray.h>
#include <maya/MViewport2Renderer.h>
#include <maya/MUIDrawManager.h>

// ****************************************************************************
// DECLARATIONS

class MObject;
class MDagPath;
class MSelectionMask;


// ****************************************************************************
// NAMESPACE

namespace MHWRender
{

class MGeometry;
class MGeometryRequirements;
class MRenderItemList;
class MRenderItem;
class MSelectionContext;
class MVertexBufferDescriptor;
class MSelectionInfo;

// ****************************************************************************
// CLASS DECLARATION (MPxGeometryOverride)

//! \ingroup OpenMayaRender MPx
//! \brief Base for user-defined classes to prepare geometry for drawing
/*!
MPxGeometryOverride allows the user to create an override to prepare vertex
data that will be used to draw a specific Maya DAG object type with an
arbitrary shader (standard Maya or custom) in Viewport 2.0.

This class is designed to be a high level data-translator independent of any
specific hardware draw API. Once registered through MDrawRegistry, an instance
will be created for each node with a matching classification string. That
instance will be used to generate the vertex streams needed by the assigned
shaders in order to draw the object. The intent of this class is that it be
used to provide data for plugin shape types (MPxSurfaceShape) however it can
also be used to override the geometry translation for any Maya geometry type.

If a more low-level interface to the Viewport 2.0 draw loop is needed, look
at either MPxDrawOverride or MPxShaderOverride.

Users of this interface must implement several virtual methods which will be
called at specific times during the draw-preparation phase.

1) updateDG() : In the updateDG() call, all data needed to compute the indexing
and geometry data must be pulled from Maya and cached. It is invalid to query
attribute values from Maya nodes in any later stage and doing so may result in
instability.

2) updateRenderItems() : For each shader assigned to the instance of the object
Maya will assign a render item. A render item is a single atomic renderable
entity containing a shader and some geometry. In updateRenderItems(),
implementations of this class may enable or disable the automatic shader-based
render items and they may add or remove custom user defined render items in
order to cause additional things to be drawn. Look at the MRenderItem interface
for more details.

3) addUIDrawables() : For each instance of the object, besides the render
items updated in 'updateRenderItems()' for the geometry rendering, there is also
a render item list there for render the simple UI elements. 'addUIDrawables()'
happens just after normal geometry item updating, The original design for this stage
is to allow user accessing 'MUIDrawManager' which helps drawing the simple geometry
easily like line, circle, rectangle, text, etc.
Overriding this methods is not always necessary, but if you want to override it, please also
override 'hasUIDrawables()' to make it return true or the overrided method will not be called.

4) populateGeometry() : In this method the implementation is expected to fill
the MGeometry data structure with the vertex and index buffers required to draw
the object as indicated by the data in the geometry requirements instance
passed to this method. Failure to fulfill the geometry requirements may result
in incorrect drawing or possibly complete failure to draw the object.

5) cleanUp() : Delete any cached data generated in the earlier phases that is
no longer needed.

The override is only triggered when the associated DAG object has changed and
that object is about to be drawn. So it is not invoked if the user is simply
tumbling the scene, or if the object is not within the current view frustum.

Implementations of MPxGeometryOverride must be registered with Maya through
MDrawRegistry.
*/
class OPENMAYARENDER_EXPORT MPxGeometryOverride
{
public:

	MPxGeometryOverride(const MObject& obj);
	virtual ~MPxGeometryOverride();

	virtual MHWRender::DrawAPI supportedDrawAPIs() const;
	virtual bool hasUIDrawables() const;

	virtual void updateDG() = 0;
	virtual bool isIndexingDirty(
		const MRenderItem& item);
	virtual bool isStreamDirty(
		const MVertexBufferDescriptor& desc);
	virtual void updateRenderItems(
		const MDagPath& path,
		MRenderItemList& list) = 0;
	virtual void addUIDrawables(
		const MDagPath& path,
		MUIDrawManager& drawManager,
		const MFrameContext& frameContext);
	virtual void populateGeometry(
		const MGeometryRequirements& requirements,
		const MHWRender::MRenderItemList& renderItems,
		MGeometry& data) = 0;
	virtual void cleanUp() = 0;

	virtual bool refineSelectionPath(const MSelectionInfo& selectInfo,
									 const MRenderItem& hitItem, 
									 MDagPath& multipath,
									 MObject& geomComponents,
									 MSelectionMask& objectMask);

	virtual void updateSelectionGranularity(
		const MDagPath& path,
		MSelectionContext& selectionContext);

	static bool pointSnappingActive();

	static	const char*	className();

private:
	MPxGeometryOverride() {}

};

} // namespace MHWRender

#endif /* __cplusplus */
#endif /* _MPxGeometryOverride */
