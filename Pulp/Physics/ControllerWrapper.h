#pragma once


X_NAMESPACE_BEGIN(physics)



template<class Base, typename ControllerType>
class XCharController : public Base
{
	typedef typename ControllerType ControllerType;

protected:
	X_INLINE XCharController(ControllerType* pController);
public:
	X_INLINE ~XCharController() X_OVERRIDE;

	X_INLINE bool setPosition(const Vec3d& position) X_FINAL;
	X_INLINE Vec3d getPosition(void) const X_FINAL;
			 
	X_INLINE bool setFootPosition(const Vec3d& position) X_FINAL;
	X_INLINE Vec3d getFootPosition(void) const X_FINAL;
			 
	X_INLINE void setStepOffset(const float32_t offset) X_FINAL;
	X_INLINE float32_t getStepOffset(void) const X_FINAL;
			 
	X_INLINE void resize(float32_t height) X_FINAL;

protected:
	X_INLINE const ControllerType* getController(void) const;
	X_INLINE ControllerType* getController(void);


private:
	ControllerType* pController_;
};

class XBoxCharController : public XCharController<IBoxCharacterController, physx::PxBoxController>
{
public:
	X_INLINE XBoxCharController(physx::PxBoxController* pController);
	X_INLINE virtual ~XBoxCharController() X_OVERRIDE;

	X_INLINE float32_t getHalfHeight(void) const X_FINAL;
	X_INLINE float32_t getHalfSideExtent(void) const X_FINAL;
	X_INLINE float32_t getHalfForwardExtent(void) const X_FINAL;
			 
	X_INLINE bool setHalfHeight(float32_t halfHeight) X_FINAL;
	X_INLINE bool setHalfSideExtent(float32_t halfSideExtent) X_FINAL;
	X_INLINE bool setHalfForwardExtent(float32_t halfForwardExtent) X_FINAL;
};


class XCapsuleCharController : public XCharController<ICapsuleCharacterController, physx::PxCapsuleController>
{
public:
	X_INLINE XCapsuleCharController(physx::PxCapsuleController* pController);
	X_INLINE virtual ~XCapsuleCharController() X_OVERRIDE;

	X_INLINE float32_t getRadius(void) const X_FINAL;
	X_INLINE float32_t getHeight(void) const X_FINAL;
	X_INLINE CapsuleControllerDesc::ClimbingMode getClimbingMode(void) const X_FINAL;
			 
	X_INLINE bool setRadius(float32_t radius) X_FINAL;
	X_INLINE bool setHeight(float32_t height) X_FINAL;
	X_INLINE bool setClimbingMode(CapsuleControllerDesc::ClimbingMode mode) X_FINAL;
};

X_NAMESPACE_END


#include "ControllerWrapper.inl"