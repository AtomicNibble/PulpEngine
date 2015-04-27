#ifndef _MShaderManager
#define _MShaderManager
//-
// ==========================================================================
// Copyright (C) 2010 Autodesk, Inc., and/or its licensors.  All
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
// ****************************************************************************

#if defined __cplusplus

// ****************************************************************************
// INCLUDED HEADER FILES

#include <maya/MStatus.h>
#include <maya/MStringArray.h>
#include <maya/MColor.h>
#include <maya/MMatrix.h>
#include <maya/MTextureManager.h>
#include <maya/MRenderTargetManager.h>

// ****************************************************************************
// NAMESPACE

namespace MHWRender
{

class MDrawContext;
class MRenderItem;
class MSamplerState;

// ****************************************************************************
// DECLARATIONS

// ****************************************************************************
// CLASS DECLARATION (MTextureAssignment)
//! \ingroup OpenMayaRender
//! \brief Structure to hold the information required to set a texture parameter on a shader
//! using a texture as input.
/*!
This structure holds the information required to set a texture parameter on a shader
using a texture as input.
*/
struct MTextureAssignment
{
	MHWRender::MTexture *texture;
};

// ****************************************************************************
// CLASS DECLARATION (MRenderTargetAssignment)
//! \ingroup OpenMayaRender
//! \brief Structure to hold the information required to set a texture parameter on a shader
//! using a render target as input.
/*!
This structure holds the information required to set a texture parameter on a shader
using a render target as input
*/
struct MRenderTargetAssignment
{
	MHWRender::MRenderTarget *target;
};

// ****************************************************************************
// CLASS DECLARATION (MShaderCompileMacro)
//! \ingroup OpenMayaRender
//! \brief Structure to define a shader compiler macro.
/*!
This structure holds the information required to define
a macro to be used when acquiring an shader instance
from an effects file.
*/
struct MShaderCompileMacro
{
	MString mName;			//!< Name of the macro
	MString mDefinition;	//!< Macro definition
};


// ****************************************************************************
// CLASS DECLARATION (MShaderInstance)
//! \ingroup OpenMayaRender
//! \brief An instance of a shader that may be used with Viewport 2.0
/*!
This class represents a shader that may be used with the MRenderItem class
for rendering in Viewport 2.0.
*/
class OPENMAYARENDER_EXPORT MShaderInstance
{
public:
	//! Pre-draw callback function pointer
	typedef void (*DrawCallback)(MDrawContext& context, const MRenderItem* renderItem);

	//! Specifies parameter types
	enum ParameterType {
		//! Invalid element type (default value)
		kInvalid,
		//! Boolean
		kBoolean,
		//! Signed 32-bit integer
		kInteger,
		//! IEEE single precision floating point
		kFloat,
		//! IEEE single precision floating point (x2)
		kFloat2,
		//! IEEE single precision floating point (x3)
		kFloat3,
		//! IEEE single precision floating point (x4)
		kFloat4,
		//! IEEE single precision floating point row-major matrix (4x4)
		kFloat4x4Row,
		//! IEEE single precision floating point column-major matrix (4x4)
		kFloat4x4Col,
		//! 1D texture
		kTexture1,
		//! 2D texture
		kTexture2,
		//! 3D texture
		kTexture3,
		//! Cube texture
		kTextureCube,
		//! Sampler
		kSampler
	};

	~MShaderInstance();

	DrawCallback preDrawCallback() const;
	DrawCallback postDrawCallback() const;

	void parameterList(MStringArray& list) const;
	ParameterType parameterType(const MString& parameterName) const;
	bool isArrayParameter(const MString& parameterName) const;

	MStatus setParameter(const MString& parameterName, bool value);
	MStatus setParameter(const MString& parameterName, int value);
	MStatus setParameter(const MString& parameterName, float value);
	MStatus setParameter(const MString& parameterName, const float* value);
	MStatus setParameter(const MString& parameterName, const MMatrix& value);
	MStatus setParameter(const MString& parameterName, MTextureAssignment& textureAssignment);
	MStatus setParameter(const MString& parameterName, MRenderTargetAssignment& targetAssignment);
	MStatus setParameter(const MString& parameterName, const MSamplerState & sampler);

	MStatus setArrayParameter(
		const MString& parameterName,
		const bool* values,
		unsigned int count);
	MStatus setArrayParameter(
		const MString& parameterName,
		const int* values,
		unsigned int count);
	MStatus setArrayParameter(
		const MString& parameterName,
		const float* values,
		unsigned int count);
	MStatus setArrayParameter(
		const MString& parameterName,
		const MMatrix* values,
		unsigned int count);

	static const char* className();

private:
	MShaderInstance(void* data, DrawCallback preCb, DrawCallback postCb);

	void* fData;
	DrawCallback fPreDrawCallback;
	DrawCallback fPostDrawCallback;

	friend class MShaderManager;
	friend class MRenderItem;
};


// ****************************************************************************
// CLASS DECLARATION (MShaderManager)
//! \ingroup OpenMayaRender
//! \brief Provides access to MShaderInstance objects for use in Viewport 2.0.
/*!
This class generates MShaderInstance objects for use with user-created
MRenderItem objects. Any MShaderInstance objects created by this class are
owned by the caller.

The methods for adding shader and shader include paths will allow
for additional search paths to be used with the "getEffectsFileShader()"
method which searches for shader files on disk. The default
search path is in the "Cg" and "HLSL" folders found in the runtime
directory. It is recommended that user defined shaders not reside
at this location to avoid any potential shader conflicts.

When acquiring a shader instance the caller may also specify pre-draw and
post-draw callback functions. These functions will be invoked any time a render
item that uses the shader instance is about to be drawn. These callbacks are
passed an MDrawContext object as well as the render item being drawn and may
alter the draw state in order to get different rendering effects. Accessing
Maya nodes from within these callbacks is an error and may result in
instability.
*/
class OPENMAYARENDER_EXPORT MShaderManager
{
public:
	MStatus addShaderPath(const MString & path) const;
	MStatus shaderPaths(MStringArray & paths) const;
	MStatus addShaderIncludePath(const MString & path) const;
	MStatus shaderIncludePaths(MStringArray & paths) const;

	MShaderInstance* getEffectsFileShader(
		const MString& effectsFileName,
		const MString& techniqueName,
		MShaderCompileMacro* macros = 0,
		unsigned int numberOfMacros = 0,
		bool useEffectCache = true,
		MShaderInstance::DrawCallback preCb = 0,
		MShaderInstance::DrawCallback postCb = 0) const;


	//! Name of an available "stock" shader
	enum MStockShader
	{
		k3dSolidShader,	//!< A solid color shader for 3d rendering
		k3dBlinnShader	//!< A stock Blinn shader for 3d rendering
	};

	MShaderInstance* getStockShader(
		MStockShader name,
		MShaderInstance::DrawCallback preCb = 0,
		MShaderInstance::DrawCallback postCb = 0) const;


	static const char* className();

private:
	MShaderManager();
	~MShaderManager();

	friend class MRenderer;
};

} // namespace MHWRender

#endif /* __cplusplus */
#endif /* _MShaderManager */
