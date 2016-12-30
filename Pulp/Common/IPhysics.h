#pragma once


#ifndef _X_PHYSICS_I_H_
#define _X_PHYSICS_I_H_

#include <IConverterModule.h>

X_NAMESPACE_BEGIN(physics)


typedef uintptr_t Handle;
typedef Handle MaterialHandle;
typedef Handle RegionHandle;
typedef Handle ActorHandle;
typedef Handle AggregateHandle;

static const Handle INVALID_HANLDE = 0;


struct MaterialDesc
{
	float32_t staticFriction;	// the coefficient of static friction
	float32_t dynamicFriction;	// the coefficient of dynamic friction
	float32_t restitutio;		// the coefficient of restitution
};

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

X_DECLARE_ENUM(CookingMode)(
	Fast,		// quickiest possible cooking time
	Slow,
	VerySlow	// all cooking optermistations enabled
);

X_DECLARE_ENUM(JointType)(
	Fixed,		// locks the orientations and origins rigidly together
	Distance,	// keeps the origins within a certain distance range
	Spherical,	// (also called a ball-and-socket) keeps the origins together, but allows the orientations to vary freely.
	Revolute,	// (also called a hinge) keeps the origins and x-axes of the frames together, and allows free rotation around this common axis.
	Prismatic	// (also called a slider) keeps the orientations identical, but allows the origin of each frame to slide freely along the common x-axis.
);

struct JointLimit
{
	JointLimit() :
		restitution(0),
		bounceThreshold(0),
		stiffness(0),
		damping(0),
		contactDistance(0)
	{
	}

	// Controls the amount of bounce when the joint hits a limit.

	// A restitution value of 1.0 causes the joint to bounce back with the velocity which it hit the limit.
	// A value of zero causes the joint to stop dead.
	
	// In situations where the joint has many locked DOFs(e.g. 5) the restitution may not be applied
	// correctly.This is due to a limitation in the solver which causes the restitution velocity to become zero
	// as the solver enforces constraints on the other DOFs.
	
	// This limitation applies to both angular and linear limits, however it is generally most apparent with limited
	// angular DOFs.Disabling joint projection and increasing the solver iteration count may improve this behavior
	// to some extent.
	
	// Also, combining soft joint limits with joint drives driving against those limits may affect stability.
	float32_t restitution;
	// determines the minimum impact velocity which will cause the joint to bounce
	float32_t bounceThreshold;
	// if greater than zero, the limit is soft, i.e. a spring pulls the joint back to the limit
	float32_t stiffness;
	// if spring is greater than zero, this is the damping of the limit spring
	float32_t damping;

	// the distance inside the limit value at which the limit will be considered to be active by the
	// solver.  As this value is made larger, the limit becomes active more quickly. It thus becomes less
	// likely to violate the extents of the limit, but more expensive.

	// The contact distance should be less than the limit angle or distance, and in the case of a pair limit,
	// less than half the distance between the upper and lower bounds. Exceeding this value will result in
	// the limit being active all the time.
	
	// Making this value too small can result in jitter around the limit.
	float32_t contactDistance;
};


struct JointLinearLimit : public JointLimit
{
	// the extent of the limit. 
	float32_t value;
};


struct JointLinearLimitPair : public JointLimit
{
	// the range of the limit.The upper limit must be no lower than the
	// lower limit, and if they are equal the limited degree of freedom will be treated as locked.

	float32_t upper;
	float32_t lower;
};

struct JointAngularLimitPair : public JointLimit
{
	// the range of the limit. The upper limit must be no lower than the lower limit.

	float32_t upper;
	float32_t lower;
};

struct JointLimitCone : public JointLimit
{
	// the maximum angle from the Y axis of the constraint frame.
	float32_t yAngle;
	// the maximum angle from the Z-axis of the constraint frame.
	float32_t zAngle;
};




struct IJoint
{
	enum class ActorIdx {
		Actor0,
		Actor1
	};

	virtual ~IJoint() {}

	virtual void setBreakForce(float32_t force, float32_t torque) X_ABSTRACT;
	virtual void getBreakForce(float32_t& force, float32_t& torque) const X_ABSTRACT;

	virtual void setLocalPose(ActorIdx actor, const QuatTransf& localPose) X_ABSTRACT;
	virtual QuatTransf getLocalPose(ActorIdx actor) const X_ABSTRACT;
};

