
X_NAMESPACE_BEGIN(physics)

template<class Base, typename ControllerType>
X_INLINE XCharController<Base, ControllerType>::XCharController(ControllerType* pController) :
    pController_(pController)
{
}

template<class Base, typename ControllerType>
X_INLINE XCharController<Base, ControllerType>::~XCharController()
{
    getController()->release();
}

template<class Base, typename ControllerType>
X_INLINE ControllerDesc::ShapeType XCharController<Base, ControllerType>::getType(void) const
{
    auto pxShapeType = getController()->getType();
    if (pxShapeType == physx::PxControllerShapeType::eBOX) {
        return ControllerDesc::ShapeType::Box;
    }
    if (pxShapeType == physx::PxControllerShapeType::eCAPSULE) {
        return ControllerDesc::ShapeType::Capsule;
    }

    X_ASSERT_UNREACHABLE();
    return ControllerDesc::ShapeType::Box;
}

template<class Base, typename ControllerType>
X_INLINE ICharacterController::ColFlags XCharController<Base, ControllerType>::move(const Vec3f& disp, float32_t minDist,
    float32_t elapsedTime)
{
    physx::PxControllerCollisionFlags flags = getController()->move(Px3FromVec3(disp), minDist, elapsedTime, physx::PxControllerFilters(), nullptr);

    static_assert(physx::PxControllerCollisionFlag::eCOLLISION_SIDES == ColFlags::SIDES, "Flags don't match");
    static_assert(physx::PxControllerCollisionFlag::eCOLLISION_UP == ColFlags::UP, "Flags don't match");
    static_assert(physx::PxControllerCollisionFlag::eCOLLISION_DOWN == ColFlags::DOWN, "Flags don't match");

    return ColFlags(flags);
}

template<class Base, typename ControllerType>
X_INLINE bool XCharController<Base, ControllerType>::setPosition(const Vec3d& position)
{
    return getController()->setPosition(Px3ExtFromVec3(position));
}

template<class Base, typename ControllerType>
X_INLINE Vec3d XCharController<Base, ControllerType>::getPosition(void) const
{
    return Vec3FromPx3Ext(getController()->getPosition());
}

template<class Base, typename ControllerType>
X_INLINE bool XCharController<Base, ControllerType>::setFootPosition(const Vec3d& position)
{
    return getController()->setFootPosition(Px3ExtFromVec3(position));
}

template<class Base, typename ControllerType>
X_INLINE Vec3d XCharController<Base, ControllerType>::getFootPosition(void) const
{
    return Vec3FromPx3Ext(getController()->getFootPosition());
}

template<class Base, typename ControllerType>
X_INLINE void XCharController<Base, ControllerType>::setStepOffset(const float32_t offset)
{
    getController()->setStepOffset(offset);
}

template<class Base, typename ControllerType>
X_INLINE float32_t XCharController<Base, ControllerType>::getStepOffset(void) const
{
    return getController()->getStepOffset();
}

template<class Base, typename ControllerType>
X_INLINE void XCharController<Base, ControllerType>::resize(float32_t height)
{
    getController()->resize(height);
}

template<class Base, typename ControllerType>
X_INLINE void XCharController<Base, ControllerType>::getState(ControllerState& stats)
{
    physx::PxControllerState pxState;
    getController()->getState(pxState);

    stats.deltaXP = Vec3FromPx3(pxState.deltaXP);
    stats.touchedActor = reinterpret_cast<ActorHandle>(pxState.touchedActor);
    stats.collisionFlags = CharacterColFlags(pxState.collisionFlags);
    stats.standOnAnotherCCT = pxState.standOnAnotherCCT;
    stats.standOnObstacle = pxState.standOnObstacle;
    stats.isMovingUp = pxState.isMovingUp;
}

template<class Base, typename ControllerType>
X_INLINE void XCharController<Base, ControllerType>::getStats(ControllerStats& states)
{
    physx::PxControllerStats pxStats;
    getController()->getStats(pxStats);

    states.nbIterations = pxStats.nbIterations;
    states.nbFullUpdates = pxStats.nbFullUpdates;
    states.nbPartialUpdates = pxStats.nbPartialUpdates;
    states.nbTessellation = pxStats.nbTessellation;
}

template<class Base, typename ControllerType>
X_INLINE const ControllerType* XCharController<Base, ControllerType>::getController(void) const
{
    return pController_;
}

template<class Base, typename ControllerType>
X_INLINE ControllerType* XCharController<Base, ControllerType>::getController(void)
{
    return pController_;
}

// --------------------------------------------------------------

X_INLINE XBoxCharController::XBoxCharController(physx::PxBoxController* pController) :
    XCharController(pController)
{
}

X_INLINE XBoxCharController::~XBoxCharController()
{
}

X_INLINE float32_t XBoxCharController::getHalfHeight(void) const
{
    return getController()->getHalfHeight();
}

X_INLINE float32_t XBoxCharController::getHalfSideExtent(void) const
{
    return getController()->getHalfSideExtent();
}

X_INLINE float32_t XBoxCharController::getHalfForwardExtent(void) const
{
    return getController()->getHalfForwardExtent();
}

X_INLINE bool XBoxCharController::setHalfHeight(float32_t halfHeight)
{
    return getController()->setHalfHeight(halfHeight);
}

X_INLINE bool XBoxCharController::setHalfSideExtent(float32_t halfSideExtent)
{
    return getController()->setHalfSideExtent(halfSideExtent);
}

X_INLINE bool XBoxCharController::setHalfForwardExtent(float32_t halfForwardExtent)
{
    return getController()->setHalfForwardExtent(halfForwardExtent);
}

// --------------------------------------------------------------

X_INLINE XCapsuleCharController::XCapsuleCharController(physx::PxCapsuleController* pController) :
    XCharController(pController)
{
}

X_INLINE XCapsuleCharController::~XCapsuleCharController()
{
}

X_INLINE float32_t XCapsuleCharController::getRadius(void) const
{
    return getController()->getRadius();
}

X_INLINE float32_t XCapsuleCharController::getHeight(void) const
{
    return getController()->getHeight();
}

X_INLINE CapsuleControllerDesc::ClimbingMode XCapsuleCharController::getClimbingMode(void) const
{
    auto pxMode = getController()->getClimbingMode();
    if (pxMode == physx::PxCapsuleClimbingMode::eEASY) {
        return CapsuleControllerDesc::ClimbingMode::Easy;
    }
    if (pxMode == physx::PxCapsuleClimbingMode::eCONSTRAINED) {
        return CapsuleControllerDesc::ClimbingMode::Constrained;
    }

    X_ASSERT_UNREACHABLE();
    return CapsuleControllerDesc::ClimbingMode::Easy;
}

X_INLINE bool XCapsuleCharController::setRadius(float32_t radius)
{
    return getController()->setRadius(radius);
}

X_INLINE bool XCapsuleCharController::setHeight(float32_t height)
{
    return getController()->setHeight(height);
}

X_INLINE bool XCapsuleCharController::setClimbingMode(CapsuleControllerDesc::ClimbingMode mode)
{
    if (mode == CapsuleControllerDesc::ClimbingMode::Easy) {
        return getController()->setClimbingMode(physx::PxCapsuleClimbingMode::eEASY);
    }
    if (mode == CapsuleControllerDesc::ClimbingMode::Constrained) {
        return getController()->setClimbingMode(physx::PxCapsuleClimbingMode::eCONSTRAINED);
    }

    X_ASSERT_UNREACHABLE();
    return false;
}

X_NAMESPACE_END