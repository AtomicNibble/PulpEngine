#ifndef _MFnTurbulenceField
#define _MFnTurbulenceField
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
// CLASS:    MFnTurbulenceField
//
// ****************************************************************************

#if defined __cplusplus

// ****************************************************************************
// INCLUDED HEADER FILES


#include <maya/MFnDagNode.h>
#include <maya/MFnField.h>

// ****************************************************************************
// CLASS DECLARATION (MFnTurbulenceField)

//! \ingroup OpenMayaFX MFn
//! \brief Function set for Turbulence Fields
/*!
Function set for creation, edit, and query of Turbulence Fields.

A turbulence field causes irregularities in the motion of affected objects.
These irregularities are also called noise or jitter.
*/
class OPENMAYAFX_EXPORT MFnTurbulenceField : public MFnField
{
    declareDagMFn(MFnTurbulenceField, MFnField);

public:
    double       frequency          ( MStatus *ReturnStatus = NULL ) const;
    MStatus      setFrequency       ( double value );
    double       phase              ( MStatus *ReturnStatus = NULL ) const;
    MStatus      setPhase           ( double value );

BEGIN_NO_SCRIPT_SUPPORT:

 	declareDagMFnConstConstructor(MFnTurbulenceField, MFnField );

END_NO_SCRIPT_SUPPORT:

protected:
// No protected members
private:
// No private members
};

#endif /* __cplusplus */
#endif /* _MFnTurbulenceField */