struct IFixedJoint : public IJoint
{


};

struct IDistanceJoint : public IJoint
{
	// Return the current distance of the joint
	virtual float32_t getDistance(void) const X_ABSTRACT;

	virtual void setMinDistance(float32_t distance) X_ABSTRACT;
	virtual float32_t getMinDistance(void) const X_ABSTRACT;

	virtual void setMaxDistance(float32_t distance) X_ABSTRACT;
	virtual float32_t getMaxDistance(void) const X_ABSTRACT;

	virtual void setTolerance(float32_t tolerance) X_ABSTRACT;
	virtual float32_t getTolerance(void) const X_ABSTRACT;

	virtual void setStiffness(float32_t spring) X_ABSTRACT;
	virtual float32_t getStiffness(void) const X_ABSTRACT;

	virtual void setDamping(float32_t damping) X_ABSTRACT;
	virtual float32_t getDamping(void) const X_ABSTRACT;
};

struct ISphericalJoint : public IJoint
{
	virtual JointLimitCone getLimitCone(void) const X_ABSTRACT;
	virtual void setLimitCone(const JointLimitCone& limit) X_ABSTRACT;

	virtual bool limitEnabled(void) const X_ABSTRACT;
	virtual void setLimitEnabled(bool enable) X_ABSTRACT;
};

struct IRevoluteJoint : public IJoint
{
	virtual float32_t getAngle(void) const X_ABSTRACT;
	virtual float32_t getVelocity(void) const X_ABSTRACT;

	virtual void setLimit(const JointAngularLimitPair& limits) X_ABSTRACT;
	virtual JointAngularLimitPair getLimit(void) const X_ABSTRACT;
};


struct IPrismaticJoint : public IJoint
{
	virtual float32_t getPosition(void) X_ABSTRACT;
	virtual float32_t getVelocity(void) X_ABSTRACT;

	virtual void setLimit(const JointLinearLimitPair& limits) X_ABSTRACT;
	virtual JointLinearLimitPair getLimit(void) const X_ABSTRACT;
};

struct ControllerDesc
{
	enum class ShapeType {
		Box,
		Capsule
	};

	enum class NonWalkableMode {
		PreventClimbing,				// Stops character from climbing up non-walkable slopes, but doesn't move it otherwise
		PreventClimbingAndForceSliding	// Stops character from climbing up non-walkable slopes, and forces it to slide down those slopes
	};

protected:
	X_INLINE ControllerDesc(ShapeType st) 
	{
		shape = st;
		position = Vec3d::zero();
		upDirection = Vec3f::zAxis();
		slopeLimit = 0.707f;
		invisibleWallHeight = 0.0f;
		maxJumpHeight = 0.0f;
		contactOffset = 0.1f;
		stepOffset = 0.5f;
		density = 10.0f;
		scaleCoeff = 0.8f;
		volumeGrowth = 1.5f;
		nonWalkableMode = NonWalkableMode::PreventClimbing;
	}

public:
	ShapeType shape;
	Vec3d position;
	Vec3f upDirection;
	float32_t slopeLimit;
	float32_t invisibleWallHeight;
	float32_t maxJumpHeight;
	float32_t contactOffset;
	float32_t stepOffset;
	float32_t density;
	float32_t scaleCoeff;
	float32_t volumeGrowth;
	NonWalkableMode nonWalkableMode;
};

struct BoxControllerDesc : public ControllerDesc
{
	X_INLINE BoxControllerDesc() :
		ControllerDesc(ShapeType::Box)
	{
		halfHeight = 1.f;
		halfSideExtent = 0.5f;
		halfForwardExtent = 0.5f;
	}

	float32_t halfHeight;
	float32_t halfSideExtent;
	float32_t halfForwardExtent;

};

struct CapsuleControllerDesc : public ControllerDesc
{
	enum class ClimbingMode {
		Easy,
		Constrained
	};

	X_INLINE CapsuleControllerDesc() :
		ControllerDesc(ShapeType::Capsule)
	{
		radius = height = 0.0f;
		climbingMode = ClimbingMode::Easy;
	}

	float32_t radius;
	float32_t height;
	ClimbingMode climbingMode;
};

struct ICharacterController
{
	virtual ~ICharacterController() {}

