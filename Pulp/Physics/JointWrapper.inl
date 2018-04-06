
X_NAMESPACE_BEGIN(physics)

template<class Base, typename JointType>
X_INLINE XJoint<Base, JointType>::XJoint(JointType* pJoint) :
    pJoint_(pJoint)
{
}

template<class Base, typename JointType>
X_INLINE XJoint<Base, JointType>::~XJoint()
{
    getJoint()->release();
}

template<class Base, typename JointType>
X_INLINE void XJoint<Base, JointType>::setBreakForce(float32_t force, float32_t torque)
{
    getJoint()->setBreakForce(force, torque);
}

template<class Base, typename JointType>
X_INLINE void XJoint<Base, JointType>::getBreakForce(float32_t& force, float32_t& torque) const
{
    getJoint()->getBreakForce(force, torque);
}

template<class Base, typename JointType>
X_INLINE void XJoint<Base, JointType>::setLocalPose(ActorIdx actor, const Transformf& localPose)
{
    getJoint()->setLocalPose(getPhysxActorIndex(actor), PxTransFromQuatTrans(localPose));
}

template<class Base, typename JointType>
X_INLINE Transformf XJoint<Base, JointType>::getLocalPose(ActorIdx actor) const
{
    auto pxTrans = getJoint()->getLocalPose(getPhysxActorIndex(actor));

    return QuatTransFromPxTrans(pxTrans);
}

template<class Base, typename JointType>
X_INLINE const JointType* XJoint<Base, JointType>::getJoint(void) const
{
    return pJoint_;
}

template<class Base, typename JointType>
X_INLINE JointType* XJoint<Base, JointType>::getJoint(void)
{
    return pJoint_;
}

template<class Base, typename JointType>
X_INLINE physx::PxJointActorIndex::Enum XJoint<Base, JointType>::getPhysxActorIndex(ActorIdx idx)
{
    if (idx == ActorIdx::Actor0) {
        return physx::PxJointActorIndex::eACTOR0;
    }
    if (idx == ActorIdx::Actor1) {
        return physx::PxJointActorIndex::eACTOR1;
    }

    X_ASSERT_UNREACHABLE();
    return physx::PxJointActorIndex::eACTOR0;
}

// --------------------------------------------------------------

X_INLINE XFixedJoint::XFixedJoint(physx::PxFixedJoint* pJoint) :
    XJoint(pJoint)
{
}

X_INLINE XFixedJoint::~XFixedJoint()
{
}

// --------------------------------------------------------------

X_INLINE XDistanceJoint::XDistanceJoint(physx::PxDistanceJoint* pJoint) :
    XJoint(pJoint)
{
}

X_INLINE XDistanceJoint::~XDistanceJoint()
{
}

X_INLINE float32_t XDistanceJoint::getDistance(void) const
{
    return getJoint()->getDistance();
}

X_INLINE void XDistanceJoint::setMinDistance(float32_t distance)
{
    return getJoint()->setMinDistance(distance);
}

X_INLINE float32_t XDistanceJoint::getMinDistance(void) const
{
    return getJoint()->getMinDistance();
}

X_INLINE void XDistanceJoint::setMaxDistance(float32_t distance)
{
    return getJoint()->setMaxDistance(distance);
}

X_INLINE float32_t XDistanceJoint::getMaxDistance(void) const
{
    return getJoint()->getMaxDistance();
}

X_INLINE void XDistanceJoint::setTolerance(float32_t tolerance)
{
    return getJoint()->setTolerance(tolerance);
}

X_INLINE float32_t XDistanceJoint::getTolerance(void) const
{
    return getJoint()->getTolerance();
}

X_INLINE void XDistanceJoint::setStiffness(float32_t spring)
{
    return getJoint()->setStiffness(spring);
}

X_INLINE float32_t XDistanceJoint::getStiffness(void) const
{
    return getJoint()->getStiffness();
}

X_INLINE void XDistanceJoint::setDamping(float32_t damping)
{
    return getJoint()->setDamping(damping);
}

X_INLINE float32_t XDistanceJoint::getDamping(void) const
{
    return getJoint()->getDamping();
}

// --------------------------------------------------------------

X_INLINE XSphericalJoint::XSphericalJoint(physx::PxSphericalJoint* pJoint) :
    XJoint(pJoint)
{
}

X_INLINE XSphericalJoint::~XSphericalJoint()
{
}

