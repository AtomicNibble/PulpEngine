#ifndef _MPxNode
#define _MPxNode
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
// CLASS:    MPxNode
//
// ****************************************************************************

#if defined __cplusplus

// ****************************************************************************
// INCLUDED HEADER FILES


#include <maya/MStatus.h>
#include <maya/MTypes.h>
#include <maya/MObject.h>

#include <maya/MTypeId.h>
#include <maya/MDataHandle.h>
#include <maya/MDataBlock.h>
#include <maya/MPlug.h>
#include <maya/MString.h>
#include <maya/MDGContext.h>

// ****************************************************************************
// DECLARATIONS

class	MPlugArray;
class 	MStringArray;

// ****************************************************************************
// CLASS DECLARATION (MPxNode)

//! \ingroup OpenMaya MPx
//! \brief Base class for user defined dependency nodes. 
/*!
  MPxNode is the the parent class for user defined dependency nodes.
  A dependency node is an object that resides in the dependency graph.
  It computes output attributes based on a set of input attributes.
  When an input changes, the compute method is called for each
  dependent output.

  The dependency graph is made up of nodes that have connections
  between their attributes. When an attribute changes, recomputation
  propagates through the graph until all affected values have been
  updated.

  When writing a dependency node, there is a very basic rule that
  should be observed. The outputs should be calculated only using the
  values of the inputs. All information about the world outside the
  node should come from input attributes. If this rule is not
  observed, then the results may be unpredictable.

  All dependency nodes have four basic attributes. Documentation for
  them may be found with the documentation for the dependency nodes in
  Maya. Only one attribute requires special attention by node
  developers. Developers must decide whether to support the
  <i>HasNoEffect</i> setting of the <i>state</i> attribute. {\em
  HasNoEffect} means that a node should pass through all data without
  performing computations on it. For example, a deformer node will
  pass geometry through unmodified when <i>state</i> is set to
  <i>HasNoEffect</i>. It is up the the plug-in writer to observe the
  <i>HasNoEffect mode</i> if it is relevant to the type of node.
*/
class OPENMAYA_EXPORT MPxNode
{
public:

	//! Defines the type of node
	enum Type {
		kDependNode,					//!< Custom node derived from MPxNode
		kLocatorNode,					//!< Custom locator derived from MPxLocatorNode
		kDeformerNode,					//!< Custom deformer derived from MPxDeformerNode
		kManipContainer,				//!< Custom container derived from MPxManipContainer
		kSurfaceShape,					//!< Custom shape derived from MPxSurfaceShape
		kFieldNode,						//!< Custom field derived from MPxFieldNode
		kEmitterNode,					//!< Custom emitter derived from MPxEmitterNode
		kSpringNode,					//!< Custom spring derived from MPxSpringNode
		kIkSolverNode,					//!< Custom IK solver derived from MPxIkSolverNode
		kHardwareShader,				//!< Custom shader derived from MPxHardwareShader
		kHwShaderNode,					//!< Custom shader derived from MPxHwShaderNode
		kTransformNode,					//!< Custom transform derived from MPxTransform
		kObjectSet,						//!< Custom set derived from MPxObjectSet
		kFluidEmitterNode,				//!< Custom fluid emitter derived from MpxFluidEmitterNode
		kImagePlaneNode,				//!< Custom image plane derived from MPxImagePlane
		kParticleAttributeMapperNode,	//!< Custom particle attribute mapper derived from MPxParticleAttributeMapperNode
		kCameraSetNode,					//!< Custom director derived from MPxCameraSet
		kConstraintNode,				//!< Custom constraint derived from MPxConstraint
		kManipulatorNode,				//!< Custom manipulator derived from MPxManipulatorNode
		kClientDeviceNode,				//!< Custom threaded device derived from MPxThreadedDeviceNode
		kThreadedDeviceNode,			//!< Custom threaded device node
		kLast							//!< Last value, used for counting
	};