	virtual ControllerDesc::ShapeType getType(void) const X_ABSTRACT;

	virtual	bool setPosition(const Vec3d& position) X_ABSTRACT;
	virtual Vec3d getPosition(void) const X_ABSTRACT;

	virtual	bool setFootPosition(const Vec3d& position) X_ABSTRACT;
	virtual	Vec3d getFootPosition(void) const X_ABSTRACT;

	virtual	void setStepOffset(const float32_t offset) X_ABSTRACT;
	virtual	float32_t getStepOffset(void) const X_ABSTRACT;

	virtual	void resize(float32_t height) X_ABSTRACT;
};

struct IBoxCharacterController : public ICharacterController
{
	virtual	float32_t getHalfHeight(void) const X_ABSTRACT;
	virtual	float32_t getHalfSideExtent(void) const X_ABSTRACT;
	virtual	float32_t getHalfForwardExtent(void) const X_ABSTRACT;

	virtual	bool setHalfHeight(float32_t halfHeight) X_ABSTRACT;
	virtual	bool setHalfSideExtent(float32_t halfSideExtent) X_ABSTRACT;
	virtual	bool setHalfForwardExtent(float32_t halfForwardExtent) X_ABSTRACT;
};

struct ICapsuleCharacterController : public ICharacterController
{
	virtual	float32_t getRadius(void) const X_ABSTRACT;
	virtual	float32_t getHeight(void) const X_ABSTRACT;
	virtual	CapsuleControllerDesc::ClimbingMode getClimbingMode(void) const X_ABSTRACT;

	virtual	bool setRadius(float32_t radius) X_ABSTRACT;
	virtual	bool setHeight(float32_t height) X_ABSTRACT;
	virtual	bool setClimbingMode(CapsuleControllerDesc::ClimbingMode mode) X_ABSTRACT;
};



struct IPhysicsCooking
{
	typedef core::Array<uint8_t> DataArr;

	virtual ~IPhysicsCooking() {}

	virtual bool cookingSupported(void) const X_ABSTRACT;
	virtual bool setCookingMode(CookingMode::Enum mode) X_ABSTRACT;

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
	typedef core::Array<uint8_t> DataArr;

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
	// removes the region, anything that stil resides in this regions bounds and another region don't overlap
	// will be reported as out of bounds.
	virtual bool removeRegion(RegionHandle handles) X_ABSTRACT;

	// An aggregate is a collection of actors.
	// which in turn allows optimized spatial data operations.
	// A typical use case is a ragdoll, made of N different body parts, with each part a actor.
	// Without aggregates, this gives rise to N broad - phase entries for the ragdoll.
	virtual AggregateHandle createAggregate(uint32_t maxActors, bool selfCollisions) X_ABSTRACT;
	virtual bool addActorToAggregate(AggregateHandle handle, ActorHandle actor) X_ABSTRACT;
	virtual bool releaseAggregate(AggregateHandle handle) X_ABSTRACT;

	// joints. I may want to return a interface for this not sure yet.
	// localFrame's are `position and orientation of the joint relative to actor`
	virtual IJoint* createJoint(JointType::Enum type, ActorHandle actor0, ActorHandle actor1, 
		const QuatTransf& localFrame0, const QuatTransf& localFrame1) X_ABSTRACT;
	virtual void releaseJoint(IJoint* pJoint) X_ABSTRACT;

	// Characters controllers
	virtual ICharacterController* createCharacterController(const ControllerDesc& desc) X_ABSTRACT;
	virtual void releaseCharacterController(ICharacterController* pController) X_ABSTRACT;


	virtual void addActorToScene(ActorHandle handle) X_ABSTRACT;
	virtual void addActorsToScene(ActorHandle* pHandles, size_t num) X_ABSTRACT;

	virtual ActorHandle createConvexMesh(const QuatTransf& myTrans, const DataArr& cooked, float density, const Vec3f& scale = Vec3f::one()) X_ABSTRACT;
	virtual ActorHandle createTriangleMesh(const QuatTransf& myTrans, const DataArr& cooked, float density, const Vec3f& scale = Vec3f::one()) X_ABSTRACT;
	virtual ActorHandle createHieghtField(const QuatTransf& myTrans, const DataArr& cooked, float density, const Vec3f& heightRowColScale = Vec3f::one()) X_ABSTRACT;
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