X_INLINE JointLimitCone XSphericalJoint::getLimitCone(void) const
{
    auto pxLimit = getJoint()->getLimitCone();

    JointLimitCone limit;
    limit.restitution = pxLimit.restitution;
    limit.bounceThreshold = pxLimit.bounceThreshold;
    limit.stiffness = pxLimit.stiffness;
    limit.damping = pxLimit.damping;
    limit.contactDistance = pxLimit.contactDistance;
    limit.yAngle = pxLimit.yAngle;
    limit.zAngle = pxLimit.zAngle;
    return limit;
}

X_INLINE void XSphericalJoint::setLimitCone(const JointLimitCone& limit)
{
    physx::PxJointLimitCone pxLimit(limit.yAngle, limit.zAngle, limit.contactDistance);

    getJoint()->setLimitCone(pxLimit);
}

X_INLINE bool XSphericalJoint::limitEnabled(void) const
{
    return getJoint()->getSphericalJointFlags() & physx::PxSphericalJointFlag::eLIMIT_ENABLED;
}

X_INLINE void XSphericalJoint::setLimitEnabled(bool enable)
{
    getJoint()->setSphericalJointFlag(physx::PxSphericalJointFlag::eLIMIT_ENABLED, enable);
}

// --------------------------------------------------------------

X_INLINE XRevoluteJoint::XRevoluteJoint(physx::PxRevoluteJoint* pJoint) :
    XJoint(pJoint)
{
}

X_INLINE XRevoluteJoint::~XRevoluteJoint()
{
}

X_INLINE float32_t XRevoluteJoint::getAngle(void) const
{
    return getJoint()->getAngle();
}

X_INLINE float32_t XRevoluteJoint::getVelocity(void) const
{
    return getJoint()->getVelocity();
}

X_INLINE void XRevoluteJoint::setLimit(const JointAngularLimitPair& limit)
{
    physx::PxJointAngularLimitPair pxLimit(limit.lower, limit.upper, limit.contactDistance);
    pxLimit.restitution = pxLimit.restitution;
    pxLimit.bounceThreshold = pxLimit.bounceThreshold;
    pxLimit.stiffness = pxLimit.stiffness;
    pxLimit.damping = pxLimit.damping;
    pxLimit.contactDistance = pxLimit.contactDistance;

    getJoint()->setLimit(pxLimit);
}

X_INLINE JointAngularLimitPair XRevoluteJoint::getLimit(void) const
{
    auto pxLimit = getJoint()->getLimit();

    JointAngularLimitPair limit;
    limit.restitution = pxLimit.restitution;
    limit.bounceThreshold = pxLimit.bounceThreshold;
    limit.stiffness = pxLimit.stiffness;
    limit.damping = pxLimit.damping;
    limit.contactDistance = pxLimit.contactDistance;
    limit.upper = pxLimit.upper;
    limit.lower = pxLimit.lower;
    return limit;
}

// --------------------------------------------------------------

X_INLINE XPrismaticJoint::XPrismaticJoint(physx::PxPrismaticJoint* pJoint) :
    XJoint(pJoint)
{
}

X_INLINE XPrismaticJoint::~XPrismaticJoint()
{
}

X_INLINE float32_t XPrismaticJoint::getPosition(void)
{
    return getJoint()->getPosition();
}

X_INLINE float32_t XPrismaticJoint::getVelocity(void)
{
    return getJoint()->getVelocity();
}

X_INLINE void XPrismaticJoint::setLimit(const JointLinearLimitPair& limit)
{
    physx::PxJointLinearLimitPair pxLimit(limit.lower, limit.upper, physx::PxSpring(limit.stiffness, limit.damping));
    pxLimit.restitution = pxLimit.restitution;
    pxLimit.bounceThreshold = pxLimit.bounceThreshold;
    pxLimit.contactDistance = pxLimit.contactDistance;

    getJoint()->setLimit(pxLimit);
}

X_INLINE JointLinearLimitPair XPrismaticJoint::getLimit(void) const
{
    auto pxLimit = getJoint()->getLimit();

    JointLinearLimitPair limit;
    limit.restitution = pxLimit.restitution;
    limit.bounceThreshold = pxLimit.bounceThreshold;
    limit.stiffness = pxLimit.stiffness;
    limit.damping = pxLimit.damping;
    limit.contactDistance = pxLimit.contactDistance;
    limit.upper = pxLimit.upper;
    limit.lower = pxLimit.lower;
    return limit;
}

X_NAMESPACE_END
