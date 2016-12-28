#pragma once


#ifndef _X_PHYSICS_I_H_
#define _X_PHYSICS_I_H_

#include <IConverterModule.h>

X_NAMESPACE_BEGIN(physics)


struct MaterialDesc
{
	float32_t staticFriction;	// the coefficient of static friction
	float32_t dynamicFriction;	// the coefficient of dynamic friction
	float32_t restitutio;		// the coefficient of restitution
};


typedef uintptr_t Handle;
typedef Handle MaterialHandle;
typedef Handle RegionHandle;
typedef Handle ActorHandle;

static const Handle INVALID_HANLDE = 0;

struct IPhysics
{
	virtual ~IPhysics() {}

	virtual void registerVars(void) X_ABSTRACT;
	virtual void registerCmds(void) X_ABSTRACT;

	virtual bool init(void) X_ABSTRACT;
	virtual bool initRenderResources(void) X_ABSTRACT; // allocates a Aux render
	virtual void shutDown(void) X_ABSTRACT;
	virtual void release(void) X_ABSTRACT;

	virtual void onTickPreRender(float dtime) X_ABSTRACT;
	virtual void onTickPostRender(float dtime) X_ABSTRACT;
	virtual void render(void) X_ABSTRACT; // render stuff like debug shapes.


	// we need to make a api for creating the physc objects for use in the 3dengine.
	virtual MaterialHandle createMaterial(MaterialDesc& desc) X_ABSTRACT;

	// you must add a region before adding actors that reside in the region.
	// best to just make all regions for level on load before adding any actors to scene.
	virtual RegionHandle addRegion(const AABB& bounds) X_ABSTRACT;

	virtual ActorHandle createStaticTrigger(const QuatTransf& myTrans, const AABB& bounds) X_ABSTRACT;


};


X_NAMESPACE_END

#endif // !_X_PHYSICS_I_H_