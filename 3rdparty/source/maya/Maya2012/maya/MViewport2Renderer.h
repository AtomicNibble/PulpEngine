#ifndef _MRenderer
#define _MRenderer
//-
// ==========================================================================
// Copyright 2010 Autodesk, Inc.  All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license agreement
// provided at the time of installation or download, or which otherwise
// accompanies this software in either electronic or hard copy form.
//
//+
//
// CLASS:    MHWRender::MRenderer
//
// ****************************************************************************
//
// CLASS DESCRIPTION (MHWRender::MRenderer)
//
//  MHWRender::MRenderer is the main interface class to the renderer
//	which is used for rendering interactive viewports in "Viewport 2.0" mode
//	as well as for rendering with the "Maya Harware 2.0" batch renderer.
//
// ****************************************************************************

#if defined __cplusplus

// ****************************************************************************
// INCLUDED HEADER FILES

#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MDagPath.h>
#include <maya/MMatrix.h>
#include <maya/MFloatPoint.h>

class MSelectionList;

// ****************************************************************************
// NAMESPACE
namespace MHWRender
{

// ****************************************************************************
// DECLARATIONS

class MTextureDescription;
class MRenderOverride;
class MShaderManager;
class MShaderInstance;
class MRenderTargetManager;
class MRenderTarget;
class MTextureManager;
class MDrawContext;

/*! Pixel / raster formats

The following short form notation is used for
channel specification:

- R = red channel
- G = green channel
- B = blue channel
- A = alpha channel
- E = exponent channel
- L = luminence channel
- X = channel is not used
- DXT1, DXT2, DXT3, DXT4, and DXT5 are S3 compression formats
- Numbers after the channel gives the bit depth
- Channel order is data storage order.

The following short form notation is used for
data format specification:

- UNORM means unsigned values which have been normalized to the  0 to 1 range.
- SNORM means signed values which have been normalized to the -1 to 1 range.
- UINT means unsigned integer values
- SINT means signed integer values
- FLOAT means floating point
- If normalization is not explicitly specified then the values are unnormalized
*/
enum MRasterFormat {
	// Depth formats
	kD24S8 = 0,		//!< Depth: 24-bit, Stencil 8-bit
	kD24X8,			//!< Depth: 24-bit
	kD32_FLOAT,		//!< Depth 32-bit

	kR24G8,			//!< Red 24-bit, Green 8-bit
	kR24X8,			//!< Red 24-bit

	// Compressed formats
	kDXT1_UNORM,		//!< DXT1 : unsigned
	kDXT1_UNORM_SRGB,	//!< DXT1 : unsigned, sRGB format
	kDXT2_UNORM,		//!< DXT2 : unsigned
	kDXT2_UNORM_SRGB,	//!< DXT2 : sRGB format
	kDXT2_UNORM_PREALPHA, //!< DXT2, pre-multiplied alpha
	kDXT3_UNORM,		//!< DXT3 : unsigned
	kDXT3_UNORM_SRGB,	//!< DXT3 : unsigned, sRGB format
	kDXT3_UNORM_PREALPHA, //!< DXT3, pre-multiplied alpha
	kDXT4_UNORM,		//!< DXT4 : unsigned
	kDXT4_SNORM,		//!< DXT4 : signed
	kDXT5_UNORM,		//!< DXT5 : unsigned
	kDXT5_SNORM,		//!< DXT5 : signed

	kR9G9B9E5_FLOAT, //!< HDR format : 9 bits for each of RGB, no alpha, 5 bit shared exponent

	// 1-bit formats
	kR1_UNORM,		//!< Red: 1-bit

	// 8-bit formats
	kA8,			//!< Alpha: 8-bit
	kR8_UNORM,		//!< Red: 8-bit
	kR8_SNORM,      //!< Red: 8-bit signed
	kR8_UINT,       //!< Red: 8-bit unsigned integer
	kR8_SINT,		//!< Red: 8-bit signed integer
	kL8,				//!< Luminence: 8-bit

	// 16-bit formats
	kR16_FLOAT,		//!< Red: 16-bit float
	kR16_UNORM,		//!< Red: 16-bit unsigned
	kR16_SNORM,		//!< Red: 16-bit signed
	kR16_UINT,		//!< Red: 16-bit unsigned integer
	kR16_SINT,		//!< Red: 16-bit signed integer
	kL16,			//!< Luminence, 16-bit
	kR8G8_UNORM,	//!< Red: 8-bit, Green : 8-bit, unsigned
	kR8G8_SNORM,	//!< Red: 8-bit, Green : 8-bit, signed
	kR8G8_UINT,		//!< Red: 8-bit, Green : 8-bit, unsigned integer
	kR8G8_SINT,		//!< Red: 8-bit, Green : 8-bit, signed integer
	kB5G5R5A1,		//!< RGB : 5-bits each, Alpha : 1-bit
	kB5G6R5,		//!< RGB : 5-bits each, Alpha : 1-bit

