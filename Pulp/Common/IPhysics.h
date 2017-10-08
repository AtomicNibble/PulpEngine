#pragma once


#ifndef _X_PHYSICS_I_H_
#define _X_PHYSICS_I_H_

#include <IConverterModule.h>

X_NAMESPACE_BEGIN(physics)

static const uint32_t MAX_SCENES = 4; // max scenes you can create (artifical limit)
static const uint32_t MAX_ACTIVE_SCENES = 1; // max scenes been simulated (to increase some stepper logic needs refactoring)

static const float SCALE_LENGTH = 1;
static const float SCALE_MASS = 1000;
static const float SCALE_SPEED = 10;


X_DECLARE_FLAGS(GroupFlag)(
	Default,	// nothing can pass through this
	Player,		// can walk through AiClip and VehicleClip
	Ai,			// can walk through PlayerClip and VehicleClip
	Vehicle,	// can walk through PlayerClip and VehicleClip


	PlayerClip,	// a actor group that stops actors in Player group
	AiClip,		// a actor group that stops actors in Ai group
	VehicleClip	// a actor group that stops actors in Vehicle group
);

typedef Flags<GroupFlag> GroupFlags;

X_DECLARE_FLAG_OPERATORS(GroupFlags);

typedef uintptr_t Handle;
typedef Handle MaterialHandle;
typedef Handle RegionHandle;
typedef Handle ActorHandle;
typedef Handle AggregateHandle;
typedef Handle TriMeshHandle;
typedef Handle ConvexMeshHandle;
typedef Handle HieghtFieldHandle;
typedef Handle LockHandle;


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
	BoundedData triangles; // it's either 16 or 32bit zero based index's depending on CookFlag. the stride is the face not index size. aka 6 for 16bit
};

struct ConvexMeshDesc
{
	BoundedData points;  // vec3f, can use stride to jump to next if it's inside a struct.
	BoundedData polygons; // 
	BoundedData indices; // 
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

	virtual void setLocalPose(ActorIdx actor, const Transformf& localPose) X_ABSTRACT;
	virtual Transformf getLocalPose(ActorIdx actor) const X_ABSTRACT;
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
		material = INVALID_HANLDE;
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
	MaterialHandle material;
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
		Easy,			// Standard mode, let the capsule climb over surfaces according to impact normal
		Constrained		// Constrained mode, try to limit climbing according to the step offset
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

X_DECLARE_FLAGS8(CharacterColFlag)(
	SIDES,
	UP,
	DOWN
);

typedef Flags8<CharacterColFlag> CharacterColFlags;

struct ControllerState
{
	Vec3f				deltaXP;			//!< delta position vector for the object the CCT is standing/riding on. Not always match the CCT delta when variable timesteps are used.
	ActorHandle			touchedActor;		//!< Actor owning 'touchedShape'
	CharacterColFlags	collisionFlags;		//!< Last known collision flags 
	bool				standOnAnotherCCT;	//!< Are we standing on another CCT?
	bool				standOnObstacle;	//!< Are we standing on a user-defined obstacle?
	bool				isMovingUp;			//!< is CCT moving up or not? (i.e. explicit jumping)
};

struct ControllerStats
{
	uint16_t nbIterations;
	uint16_t nbFullUpdates;
	uint16_t nbPartialUpdates;
	uint16_t nbTessellation;
};


struct ICharacterController
{
	typedef CharacterColFlag ColFlag;
	typedef CharacterColFlags ColFlags;

	virtual ~ICharacterController() {}

	virtual ControllerDesc::ShapeType getType(void) const X_ABSTRACT;

	virtual ColFlags move(const Vec3f& disp, float32_t minDist, float32_t elapsedTime) X_ABSTRACT;

	virtual	bool setPosition(const Vec3d& position) X_ABSTRACT;
	virtual Vec3d getPosition(void) const X_ABSTRACT;

	virtual	bool setFootPosition(const Vec3d& position) X_ABSTRACT;
	virtual	Vec3d getFootPosition(void) const X_ABSTRACT;

	virtual	void setStepOffset(const float32_t offset) X_ABSTRACT;
	virtual	float32_t getStepOffset(void) const X_ABSTRACT;

	virtual	void resize(float32_t height) X_ABSTRACT;