	MPxNode();
	virtual ~MPxNode();
	virtual void			postConstructor();
	virtual MStatus			compute( const MPlug& plug,
									 MDataBlock& dataBlock );

	virtual bool			getInternalValueInContext( const MPlug& plug,
											  MDataHandle& dataHandle,
											  MDGContext& ctx);
    virtual bool			setInternalValueInContext( const MPlug& plug,
											  const MDataHandle& dataHandle,
											  MDGContext& ctx);
	// Obsolete
	virtual bool			getInternalValue( const MPlug& plug,
											  MDataHandle& dataHandle);
    virtual bool			setInternalValue( const MPlug& plug,
											  const MDataHandle& dataHandle);
    virtual int				internalArrayCount( const MPlug& plug,
											    const MDGContext& ctx) const;

    virtual void			copyInternalData( MPxNode* );

	virtual MStatus			legalConnection( const MPlug& plug,
											 const MPlug& otherPlug,
											 bool asSrc,
											 bool& isLegal ) const;
	virtual MStatus			legalDisconnection( const MPlug& plug,
											 const MPlug& otherPlug,
											 bool asSrc,
											 bool& isLegal ) const;
	virtual MStatus			setDependentsDirty( const MPlug& plug, MPlugArray& plugArray);

	virtual MStatus			connectionMade( const MPlug& plug,
											 const MPlug& otherPlug,
											 bool asSrc );
	virtual MStatus			connectionBroken( const MPlug& plug,
											 const MPlug& otherPlug,
											 bool asSrc );
	virtual bool			isPassiveOutput( const MPlug& plug ) const;
	virtual MStatus			shouldSave( const MPlug& plug, bool& isSaving );
	virtual MPlug			passThroughToOne( const MPlug& plug ) const;
	virtual bool			passThroughToMany( const MPlug& plug,
											   MPlugArray& plugArray ) const;

	MTypeId					typeId() const;
	MString					typeName() const;
	MString					name() const;
	virtual Type            type() const;
	virtual bool			isAbstractClass  () const;
	MObject                 thisMObject() const;

	static MStatus          addAttribute( const MObject & attr );
	static MStatus		    inheritAttributesFrom(
											const MString & parentClassName );
	static MStatus          attributeAffects( const MObject & whenChanges,
											  const MObject & isAffected );

	MStatus					setExistWithoutInConnections( bool flag );
	bool					existWithoutInConnections(
								MStatus* ReturnStatus = NULL ) const;
	MStatus					setExistWithoutOutConnections( bool flag );
	bool					existWithoutOutConnections(
								MStatus* ReturnStatus = NULL ) const;

	virtual MStringArray	getFilesToArchive( bool shortName = false,
											   bool unresolvedName = false,
											   bool markCouldBeImageSequence = false ) const;	
	// SCRIPT USE ONLY
	MDataBlock _forceCache( MDGContext& ctx=MDGContext::fsNormal )
		{ return forceCache(ctx); }
	void _setMPSafe ( bool flag )
		{ setMPSafe(flag); }
	void _setDoNotWrite( bool flag )
		{ setDoNotWrite(flag); }
	bool _doNotWrite()
		{ return doNotWrite(); }
	//

	//! message attribute
	static MObject          message;
	//! is historically interesting attribute
	static MObject          isHistoricallyInteresting;
	//! caching attribute
	static MObject          caching;
	//! state attribute
	static MObject          state;

	static const char*	    className();

protected:
	//!	USE _forceCache() IN SCRIPT
	MDataBlock				forceCache( MDGContext& ctx=MDGContext::fsNormal );

	//!	USE _setMPSafe() IN SCRIPT
	void					setMPSafe ( bool flag );

	MStatus					setDoNotWrite ( bool flag );
	bool					doNotWrite( MStatus *ReturnStatus = NULL ) const;

	void*					instance;

private:
	static void				initialSetup();
	static void*            initClass;
};

#endif /* __cplusplus */
#endif /* _MPxNode */