	// 32-bit formats
	kR32_FLOAT,		//!< Red : 32-bit float
	kR32_UINT,		//!< Red : 32-bit unsigned integer
	kR32_SINT,		//!< Red : 32-bit signed integer
	kR16G16_FLOAT,	//!< Red and green : 16-bit float each
	kR16G16_UNORM,	//!< Red and green : 16-bit unsigned
	kR16G16_SNORM,	//!< Red and green : 16-bit signed
	kR16G16_UINT,	//!< Red and green : 16-bit unsigned
	kR16G16_SINT,	//!< Red and green : 16-bit signed
	kR8G8B8A8_UNORM, //!< RGBA : 8-bits unsigned each
	kR8G8B8A8_SNORM, //!< RGBA : 8-bits signed each
	kR8G8B8A8_UINT,	//!< RGBA : 8-bits unsigned integer each
	kR8G8B8A8_SINT,	//!< RGBA : 8-bits signed integer each
	kR10G10B10A2_UNORM, //!< 2 bit alpha, 10 bits for each of RGB
	kR10G10B10A2_UINT, //!< 2 bit alpha, 10 bits for each of RGB, unsigned integer
	kB8G8R8A8,		//!< BGRA : 8-bits each
	kB8G8R8X8,		//!< BGR : 8-bits each. No alpha
	kR8G8B8X8,      //!< RGB : 8-bits each
	kA8B8G8R8,		//< ABGR : 8-bits each

	// 64 bit formats
	kR32G32_FLOAT,	//!< RG : 32-bits float each
	kR32G32_UINT,	//!< RG : 32-bits unsigned each
	kR32G32_SINT,	//!< RG : 32-bits signed each
	kR16G16B16A16_FLOAT, //!< RGBA : 16-bits float each
	kR16G16B16A16_UNORM, //!< RGBA : 16-bits unsigned each
	kR16G16B16A16_SNORM, //!< RGBA : 16-bits signed each
	kR16G16B16A16_UINT, //!< RGBA : 16-bits unsigned integer each
	kR16G16B16A16_SINT, //!< RGBA : 16-bits unsigned integer each

	// 96-bit formats
	kR32G32B32_FLOAT, //!< RGB : 32-bits float each
	kR32G32B32_UINT, //!< RGB : 32-bits unsigned integer each
	kR32G32B32_SINT, //!< RGB : 32-bits signed integer each

	// 128-bit formats
	kR32G32B32A32_FLOAT, //!< RGBA : 32-bits float each
	kR32G32B32A32_UINT, //!< RGBA : 32-bits unsigned integer each
	kR32G32B32A32_SINT, //!< RGBA : 32-bits signed integer each

	kNumberOfRasterFormats //!< Not to be used to describe a raster. This is the number of rasters formats
};


// ****************************************************************************
/*! Camera override description. Provides information for specifying
	a camera override for a render operation.
*/
class OPENMAYARENDER_EXPORT MCameraOverride
{
public:
	MCameraOverride()
	{
		mUseProjectionMatrix = false;
		mUseViewMatrix = false;
		mUseNearClippingPlane = false;
		mUseFarClippingPlane = false;
	}
	MDagPath mCameraPath;		//!< Camera path override
	bool mUseProjectionMatrix;	//!< Whether to use the projection matrix override
	MMatrix mProjectionMatrix;	//!< Camera projection matrix override
	bool mUseViewMatrix;		//!< Whether to use the view matrix override
	MMatrix mViewMatrix;		//!< Camera view matrix override
	bool mUseNearClippingPlane; //!< Whether to use the near clipping plane override
	double mNearClippingPlane;  //!< Near clipping plane override
	bool mUseFarClippingPlane;  //!< Whether to use the far clipping plane override
	double mFarClippingPlane;   //!< Far clipping plane override
};

// ****************************************************************************
// CLASS DECLARATION (MRenderOperation)
//! \ingroup OpenMayaRender
//! \brief Class which defines a rendering operation
//
class OPENMAYARENDER_EXPORT MRenderOperation
{
public:
	virtual ~MRenderOperation();

	// Render target overrides
	virtual MRenderTarget** targetOverrideList(unsigned int &listSize);

	// Viewport rectangle override
	virtual const MFloatPoint * viewportRectangleOverride();

	// Identifier query
	virtual const MString & name() const;