	virtual void getState(ControllerState& state) X_ABSTRACT;
	virtual void getStats(ControllerStats& stats) X_ABSTRACT;

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
public:
	X_DECLARE_FLAGS(CookFlag)(
		INDICES_16BIT,
		COMPUTE_CONVEX
	);

	typedef Flags<CookFlag> CookFlags;
	typedef core::Array<uint8_t> DataArr;

	virtual ~IPhysicsCooking() {}

	virtual bool cookingSupported(void) const X_ABSTRACT;
	virtual bool setCookingMode(CookingMode::Enum mode) X_ABSTRACT;

	virtual bool cookTriangleMesh(const TriangleMeshDesc& desc, DataArr& dataOut, CookFlags flags = CookFlags()) X_ABSTRACT;
	// cook convex from a tri description, the tri mesh must still be convex.
	virtual bool cookConvexMesh(const TriangleMeshDesc& desc, DataArr& dataOut, CookFlags flags = CookFlags()) X_ABSTRACT;
	virtual bool cookConvexMesh(const ConvexMeshDesc& desc, DataArr& dataOut, CookFlags flags = CookFlags()) X_ABSTRACT;
	virtual bool cookHeightField(const HeightFieldDesc& desc, DataArr& dataOut) X_ABSTRACT;

};

struct IPhysLib : public IConverter
{
	virtual ~IPhysLib() {}

	virtual IPhysicsCooking* getCooking(void) X_ABSTRACT;
};


// 0 = no limit
// these values are not hard limits
// more of a 'what to expect'
struct SceneLimits
{
	uint32_t maxActors;
	uint32_t maxBodies;
	uint32_t maxStaticShapes;
	uint32_t maxDynamicShapes;
	uint32_t maxAggregates;
	uint32_t maxConstraints;
	uint32_t maxRegions; // has hard limit of 256
	uint32_t maxObjectsPerRegion;
};

struct ToleranceScale
{
	// The approximate size of objects in the simulation.
	// 
	// For simulating roughly human - sized in metric units, 1 is a good choice.
	// If simulation is done in centimetres, use 100 instead.This is used to
	// estimate certain length - related tolerances.
	float32_t length;

	// The approximate mass of a length * length * length block.
	// If using metric scale for character sized objects and measuring mass in
	// kilogrammes, 1000 is a good choice.
	float32_t mass;

	// The typical magnitude of velocities of objects in simulation. This is used to estimate
	// whether a contact should be treated as bouncing or resting based on its impact velocity,
	// and a kinetic energy threshold below which the simulation may put objects to sleep.
	// 
	// For normal physical environments, a good choice is the approximate speed of an object falling
	// under gravity for one second.
	float32_t speed;
};

enum class FrictionType {
	Patch,
	OneDirectional,
	TwoDirectional
};

struct SceneDesc
{
	SceneLimits sceneLimitHint;

	Vec3f gravity;						// constant gravity for the entire scene.
	FrictionType frictionType;			// the type of friction :D !
	float32_t frictionOffsetThreshold;	// (multiplied by scale.length) A threshold of contact separation distance used to decide if a contact point will experience friction forces.
	float32_t contractCorrelationDis;	// (multiplied by scale.length) The patch friction model uses this coefficient to determine if a friction anchor can persist between frames.
	float32_t bounceThresholdVelocity;	// (multiplied by scale.speed) Collision speeds below this threshold will not cause a bounce.
	AABB sanityBounds;					// nothing sound ever be outside these bounds, it's reported if so.
};

// ------------------------------------------------

X_DECLARE_FLAGS(QueryFlag)(
	STATIC,
	DYNAMIC,
	PREFILTER,
	POSTFILTER,
	ANY_HIT,
	NO_BLOCK
);

typedef Flags<QueryFlag> QueryFlags;

X_DECLARE_FLAG_OPERATORS(QueryFlags);

X_DECLARE_FLAGS(HitFlag)(
	POSITION,						// "position" member of QueryHit is valid
	NORMAL,							// "normal" member of QueryHit is valid
	DISTANCE,						// "distance" member of QueryHit is valid
	UV,								// "u" and "v" barycentric coordinates of QueryHit are valid. Not applicable to sweep queries.
	ASSUME_NO_INITIAL_OVERLAP,		// Performance hint flag for sweeps when it is known upfront there's no initial overlap.
									// NOTE: using this flag may cause undefined results if shapes are initially overlapping.
	MESH_MULTIPLE,					// Report all hits for meshes rather than just the first. Not applicable to sweep queries.
	MESH_ANY,						// Report any first hit for meshes. If neither MESH_MULTIPLE nor MESH_ANY is specified,
									// a single closest hit will be reported for meshes.
	MESH_BOTH_SIDES,				// Report hits with back faces of mesh triangles. Also report hits for raycast
									// originating on mesh surface and facing away from the surface normal. Not applicable to sweep queries.
									// Please refer to the user guide for heightfield-specific differences.
	PRECISE_SWEEP,					// Use more accurate but slower narrow phase sweep tests.
	MTD								// Report the minimum translation depth, normal and contact point. 
);

typedef Flags<HitFlag> HitFlags;


X_DECLARE_FLAG_OPERATORS(HitFlags);

struct ActorShape
{
	X_INLINE ActorShape() : actor(INVALID_HANLDE), pShape(nullptr) {}
	X_INLINE ActorShape(ActorHandle a, void* s) : actor(a), pShape(s) {}

