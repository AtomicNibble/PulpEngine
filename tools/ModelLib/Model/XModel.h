#pragma once

#ifndef X_MODEL_H_
#define X_MODEL_H_

#include <Util\UniquePointer.h>
#include <Util\Span.h>
#include <Assets\AssetBase.h>

#include <IModel.h>

X_NAMESPACE_DECLARE(physics,
                    typedef uintptr_t Handle;
                    typedef Handle ActorHandle;)

X_NAMESPACE_DECLARE(engine, struct IMaterialManager)

X_NAMESPACE_BEGIN(model)

class XModel : public core::AssetBase
{
    X_NO_COPY(XModel);
    X_NO_ASSIGN(XModel);

    typedef core::Array<Matrix44f> MatrixArr;

public:
    MODELLIB_EXPORT XModel(core::string& name);
    MODELLIB_EXPORT virtual ~XModel();

    X_INLINE int32_t getNumLods(void) const;
    X_INLINE int32_t getNumBones(void) const;
    X_INLINE int32_t getNumRootBones(void) const;
    X_INLINE int32_t getNumMeshTotal(void) const;
    X_INLINE int32_t getNumVerts(size_t lodIdx) const;
    X_INLINE bool hasLods(void) const;
    X_INLINE bool hasPhys(void) const;
    X_INLINE bool isAnimated(void) const;
    X_INLINE size_t lodIdxForDistance(float distance) const;

    X_INLINE const AABB& bounds(void) const;
    X_INLINE const AABB& bounds(size_t lodIdx) const;
    X_INLINE const Sphere& boundingSphere(size_t lodIdx) const;

    X_INLINE const LODHeader& getLod(size_t idx) const;
    X_INLINE const MeshHeader& getLodMeshHdr(size_t idx) const;
    X_INLINE const SubMeshHeader& getMeshHead(size_t idx) const;

    X_INLINE const char* getBoneName(size_t idx) const;
    X_INLINE BoneHandle getBoneHandle(const char* pName) const;
    X_INLINE core::span<const uint8_t> getTagTree(void) const;
    X_INLINE const XQuatCompressedf& getBoneAngle(size_t idx) const;
    X_INLINE const XQuatCompressedf& getBoneAngleRel(size_t idx) const;
    X_INLINE const Vec3f getBonePosRel(size_t idx) const;
    X_INLINE const Vec3f getBonePos(size_t idx) const;
    X_INLINE const MatrixArr& getInverseBoneMatrix(void) const;

    MODELLIB_EXPORT bool processData(core::UniquePointer<char[]> data, uint32_t dataSize, engine::IMaterialManager* pMatMan);

    MODELLIB_EXPORT void addPhysToActor(physics::ActorHandle actor);

protected:
    // runtime pointers.
    const uint16_t* pTagNames_;
    const uint8_t* pTagTree_;
    const XQuatCompressedf* pBoneAngles_;
    const Vec3f* pBonePos_;

    const XQuatCompressedf* pBoneAnglesRel_;
    const Vec3f* pBonePosRel_;

    // pointer to all the meshes headers for all lods
    // sotred in lod order.
    // can probs get rid of this pointer.
    const SubMeshHeader* pMeshHeads_;

    core::UniquePointer<char[]> data_;
    MatrixArr inverseBones_; // could skip this allocation later by just increasing size of data buffer we allocate.
    ModelHeader* pHdr_;
};

X_NAMESPACE_END

#include "XModel.inl"

#endif // !X_MODEL_H_