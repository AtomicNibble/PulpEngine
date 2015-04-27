#ifndef _MTextureManager
#define _MTextureManager
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
// CLASS:    MHWRender::MTextureManager
//
// ****************************************************************************
//
// CLASS DESCRIPTION (MHWRender::MTextureManager)
//
//  MHWRender::MTextureManager is the main interface class to get texture
//	which is used for rendering interactive viewports in "Viewport 2.0" mode
//	as well as for rendering with the "Maya Harware 2.0" batch renderer.
//
// ****************************************************************************

#if defined __cplusplus

// ****************************************************************************
// INCLUDED HEADER FILES

#include <maya/MViewport2Renderer.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MPlug.h>

// ****************************************************************************
// NAMESPACE
namespace MHWRender
{
// ****************************************************************************
// DECLARATIONS

//! Texture types
enum MTextureType {
	kImage1D = 0,	//!< 1D image
	kImage1DArray,	//!< Array of 1D images
	kImage2D,		//!< 2D image
	kImage2DArray,	//!< Array of 2D images
	kCubeMap,		//!< Cube map
	kVolumeTexture,	//!< Volume texture
	kDepthTexture,	//!< Depth texture

	kNumberOfTextureTypes //! Not to be used to describe a raster. This is the number of texture types.
};

/*! Environment mapping types for textures. Describes the algorithm for
how the data in the texture should be looked up.
*/
enum MEnvironmentMapType {
	kEnvNone = 0,	//!< Direct mapping
	kEnvSphere,		//!< Sphere style environment mapping
	kEnvHemiSphere,	//!< Hemi-sphere style environment mapping
	kEnvLatLong,	//!< Latitude/longitude style environment map
	kEnvCrossVert,	//!< Vertical cross style environment map
	kEnvCrossHoriz,	//!< Horizontal cross style environment map
	kEnvCubemap,	//!< 6 faces cube map style environment map

	kNumberOfEnvMapTypes //! Not to be used to describe a raster. This is the number of environment map types.
};

/*! Texture description. Provides sufficient information to describe how
	a block of data can be interpreted as a texture.
*/
class MTextureDescription
{
public:
	//! Utility to set texture description to describe a 0 size 2-dimensional texture
	void setToDefault2DTexture()
	{
		fWidth = 0;
		fHeight = 0;
		fDepth = 1;				// depth of 1
		fBytesPerRow = 0;
		fBytesPerSlice = 0;
		fMipmaps = 1;			// No mip-maps
		fArraySlices = 1;		// 1 slice
		fFormat = kR8G8B8A8_UNORM; // "standard" fixed-point RGBA8
		fTextureType = kImage2D; // 2D texture
		fEnvMapType = kEnvNone; // No special lookup
	}

	unsigned int fWidth;		//!< Width in pixels
	unsigned int fHeight;		//!< Height in pixels
	unsigned int fDepth;		//!< Depth in pixels. A 2D texture has depth of 1.
	unsigned int fBytesPerRow;	//!< Number of bytes in a row of pixels
	unsigned int fBytesPerSlice;//!< Number of bytes in a slice (if an array)
	unsigned int fMipmaps;		//!< Number of mipmap levels. 0 means the entire mipmap chain.
	unsigned int fArraySlices;	//!< Number of array slices. e.g. 6 would be required for a cube-map
	MRasterFormat fFormat;		//!< Pixel / raster format
	MTextureType	fTextureType;  //!< Type of texture
	MEnvironmentMapType fEnvMapType;	//!< Type of environment mapping
};

// ****************************************************************************
// CLASS DECLARATION (MTexture)
//! \ingroup OpenMayaRender
//!\brief Class which includes texture data
//
class OPENMAYARENDER_EXPORT MTexture
{
public:
	~MTexture();
	// Access to draw API dependent texture data
	void* resourceHandle() const;
	const MString& name() const;
	void textureDescription(MTextureDescription& desc) const;

	static const char* className();

private:
	MTexture(const MString& name, void* data);
	MString fName;
	void* fData;

	friend class MTextureManager;
	friend class MShaderInstance;
};

// ****************************************************************************
// CLASS DECLARATION (MTextureManager)
//! \ingroup OpenMayaRender
//! \brief Class which manages texture
//
class OPENMAYARENDER_EXPORT MTextureManager
{
public:
	//////////////////////////////////////////////////////////////////
	// Image paths
	MStatus addImagePath( const MString & path );
	MStatus imagePaths(MStringArray & paths) const;

	// Texture management
	MTexture* acquireTexture(const MString& filePath); // From disk
	MTexture* acquireTexture(const MString& textureName, const MPlug& plug, int width, int height); // From plug
	MTexture* acquireTexture(const MString& textureName, const MHWRender::MTextureDescription& textureDesc, const void* pixelData); // From memory

	static const char* className();

private:
	unsigned int fTextureTypeMap[kNumberOfTextureTypes];
	unsigned int fEnvMapTypeMap[kNumberOfEnvMapTypes];

	unsigned int* fRasterMap;

	MTextureManager(unsigned int* rasterMap);
	~MTextureManager();

	friend class MTexture;
	friend class MRenderer;
};

} // namespace MHWRender

#endif /* __cplusplus */
#endif /* _MTextureManager */