	ActorHandle		actor;
	void*			pShape;
};

struct QueryHit : ActorShape
{
	X_INLINE QueryHit() : faceIndex(0xFFFFffff) {}

	uint32_t faceIndex;
};

struct LocationHit : public QueryHit
{
	X_INLINE LocationHit() : 
		flags(0), 
		position(Vec3f::zero()), 
		normal(Vec3f::zero()),
		distance(std::numeric_limits<float32_t>::max()) 
	{}

	X_INLINE bool hadInitialOverlap(void) const { 
		return (distance <= 0.0f); 
	}

	// the following fields are set in accordance with the HitFlags
	HitFlags flags;		
	Vec3f position;								
	Vec3f normal;		
	float32_t distance;
};


struct RaycastHit : public LocationHit
{
	X_INLINE RaycastHit() : u(0.0f), v(0.0f) {}

	// the following fields are set in accordance with the HitFlags
	float32_t u, v;

#if X_64 == 0
	uint32_t padTo16Bytes[3];
#endif // !X_64
};

struct OverlapHit : public QueryHit 
{ 
	uint32_t padTo16Bytes;
};

struct SweepHit : public LocationHit
{
	X_INLINE SweepHit() {}

	uint32_t padTo16Bytes;
};

template<typename HitType>
struct HitCallback
{
	/*

	param[in] aTouches			Optional buffer for recording PxQueryHitType::eTOUCH type hits.
	param[in] aMaxNbTouches		Size of touch buffer.

	note	if aTouches is nullptr and aMaxNbTouches is 0, only the closest blocking hit will be recorded by the query.
	note	If ANY_HIT flag is used as a query parameter, hasBlock will be set to true and blockingHit will be used to receive the result.
	note	Both TOUCH and BLOCK hits will be registered as hasBlock = true and stored in HitCallback.block when ANY_HIT flag is used.

	*/
	HitCallback(HitType* aTouches, uint32_t aMaxNbTouches) :
		hasBlock(false),
		pTouches(aTouches),
		maxNbTouches(aMaxNbTouches), 
		nbTouches(0)
	{
	
	}

	virtual ~HitCallback() {}

	virtual bool processTouches(const HitType* buffer, uint32_t nbHits) X_ABSTRACT;
	virtual void finalizeQuery() {} 

	X_INLINE bool hasAnyHits(void) { 
		return (hasBlock || (nbTouches > 0));
	}


	HitType		block;			
	bool		hasBlock;		

	HitType*	pTouches;		
	uint32_t	maxNbTouches;
	uint32_t	nbTouches;
};

template<typename HitType>
struct HitBuffer : public HitCallback<HitType>
{
	HitBuffer(HitType* pTouches = nullptr, uint32_t maxNbTouches = 0) : 
		HitCallback<HitType>(pTouches, maxNbTouches)
	{}
	
	virtual ~HitBuffer() {}

	X_INLINE uint32_t getNbAnyHits(void) const { return getNbTouches() + uint32_t(this->hasBlock); }
	X_INLINE const HitType&	getAnyHit(const uint32_t index) const {
		X_ASSERT(index < getNbTouches() + uint32_t(this->hasBlock), "")();
		return index < getNbTouches() ? getTouches()[index] : this->block;
	}