	/*!
		Supported render operation types
	*/
	enum MRenderOperationType
	{
		kClear, //!< Clear background operation
		kSceneRender, //!< Render a 3d scene
		kQuadRender, //!< Render a 2d quad
		kUserDefined, //!< User defined operation
		kHUDRender, //!< 2D HUD draw operation
		kPresentTarget //!< Present target for viewing
	};

	// Type identifier query
	MRenderOperationType operationType() const;

protected:
	MRenderOperation( const MString & name );
	MRenderOperation();

	// Operation type
	MRenderOperationType mOperationType;

	// Identifier for a sub render
	MString mName;
};

// ****************************************************************************
// CLASS DECLARATION (MUserRenderOperation)
//! \ingroup OpenMayaRender
//! \brief Class which defines a user defined rendering operation
//
class OPENMAYARENDER_EXPORT MUserRenderOperation : public MRenderOperation
{
public:
	MUserRenderOperation(const MString &name);
	virtual ~MUserRenderOperation();

	// Camera override
	virtual const MCameraOverride * cameraOverride();

	// Derived classes define the what the operation does
	virtual MStatus execute(const MDrawContext & drawContext ) = 0;
};

// ****************************************************************************
// CLASS DECLARATION (MHUDRender)
//! \ingroup OpenMayaRender
//! \brief Class which defines rendering the 2D heads-up-display
//
class OPENMAYARENDER_EXPORT MHUDRender : public MRenderOperation
{
public:
	MHUDRender();
	virtual ~MHUDRender();

	const MString & name() const;
};

// ****************************************************************************
// CLASS DECLARATION (MPresentTarget)
//! \ingroup OpenMayaRender
//! \brief Class which defines the operation of
//!  presenting a target for final output.
//
class OPENMAYARENDER_EXPORT MPresentTarget : public MRenderOperation
{
public:
	MPresentTarget(const MString &name);
	virtual ~MPresentTarget();

	bool presentDepth() const;
	void setPresentDepth(bool val);
protected:
	bool mPresentDepth;
};

// ****************************************************************************
// CLASS DECLARATION (MClearOperation)
//! \ingroup OpenMayaRender
//! \brief Class which defines the operation of
//!  clearing render target channels.
//
class OPENMAYARENDER_EXPORT MClearOperation : public MRenderOperation
{
public:

	/*!
		ClearMask describes the set of channels to clear
		If the mask value is set then that given channel will
		be cleared.
	*/
	enum ClearMask
	{
		kClearNone = 0, //!< Clear nothing
		kClearColor = 1, //!< Clear color
		kClearDepth = 1 << 1 , //!< Clear depth
		kClearStencil = 1 << 2, //!< Clear stencil
		kClearAll = ~0 // !< Clear all
	};

	unsigned int mask() const;
	const float* clearColor() const;
	bool clearGradient() const;
	const float* clearColor2() const;
	int clearStencil() const;
	float clearDepth() const;

	void setMask( unsigned int mask );
	void setClearColor( float value[4] );
	void setClearGradient( bool value );
	void setClearColor2( float value[4] );
	void setClearStencil( int value );
	void setClearDepth( float value );

protected:
	unsigned int mClearMask;
	float mClearColor[4];
	float mClearColor2[4];
	bool mClearGradient;
	int mClearStencil;
	float mClearDepth;
private:
	MClearOperation(const MString &name);
	virtual ~MClearOperation();
	MClearOperation();

	friend class MSceneRender;
	friend class MQuadRender;
};



// ****************************************************************************
// CLASS DECLARATION (MSceneRender)
//! \ingroup OpenMayaRender
//! \brief Class which defines a scene render
//
class OPENMAYARENDER_EXPORT MSceneRender : public MRenderOperation
{
public:
	//! Object type exclusions
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
		kExcludeAll					= ~0
	} MObjectTypeExclusions;

	MSceneRender(const MString &name);
	virtual ~MSceneRender();

	// Operation pre and post render callbacks
	virtual void preRender();
	virtual void postRender();

	//! Render filter options. Refer to the renderFilterOverride() method
	//! for details on usage.
	typedef enum
	{
		kNoSceneFilterOverride = 0,
		kRenderShadedItems		= 1 << (0),	//!< Render only shaded objects but not their wireframe or components
		kRenderNonShadedItems	= 1 << (1),	//!< Render wireframe and components for surfaces as well as non-surface objects.
		kRenderAllItems		= ~0			//!< Render all items.
	} MSceneFilterOption;
	virtual MSceneFilterOption renderFilterOverride();

	// Camera override
	virtual const MCameraOverride * cameraOverride();

	// Object set filter
	virtual const MSelectionList * objectSetOverride();

	// Shader override
	virtual const MShaderInstance* shaderOverride();

	// Object type exclusions
	virtual MObjectTypeExclusions objectTypeExclusions();

