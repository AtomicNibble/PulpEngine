#pragma once

X_NAMESPACE_BEGIN(physics)

template<class Base, typename JointType>
class XJoint : public Base
{
    typedef typename Base::ActorIdx ActorIdx;
    typedef typename JointType JointType;

protected:
    X_INLINE XJoint(JointType* pJoint);

public:
    X_INLINE ~XJoint() X_OVERRIDE;

    X_INLINE void setBreakForce(float32_t force, float32_t torque) X_FINAL;
    X_INLINE void getBreakForce(float32_t& force, float32_t& torque) const X_FINAL;

    X_INLINE void setLocalPose(ActorIdx actor, const Transformf& localPose) X_FINAL;
    X_INLINE Transformf getLocalPose(ActorIdx actor) const X_FINAL;

protected:
    X_INLINE const JointType* getJoint(void) const;
    X_INLINE JointType* getJoint(void);

    X_INLINE static physx::PxJointActorIndex::Enum getPhysxActorIndex(ActorIdx idx);

private:
    JointType* pJoint_;
};

class XFixedJoint : public XJoint<IFixedJoint, physx::PxFixedJoint>
{
public:
    X_INLINE XFixedJoint(physx::PxFixedJoint* pJoint);
    X_INLINE virtual ~XFixedJoint() X_OVERRIDE;
};

class XDistanceJoint : public XJoint<IDistanceJoint, physx::PxDistanceJoint>
{
public:
    X_INLINE XDistanceJoint(physx::PxDistanceJoint* pJoint);
    X_INLINE virtual ~XDistanceJoint() X_OVERRIDE;

    X_INLINE float32_t getDistance(void) const X_FINAL;

    X_INLINE void setMinDistance(float32_t distance) X_FINAL;
    X_INLINE float32_t getMinDistance(void) const X_FINAL;

    X_INLINE void setMaxDistance(float32_t distance) X_FINAL;
    X_INLINE float32_t getMaxDistance(void) const X_FINAL;

    X_INLINE void setTolerance(float32_t tolerance) X_FINAL;
    X_INLINE float32_t getTolerance(void) const X_FINAL;

    X_INLINE void setStiffness(float32_t spring) X_FINAL;
    X_INLINE float32_t getStiffness(void) const X_FINAL;

    X_INLINE void setDamping(float32_t damping) X_FINAL;
    X_INLINE float32_t getDamping(void) const X_FINAL;
};

class XSphericalJoint : public XJoint<ISphericalJoint, physx::PxSphericalJoint>
{
public:
    X_INLINE XSphericalJoint(physx::PxSphericalJoint* pJoint);
    X_INLINE virtual ~XSphericalJoint() X_OVERRIDE;

    X_INLINE JointLimitCone getLimitCone(void) const X_FINAL;
    X_INLINE void setLimitCone(const JointLimitCone& limit) X_FINAL;

    X_INLINE bool limitEnabled(void) const X_FINAL;
    X_INLINE void setLimitEnabled(bool enable) X_FINAL;
};

class XRevoluteJoint : public XJoint<IRevoluteJoint, physx::PxRevoluteJoint>
{
public:
    X_INLINE XRevoluteJoint(physx::PxRevoluteJoint* pJoint);
    X_INLINE virtual ~XRevoluteJoint() X_OVERRIDE;

    X_INLINE float32_t getAngle(void) const X_FINAL;
    X_INLINE float32_t getVelocity(void) const X_FINAL;

    X_INLINE void setLimit(const JointAngularLimitPair& limits) X_FINAL;
    X_INLINE JointAngularLimitPair getLimit(void) const X_FINAL;
};

class XPrismaticJoint : public XJoint<IPrismaticJoint, physx::PxPrismaticJoint>
{
public:
    X_INLINE XPrismaticJoint(physx::PxPrismaticJoint* pJoint);
    X_INLINE virtual ~XPrismaticJoint() X_OVERRIDE;

    X_INLINE float32_t getPosition(void) X_FINAL;
    X_INLINE float32_t getVelocity(void) X_FINAL;

    X_INLINE void setLimit(const JointLinearLimitPair& limits) X_FINAL;
    X_INLINE JointLinearLimitPair getLimit(void) const X_FINAL;
};

X_NAMESPACE_END

#include "JointWrapper.inl"