	X_INLINE uint32_t getNbTouches(void) const { return this->nbTouches; }
	X_INLINE const HitType*	getTouches(void) const { return this->touches; }
	X_INLINE const HitType&	getTouch(const uint32_t index) const {
		X_ASSERT(index < getNbTouches(), "")(); 
		return getTouches()[index]; 
	}
	X_INLINE uint32_t getMaxNbTouches(void) const { return this->maxNbTouches; }

protected:
	// stops after the first callback
	virtual bool processTouches(const HitType* buffer, uint32_t nbHits) { 
		X_UNUSED(buffer, nbHits); 
		return false;
	}
};

typedef HitCallback<RaycastHit> RaycastCallback;
typedef HitCallback<OverlapHit> OverlapCallback;
typedef HitCallback<SweepHit> SweepCallback;
typedef HitBuffer<RaycastHit> RaycastBuffer;
typedef HitBuffer<OverlapHit> OverlapBuffer;
typedef HitBuffer<SweepHit> SweepBuffer;


// ------------------------------------------------

// For passing geo to physics in abstract way.
enum class GeometryType
{
	Sphere,
	Plane,
	Capsule,
	Box
};

struct GeometryBase
{
protected:
	X_INLINE GeometryBase(GeometryType t) : type_(t) {}

protected:
	GeometryType type_;
};

class SphereGeometry : public GeometryBase
{
public:
	X_INLINE SphereGeometry() : GeometryBase(GeometryType::Sphere), radius_(0) {}
	X_INLINE SphereGeometry(float32_t ir) : GeometryBase(GeometryType::Sphere), radius_(ir) {}

private:
	float32_t radius_;
};

class PlaneGeometry : public GeometryBase
{
public:
	X_INLINE PlaneGeometry() : GeometryBase(GeometryType::Plane) {}
};

class CapsuleGeometry : public GeometryBase
{
public:
	X_INLINE CapsuleGeometry() : GeometryBase(GeometryType::Capsule), radius_(0), halfHeight_(0) {}
	X_INLINE CapsuleGeometry(float32_t radius, float32_t halfHeight): GeometryBase(GeometryType::Capsule), radius_(radius), halfHeight_(halfHeight) {}

private:
	float32_t radius_;
	float32_t halfHeight_;
};

class BoxGeometry : public GeometryBase
{
public:
	X_INLINE BoxGeometry() : GeometryBase(GeometryType::Sphere) {}
	X_INLINE BoxGeometry(float hx, float hy, float hz) : GeometryBase(GeometryType::Sphere), halfExtents_(hx, hy, hz) {}
	X_INLINE BoxGeometry(const Vec3f& halfExtents) : GeometryBase(GeometryType::Sphere), halfExtents_(halfExtents) {}

private:
	Vec3f halfExtents_;
};

// ------------------------------------------------

struct ActiveTransform
{
	ActorHandle		actor;				
	void*			userData;			
	Transformf		actor2World;		
};

// ------------------------------------------------


template<typename HitType>
struct BatchQueryResult
{
	HitType			block;			//!< Holds the closest blocking hit for a single query in a batch. Only valid if hasBlock is true.
	HitType*		pTouches;		//!< This pointer will either be set to NULL for 0 nbTouches or will point
									//!< into the user provided batch query results buffer specified in PxBatchQueryDesc.
	uint32_t		nbTouches;		//!< Number of touching hits returned by this query, works in tandem with touches pointer.
	void*			pUserData;		//!< Copy of the userData pointer specified in the corresponding query.
	uint8_t			queryStatus;	//!< Takes on values from PxBatchQueryStatus::Enum.
	bool			hasBlock;		//!< True if there was a blocking hit.
	uint16_t		pad;			//!< pads the struct to 16 bytes.

	X_INLINE uint32_t getNbAnyHits(void) const {
		return nbTouches + (hasBlock ? 1 : 0);
	}

