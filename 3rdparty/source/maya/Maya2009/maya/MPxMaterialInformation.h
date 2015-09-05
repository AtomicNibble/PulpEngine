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
// CLASS:    MPxMaterialInformation
//
// ****************************************************************************

#if defined __cplusplus

// ****************************************************************************
// INCLUDED HEADER FILES

#ifndef _MPxMaterialInformation
#define _MPxMaterialInformation

#include <maya/MTypes.h>
#include <maya/MString.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MColor.h>

//! \ingroup OpenMaya MPx
//! \brief Phong shading parameters for an MPxMaterialInformation instance.
/*!
The MaterialInputData class is used to specify the phong shading parameters
to be used with an MPxMaterialInformation instance.
*/
class MaterialInputData
{
public:
	//! Shininess value
    float				shininess;
	//! Ambient color
	MColor				ambient;
	//! Diffuse color
    MColor              diffuse;
	//! Emission color
    MColor              emission;
	//! Specular color
    MColor              specular;

	//! Tells whether the material has transparency or not.
	bool				hasTransparency;
protected:
// No protected members
private:
// No private members
};

// ****************************************************************************
// CLASS DECLARATION (MPxMaterialInformation)

//! \ingroup OpenMaya MPx
//! \brief Material information proxy.
/*!
The MPxMaterialInformation class is a way for users to override the viewport
representation of shaders.  The viewport uses a simple phong model for display
in the viewport.  With this class users can provide their own values for the
phong shading parameters.
*/
class OPENMAYA_EXPORT MPxMaterialInformation
{
public:

	//! Material types. These affect how the material is shaded.
	enum MaterialType
	{
		kSimpleMaterial = 0,	//!< \nop
		kTexture,		//!< \nop
		kOverrideDraw		//!< \nop
	};

	MPxMaterialInformation(MObject & materialNode);

	virtual ~MPxMaterialInformation();

	bool	useMaterialAsTexture();

	// Virtual overrides

	virtual bool materialInfoIsDirty(const MPlug& plug) = 0 ;

	virtual bool connectAsTexture(const MPlug& plug) = 0 ;
	virtual bool textureDisconnected(const MPlug& plug) = 0 ;
	virtual bool computeMaterial(MaterialInputData& data)= 0 ;

protected:
	friend class MFnPlugin;
	//! Default constructor.
	MPxMaterialInformation();

	//! The shader node this material is based on
	MObject				fMaterialNode;

	//! Based on the material type the shader will be rendered
	//! differently in the Maya viewport
	MaterialType		fMaterialType;

	// fInstance Needs to be visible from static non-member methods in MFnPlugin
public:
	void*				fInstance;

private:
// No private members
};

#endif/* __cplusplus */
#endif/* _MPxMaterialInformation */