	//! Display modes
    typedef enum {
		kNoDisplayModeOverride = 0,	//!< No display mode override
        kWireFrame = 1<<(0),		//!< Display wireframe
		kShaded = 1<<(1),			//!< Display shaded
		kDefaultMaterial = 1<<(2)	//!< Use default material. Only applicable if shaded mode enabled.
    } MDisplayMode;
	virtual MDisplayMode displayModeOverride();

	//! Lighting mode
	typedef enum {
		kNoLightingModeOverride = 0, //!< No lighting mode override
		kAmbientLight,		//!< Use global ambient light
		kLightDefault,		//!< Use default directional light
		kSceneLights		//!< Use the lights in the scene
	} MLightingMode ;
	virtual MLightingMode lightModeOverride();
	virtual const bool* shadowEnableOverride();

	//! Culling option
	typedef enum {
		kNoCullingOverride = 0,	//!< No culling override
		kCullNone,				//!< Don't perform culling
		kCullBackFaces,			//!< Cull back faces
		kCullFrontFaces,		//!< Cull front faces
	} MCullingOption;
	virtual MCullingOption cullingOverride();

	// Clear operation helper
	virtual MClearOperation & clearOperation();
protected:
	MClearOperation mClearOperation;
};

// ****************************************************************************
// CLASS DECLARATION (MQuadRender)
//! \ingroup OpenMayaRender
//! \brief Class which defines a 2d geometry quad render
//
class OPENMAYARENDER_EXPORT MQuadRender : public MRenderOperation
{
public:
	MQuadRender(const MString &name);
	virtual ~MQuadRender();

	// Shader
	virtual const MShaderInstance* shader();

	// Clear operation helper
	virtual MClearOperation & clearOperation();
protected:
	MClearOperation mClearOperation;
};

// ****************************************************************************
// CLASS DECLARATION (MRenderOverride)
//! \ingroup OpenMayaRender
//! \brief Base class for defining a rendering override
//
class OPENMAYARENDER_EXPORT MRenderOverride
{
public:
	MRenderOverride( const MString & name );
	virtual ~MRenderOverride();

	// Methods to iterate through operations
	// on an override
	virtual bool startOperationIterator() = 0;
	virtual MRenderOperation * renderOperation() = 0;
	virtual bool nextRenderOperation() = 0;

	// Identifier
	const MString & name() const;

	// Pre / post setup and cleanup methods
	virtual MStatus setup( const MString & destination );
	virtual MStatus cleanup();

	// Unique identifier for the override
	MString mName;
private:
	friend class M3dView;
	MRenderOverride();
};

// ****************************************************************************
// CLASS DECLARATION (MRenderer)
//! \ingroup OpenMayaRender
//! \brief Main interface class to the Viewport 2.0 renderer.
//
class OPENMAYARENDER_EXPORT MRenderer
{
public:
	// The renderer
	static MRenderer *theRenderer();

	//////////////////////////////////////////////////////////////////
	// Drawing API information
	//
	bool drawAPIIsOpenGL() const;
	unsigned int drawAPIVersion() const;

	//////////////////////////////////////////////////////////////////
	// Shader manager
	//
	const MShaderManager* getShaderManager() const;

	//////////////////////////////////////////////////////////////////
	// Target manager
	//
	const MRenderTargetManager* getRenderTargetManager() const;

	//////////////////////////////////////////////////////////////////
	// Texture manager
	//
	MTextureManager* getTextureManager() const;

	//////////////////////////////////////////////////////////////////
	// Render override methods
	//
	MStatus registerOverride(const MRenderOverride *roverride);
	MStatus deregisterOverride(const MRenderOverride *roverride);
	const MRenderOverride * findRenderOverride( const MString &name );
	const MString activeRenderOverride() const;
	unsigned int renderOverrideCount();
    MStatus setRenderOverrideName( const MString & name );
    MString renderOverrideName( MStatus *ReturnStatus = NULL ) const;

    // Get target size
    MStatus outputTargetSize(unsigned int& w, unsigned int& h)const;

	static void setGeometryDrawDirty(MObject obj);
private:
	static MRenderer *fsTheRenderer;

	MShaderManager* fShaderManager;
	MRenderTargetManager* fRenderTargetManager;
	MTextureManager* fTextureManager;
	unsigned int fRasterMap[kNumberOfRasterFormats];
	bool fInitialized;

	MRenderer();
	~MRenderer();
	// Left unimplemented on purpose to prevent methods
	// from being called.
	MRenderer( const MRenderer &other );
	MRenderer& operator=( const MRenderer &rhs );

};

} // namespace MHWRender

#endif /* __cplusplus */
#endif /* _MRenderer */
