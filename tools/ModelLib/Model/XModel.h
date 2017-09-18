#pragma once


#ifndef X_MODEL_H_
#define X_MODEL_H_

#include <Util\UniquePointer.h>

#include <IModel.h>
#include <IAsyncLoad.h>



X_NAMESPACE_DECLARE(physics, 
typedef uintptr_t Handle;
typedef Handle ActorHandle;
)

X_NAMESPACE_DECLARE(engine, struct IMaterialManager)

X_NAMESPACE_BEGIN(model)


class XModel
{
	X_NO_COPY(XModel);
	X_NO_ASSIGN(XModel);

	typedef core::Array<Matrix44f> MatrixArr;

public:
	MODELLIB_EXPORT XModel(core::string& name);
	MODELLIB_EXPORT virtual ~XModel();

	X_INLINE const int32_t getID(void) const;
	X_INLINE void setID(int32_t id);

	X_INLINE core::LoadStatus::Enum getStatus(void) const;
	X_INLINE bool isLoaded(void) const;
	X_INLINE bool loadFailed(void) const;
	X_INLINE void setStatus(core::LoadStatus::Enum status);

	X_INLINE const core::string& getName(void) const;
	X_INLINE int32_t numLods(void) const;
	X_INLINE int32_t numBones(void) const;
	X_INLINE int32_t numRootBones(void) const;
	X_INLINE int32_t numMeshTotal(void) const;
	X_INLINE int32_t numVerts(size_t lodIdx) const;
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
	X_INLINE const uint8_t*	getTagTree(void) const;
	X_INLINE const XQuatCompressedf& getBoneAngle(size_t idx) const;
	X_INLINE const XQuatCompressedf& getBoneAngleRel(size_t idx) const;
	X_INLINE const Vec3f getBonePosRel(size_t idx) const;
	X_INLINE const Vec3f getBonePos(size_t idx) const;
	X_INLINE const MatrixArr& getInverseBoneMatrix(void) const;


	MODELLIB_EXPORT void processData(ModelHeader& hdr, core::UniquePointer<uint8_t[]> data, engine::IMaterialManager* pMatMan);

	MODELLIB_EXPORT void addPhysToActor(physics::ActorHandle actor);

protected:
	int32_t id_;
	core::string name_;

	core::LoadStatus::Enum status_;
	uint8_t _pad[3];

	// runtime pointers.
	const uint16_t*			pTagNames_;
	const uint8_t*			pTagTree_;
	const XQuatCompressedf* pBoneAngles_;
	const Vec3f*			pBonePos_;

	const XQuatCompressedf* pBoneAnglesRel_;
	const Vec3f*			pBonePosRel_;

	// pointer to all the meshes headers for all lods
	// sotred in lod order.
	// can probs get rid of this pointer.
	const SubMeshHeader*	pMeshHeads_;

	core::UniquePointer<uint8_t[]> data_;
	MatrixArr inverseBones_; // could skip this allocation later by just increasing size of data buffer we allocate.
	ModelHeader hdr_;
};


X_NAMESPACE_END

#include "XModel.inl"

#endif // !X_MODEL_H_