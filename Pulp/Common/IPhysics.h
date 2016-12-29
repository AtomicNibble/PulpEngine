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


struct StridedData
{
	X_INLINE StridedData() : pData(nullptr), stride(0) {}

	const void* pData;
	uint32_t stride;
};

struct BoundedData : public StridedData
{
	X_INLINE BoundedData() : count(0) {}

	uint32_t count;
};

struct TriangleMeshDesc
{
	BoundedData points; // Vec3f
	BoundedData triangles; // 16bit int's
};

struct ConvexMeshDesc
{
	BoundedData points;
	// optional
	BoundedData polygons;
};

struct HeightFieldSample
{
	uint16_t height;
	uint8_t matIdx0;
	uint8_t matIdx1;
};

struct HeightFieldDesc
{
	uint32_t numRows;
	uint32_t numCols;
	StridedData	samples; // HeightFieldSample
};

struct IPhysicsCooking
{
	typedef core::Array<uint8_t> DataArr;

	virtual ~IPhysicsCooking() {}

	virtual bool cookingSupported(void) const X_ABSTRACT;

	virtual bool cookTriangleMesh(const TriangleMeshDesc& desc, DataArr& dataOut) X_ABSTRACT;
	virtual bool cookConvexMesh(const ConvexMeshDesc& desc, DataArr& dataOut) X_ABSTRACT;
	virtual bool cookHeightField(const HeightFieldDesc& desc, DataArr& dataOut) X_ABSTRACT;

};

struct IPhysLib : public IConverter
{
	virtual ~IPhysLib() {}

	virtual IPhysicsCooking* getCooking(void) X_ABSTRACT;
};


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

	// if you create a full physics instance you get cooking with it.
	// if you want just cooking use the converter interface.
	virtual IPhysicsCooking* getCooking(void) X_ABSTRACT;

	// we need to make a api for creating the physc objects for use in the 3dengine.
	virtual MaterialHandle createMaterial(MaterialDesc& desc) X_ABSTRACT;

	// you must add a region before adding actors that reside in the region.
	// best to just make all regions for level on load before adding any actors to scene.
	virtual RegionHandle addRegion(const AABB& bounds) X_ABSTRACT;


	virtual void addActorToScene(ActorHandle handle) X_ABSTRACT;
	virtual void addActorsToScene(ActorHandle* pHandles, size_t num) X_ABSTRACT;

	virtual ActorHandle createPlane(const QuatTransf& myTrans, float density) X_ABSTRACT;
	virtual ActorHandle createSphere(const QuatTransf& myTrans, float radius, float density) X_ABSTRACT;
	virtual ActorHandle createCapsule(const QuatTransf& myTrans, float radius, float halfHeight, float density) X_ABSTRACT;
	virtual ActorHandle createBox(const QuatTransf& myTrans, const AABB& bounds, float density) X_ABSTRACT;

	virtual ActorHandle createStaticPlane(const QuatTransf& myTrans) X_ABSTRACT;
	virtual ActorHandle createStaticSphere(const QuatTransf& myTrans, float radius) X_ABSTRACT;
	virtual ActorHandle createStaticCapsule(const QuatTransf& myTrans, float radius, float halfHeight) X_ABSTRACT;
	virtual ActorHandle createStaticBox(const QuatTransf& myTrans, const AABB& bounds) X_ABSTRACT;
	virtual ActorHandle createStaticTrigger(const QuatTransf& myTrans, const AABB& bounds) X_ABSTRACT;

};


X_NAMESPACE_END

#endif // !_X_PHYSICS_I_H_