	X_INLINE const HitType& getAnyHit(const uint32_t index) const {
		X_ASSERT(index < nbTouches + (hasBlock ? 1 : 0), "")();
		return index < nbTouches ? touches[index] : block;
	}
};


typedef BatchQueryResult<RaycastHit>	RaycastQueryResult;
typedef BatchQueryResult<SweepHit>		SweepQueryResult;
typedef BatchQueryResult<OverlapHit>	OverlapQueryResult;


// all these must be 16 byte aligned.
struct QueryMemory
{
	RaycastQueryResult*			pUserRaycastResultBuffer;
	RaycastHit*					pUserRaycastTouchBuffer;
	SweepQueryResult*			pUserSweepResultBuffer;
	SweepHit*					pUserSweepTouchBuffer;
	OverlapQueryResult*			pUserOverlapResultBuffer;
	OverlapHit*					pUserOverlapTouchBuffer;

	size_t raycastTouchBufferSize;	// Capacity in elements
	size_t sweepTouchBufferSize;	// Capacity in elements
	size_t overlapTouchBufferSize;	// Capacity in elements
};


struct IBatchedQuery
{
	virtual ~IBatchedQuery() {}

	virtual	void execute(void) X_ABSTRACT;
	virtual	void release(void) X_ABSTRACT;

	virtual void raycast(const Vec3f& origin, const Vec3f& unitDir, const float32_t distance,
		int16_t maxtouchHits, HitFlags hitFlags = HitFlag::POSITION | HitFlag::NORMAL | HitFlag::DISTANCE,
		QueryFlags queryFlags = QueryFlag::STATIC | QueryFlag::DYNAMIC) const X_ABSTRACT;

	virtual void sweep(const GeometryBase& geometry, const Transformf& pose, const Vec3f& unitDir, const float32_t distance,
		int16_t maxTouchHits, HitFlags hitFlags = HitFlag::POSITION | HitFlag::NORMAL | HitFlag::DISTANCE,
		QueryFlags queryFlags = QueryFlag::STATIC | QueryFlag::DYNAMIC,
		const float32_t inflation = 0.f) const X_ABSTRACT;

	virtual void overlap(const GeometryBase& geometry, const Transformf& pose, int16_t maxTouchHits) const X_ABSTRACT;

};

struct TriggerPair
{
	X_INLINE TriggerPair(ActorHandle trigActor, ActorHandle othActor) :
		triggerActor(trigActor),
		otherActor(othActor)
	{}

	ActorHandle triggerActor;	//!< The actor to which triggerShape is attached
	ActorHandle otherActor;		//!< The actor to which otherShape is attached
};


// ------------------------------------------------

static const HitFlags DEFAULT_HIT_FLAGS = HitFlag::POSITION | HitFlag::NORMAL | HitFlag::DISTANCE;
static const QueryFlags DEFAULT_QUERY_FLAGS = QueryFlag::STATIC | QueryFlags::DYNAMIC;

struct IScene
{
	virtual ~IScene() {}


	// some runtime tweaks.
	virtual void setGravity(const Vec3f& gravity) X_ABSTRACT;
	virtual void setBounceThresholdVelocity(float32_t bounceThresholdVelocity) X_ABSTRACT;
	// ~

	// locking
	virtual LockHandle lock(bool write = false) X_ABSTRACT;
	virtual void unLock(LockHandle lock) X_ABSTRACT;

	// you must add a region before adding actors that reside in the region.
	// best to just make all regions for level on load before adding any actors to scene.
	// regions can overlap but none overlapping is best for performance.
	virtual RegionHandle addRegion(const AABB& bounds) X_ABSTRACT;
	// removes the region, anything that stil resides in this regions bounds and another region don't overlap
	// will be reported as out of bounds.
	virtual bool removeRegion(RegionHandle handles) X_ABSTRACT;

	virtual void addActorToScene(ActorHandle handle) X_ABSTRACT;
	virtual void addActorToScene(ActorHandle handle, const char* pDebugNamePointer) X_ABSTRACT;
	virtual void addActorsToScene(ActorHandle* pHandles, size_t num) X_ABSTRACT;
	virtual void removeActor(ActorHandle handle) X_ABSTRACT;
	virtual void removeActors(ActorHandle* pHandles, size_t num) X_ABSTRACT;

	// Aggregate
	virtual void addAggregate(AggregateHandle handle) X_ABSTRACT;
	virtual void removeAggregate(AggregateHandle handle) X_ABSTRACT;

