#ifndef _MViewportRenderer
#define _MViewportRenderer
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
// CLASS:    MViewportRenderer
//
// ****************************************************************************

#if defined __cplusplus

// ****************************************************************************
// INCLUDED HEADER FILES


#include <maya/MTypes.h>
#include <maya/MString.h>

class MRenderTargetInfo;

// ****************************************************************************
// CLASS DECLARATION (MViewportRenderer)

class MHardwareRenderer;
class MRenderingInfo;

//! \ingroup OpenMayaRender
//! \brief Base class for hardware viewport renderers.
/*!
MViewportRenderer is a class which represents a hardware viewport renderer.
*/
class OPENMAYARENDER_EXPORT MViewportRenderer
{
public:
	//! API used for rendering
	typedef enum
	{
		//! Renderer uses hardware OpenGL for rendering
		kOpenGL,
		//! Renderer uses hardware Direct3D for rendering
		kDirect3D,
		//! Renderer renders using software 
		kSoftware
	} RenderingAPI;

	//! Override status
	typedef enum {
		//! Override nothing
		kNoOverride,
		//! Override all drawing
		kOverrideAllDrawing,
		//! Override all drawing, then follow with a standard render pass
		kOverrideThenStandard,
	} RenderingOverride;

	//! Rendering exclusions when RenderingOverride is kOverrideThenStandard
	typedef enum {
		kExcludeNone				= 0,
		kExcludeNurbsCurves			= 1<<(0),
		kExcludeNurbsSurfaces		= 1<<(1),
		kExcludeMeshes				= 1<<(2),
		kExcludePlanes				= 1<<(3),
		kExcludeLights				= 1<<(4),
		kExcludeCameras				= 1<<(5),
		kExcludeJoints				= 1<<(6),
		kExcludeIkHandles			= 1<<(7),
		kExcludeDeformers			= 1<<(8),
		kExcludeDynamics			= 1<<(9),
		kExcludeLocators			= 1<<(10),
		kExcludeDimensions			= 1<<(11),
		kExcludeSelectHandles		= 1<<(12),
		kExcludePivots				= 1<<(13),
		kExcludeTextures			= 1<<(14),
		kExcludeGrid				= 1<<(15),
		kExcludeCVs					= 1<<(16),
		kExcludeHulls				= 1<<(17),
		kExcludeStrokes				= 1<<(18),
		kExcludeSubdivSurfaces		= 1<<(19),
		kExcludeFluids				= 1<<(20),
		kExcludeFollicles			= 1<<(21),
		kExcludeHairSystems			= 1<<(22),
		kExcludeImagePlane			= 1<<(23),
		kExcludeNCloths				= 1<<(24),
		kExcludeNRigids				= 1<<(25),
		kExcludeDynamicConstraints	= 1<<(26),
		kExcludeManipulators		= 1<<(27),
		kExcludeNParticles			= 1<<(28),
		kExcludeAll					= ~0,
	} RenderingExclusion;

	//! Constructor
	MViewportRenderer(const MString & name);

	//! Destructor
	virtual ~MViewportRenderer();

	// Virtual overrides
	//
	//! Renderer initialization.
	virtual	MStatus	initialize() = 0;
	//! Renderer cleanup.
	virtual	MStatus	uninitialize() = 0;

	//! Method which does the actual rendering.
	virtual MStatus	render( const MRenderingInfo &info ) = 0;

	//! Query the native rendering API's supported by this renderer. 
	virtual bool	nativelySupports( MViewportRenderer::RenderingAPI api, 
										  float version ) = 0;
	//! Check if override exists
	virtual bool	override( MViewportRenderer::RenderingOverride override ) = 0;

	//! Rendering exclusion for standard pass of kOverrideThenStandard
	virtual unsigned int	overrideThenStandardExclusion() const;

	// Registration methods
	MStatus			registerRenderer() const;
	MStatus			deregisterRenderer() const;

	// Basic access methods
	//
	const MString &	name() const;

	const MString &	UIname() const;
	void			setUIName(const MString &name );

	MViewportRenderer::RenderingOverride  renderingOverride() const;
	void			setRenderingOverride( RenderingOverride override );

protected:
	// Default constructor is protected
	MViewportRenderer();

	friend class MHardwareRenderer;

	// Rendering identification
	MString			fName;
	MString			fUIName;

	// Rendering override information
	RenderingOverride	fRenderingOverride;

private:
// No private members

};

#endif
#endif // _MViewportRenderer