	// Characters controllers
	virtual ICharacterController* createCharacterController(const ControllerDesc& desc) X_ABSTRACT;
	virtual void releaseCharacterController(ICharacterController* pController) X_ABSTRACT;

	// some query api.

	virtual bool raycast(const Vec3f& origin, const Vec3f& unitDir, const float32_t distance,
		RaycastCallback& hitCall, HitFlags hitFlags = HitFlag::POSITION | HitFlag::NORMAL | HitFlag::DISTANCE,
		QueryFlags queryFlags = QueryFlag::STATIC | QueryFlag::DYNAMIC) const X_ABSTRACT;

	virtual bool sweep(const GeometryBase& geometry, const Transformf& pose, const Vec3f& unitDir, const float32_t distance,
		SweepCallback& hitCall, HitFlags hitFlags = HitFlag::POSITION | HitFlag::NORMAL | HitFlag::DISTANCE,
		QueryFlags queryFlags = QueryFlag::STATIC | QueryFlag::DYNAMIC,
		const float32_t inflation = 0.f) const X_ABSTRACT;

	virtual bool overlap(const GeometryBase& geometry, const Transformf& pose, OverlapCallback& hitCall) const X_ABSTRACT;

	virtual	IBatchedQuery* createBatchQuery(const QueryMemory& desc) X_ABSTRACT;

	// post simulation results.
	virtual const ActiveTransform* getActiveTransforms(size_t& numTransformsOut) X_ABSTRACT;
	virtual const TriggerPair* getTriggerPairs(size_t& numTriggerPairs) X_ABSTRACT;

};

struct ScopedLock
{
	ScopedLock(IScene* pScene, bool write = false) : pScene_(pScene) {
		lock_ = pScene->lock(write);
	}

	~ScopedLock() {
		pScene_->unLock(lock_);
	}

private:
	LockHandle lock_;
	IScene* pScene_;
};

// ------------------------------------------------

struct IPhysics
{
	typedef core::Array<uint8_t> DataArr;

	virtual ~IPhysics() {}

	virtual void registerVars(void) X_ABSTRACT;
	virtual void registerCmds(void) X_ABSTRACT;

	virtual bool init(const ToleranceScale& scale) X_ABSTRACT;
	virtual void shutDown(void) X_ABSTRACT;
	virtual void release(void) X_ABSTRACT;

	virtual void onTickPreRender(float dtime, const AABB& debugVisCullBounds) X_ABSTRACT;
	virtual void onTickPostRender(float dtime) X_ABSTRACT;
	virtual void render(void) X_ABSTRACT; // render stuff like debug shapes.

	// if you create a full physics instance you get cooking with it.
	// if you want just cooking use the converter interface.
	virtual IPhysicsCooking* getCooking(void) X_ABSTRACT;

	// Scene stuff
	virtual IScene* createScene(const SceneDesc& desc) X_ABSTRACT;
	// once this is called at static geo should of been added, to prevent AABB rebuilds. (adding static stuff after is still possible tho)
	virtual void addSceneToSim(IScene* pScene) X_ABSTRACT;
	virtual bool removeSceneFromSim(IScene* pScene) X_ABSTRACT;
	virtual void releaseScene(IScene* pScene) X_ABSTRACT;

	// we need to make a api for creating the physc objects for use in the 3dengine.
	virtual MaterialHandle createMaterial(MaterialDesc& desc) X_ABSTRACT;
	virtual MaterialHandle getDefaultMaterial(void) X_ABSTRACT;

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
		const Transformf& localFrame0, const Transformf& localFrame1) X_ABSTRACT;
	virtual void releaseJoint(IJoint* pJoint) X_ABSTRACT;

	// debug name for logs, only stores the pointer you must ensure the memory outlives the actor :) !
	virtual void setActorDebugNamePointer(ActorHandle handle, const char* pNamePointer) X_ABSTRACT;
	// value between 0-31 groups that higer can pusher lower ones, all actors default to 0.
	virtual void setActorDominanceGroup(ActorHandle handle, int8_t group) X_ABSTRACT;
	// actors have default group, only need to call this for none default.
	virtual void setGroup(ActorHandle handle, const GroupFlag::Enum group) X_ABSTRACT;
	// for setting multiple groups on a actor, this does not override the group, set with function above.
	virtual void setGroupFlags(ActorHandle handle, const GroupFlags groupFlags) X_ABSTRACT;

	// for setting what collides with what, by default everything collides.
	virtual bool GetGroupCollisionFlag(const GroupFlag::Enum group1, const GroupFlag::Enum group2) X_ABSTRACT;
	virtual void SetGroupCollisionFlag(const GroupFlag::Enum group1, const GroupFlag::Enum group2, const bool enable) X_ABSTRACT;


	virtual TriMeshHandle createTriangleMesh(const DataArr& cooked) X_ABSTRACT;
	virtual ConvexMeshHandle createConvexMesh(const DataArr& cooked) X_ABSTRACT;
	virtual ConvexMeshHandle createConvexMesh(const uint8_t* pData, size_t length) X_ABSTRACT;
	virtual HieghtFieldHandle createHieghtField(const DataArr& cooked) X_ABSTRACT;


	virtual ActorHandle createConvexMesh(const Transformf& myTrans, TriMeshHandle mesh, float density, const Vec3f& scale = Vec3f::one()) X_ABSTRACT;
	virtual ActorHandle createTriangleMesh(const Transformf& myTrans, ConvexMeshHandle convex, float density, const Vec3f& scale = Vec3f::one()) X_ABSTRACT;
	virtual ActorHandle createHieghtField(const Transformf& myTrans, HieghtFieldHandle hf, float density, const Vec3f& heightRowColScale = Vec3f::one()) X_ABSTRACT;
	virtual ActorHandle createSphere(const Transformf& myTrans, float radius, float density) X_ABSTRACT;
	virtual ActorHandle createCapsule(const Transformf& myTrans, float radius, float halfHeight, float density) X_ABSTRACT;
	virtual ActorHandle createBox(const Transformf& myTrans, const AABB& bounds, float density) X_ABSTRACT;

	virtual ActorHandle createStaticTriangleMesh(const Transformf& myTrans, TriMeshHandle mesh, const Vec3f& scale = Vec3f::one()) X_ABSTRACT;
	virtual ActorHandle createStaticHieghtField(const Transformf& myTrans, HieghtFieldHandle hf, const Vec3f& heightRowColScale = Vec3f::one()) X_ABSTRACT;
	virtual ActorHandle createStaticPlane(const Transformf& myTrans) X_ABSTRACT;
	virtual ActorHandle createStaticSphere(const Transformf& myTrans, float radius) X_ABSTRACT;
	virtual ActorHandle createStaticCapsule(const Transformf& myTrans, float radius, float halfHeight) X_ABSTRACT;
	virtual ActorHandle createStaticBox(const Transformf& myTrans, const AABB& bounds) X_ABSTRACT;
	virtual ActorHandle createStaticTrigger(const Transformf& myTrans, const AABB& bounds) X_ABSTRACT;

	// for creating a actor without any initial shape.
	virtual ActorHandle createActor(const Transformf& myTrans, float density, bool kinematic = false, const void* pUserData = nullptr) X_ABSTRACT;
	virtual ActorHandle createStaticActor(const Transformf& myTrans, const void* pUserData = nullptr) X_ABSTRACT;


	// adding additional / initial shape to a actor.
	virtual void addTriMesh(ActorHandle handle, TriMeshHandle mesh, const Vec3f& scale = Vec3f::one()) X_ABSTRACT;
	virtual void addConvexMesh(ActorHandle handle, ConvexMeshHandle mesh, const Vec3f& scale = Vec3f::one()) X_ABSTRACT;
	virtual void addHieghtField(ActorHandle handle, HieghtFieldHandle hf, const Vec3f& heightRowColScale = Vec3f::one()) X_ABSTRACT;
	virtual void addBox(ActorHandle handle, const AABB& aabb) X_ABSTRACT;
	virtual void addBox(ActorHandle handle, const AABB& aabb, const Vec3f& localPose) X_ABSTRACT;
	virtual void addSphere(ActorHandle handle, float radius) X_ABSTRACT;
	virtual void addSphere(ActorHandle handle, float radius, const Vec3f& localPose) X_ABSTRACT;
	virtual void addCapsule(ActorHandle handle, float radius, float halfHeight) X_ABSTRACT;
};


X_NAMESPACE_END

#endif // !_X_PHYSICS_I_H_