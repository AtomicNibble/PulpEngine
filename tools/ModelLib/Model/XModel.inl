
X_NAMESPACE_BEGIN(model)


X_INLINE int32_t XModel::getNumLods(void) const
{
	return pHdr_->numLod;
}

X_INLINE int32_t XModel::getNumBones(void) const
{
	return pHdr_->numBones;
}

X_INLINE int32_t XModel::getNumRootBones(void) const
{
	return pHdr_->numRootBones;
}

X_INLINE int32_t XModel::getNumMeshTotal(void) const
{
	return pHdr_->numMesh;
}

X_INLINE int32_t XModel::getNumVerts(size_t lodIdx) const
{
	X_ASSERT(lodIdx < static_cast<size_t>(getNumLods()), "invalid lod index")(getNumLods(), lodIdx);

	return pHdr_->lodInfo[lodIdx].numVerts;
}

X_INLINE bool XModel::hasLods(void) const
{
	return getNumLods() > 1;
}


X_INLINE bool XModel::hasPhys(void) const
{
	X_ASSERT_NOT_NULL(pHdr_);
	return pHdr_->flags.IsSet(ModelFlag::PHYS_DATA);
}

X_INLINE bool XModel::isAnimated(void) const
{
	X_ASSERT_NOT_NULL(pHdr_);
	return pHdr_->flags.IsSet(ModelFlag::ANIMATED);
}


X_INLINE size_t XModel::lodIdxForDistance(float distance) const
{
	// we select the lowest level lod that is visible
	for (int32_t i = 0; i < getNumLods(); i++)
	{
		if (pHdr_->lodInfo[i].lodDistance > distance) {
			return i;
		}
	}

	// lowest lod
	return getNumLods() - 1;
}

X_INLINE const AABB& XModel::bounds(void) const
{
	return pHdr_->boundingBox;
}

X_INLINE const AABB& XModel::bounds(size_t lodIdx) const
{
	return pHdr_->lodInfo[lodIdx].boundingBox;
}

X_INLINE const Sphere& XModel::boundingSphere(size_t lodIdx) const
{
	return pHdr_->lodInfo[lodIdx].boundingSphere;
}


X_INLINE const LODHeader& XModel::getLod(size_t idx) const
{
	X_ASSERT(idx < static_cast<size_t>(getNumLods()), "invalid lod index")(getNumLods(), idx);
	return pHdr_->lodInfo[idx];
}

X_INLINE const MeshHeader& XModel::getLodMeshHdr(size_t idx) const
{
	X_ASSERT(idx < static_cast<size_t>(getNumLods()), "invalid lod index")(getNumLods(), idx);
	return pHdr_->lodInfo[idx];
}

X_INLINE const SubMeshHeader& XModel::getMeshHead(size_t idx) const
{
	X_ASSERT(idx < static_cast<size_t>(getNumMeshTotal()), "invalid mesh index")(getNumMeshTotal(), idx);
	X_ASSERT_NOT_NULL(pMeshHeads_);
	return pMeshHeads_[idx];
}


X_INLINE const char* XModel::getBoneName(size_t idx) const
{
	X_ASSERT(static_cast<int32_t>(idx) < getNumBones(), "invalid bone index")(getNumBones(), idx);

	// temp hack.
	const char* pBoneName = (char*)(data_.ptr() + sizeof(ModelHeader) + pTagNames_[idx]);

	return pBoneName;
}


X_INLINE BoneHandle XModel::getBoneHandle(const char* pName) const
{
	for (int32_t i = 0; i < getNumBones(); i++)
	{
		if (core::strUtil::IsEqual(pName, getBoneName(i)))
		{
			return i;
		}
	}

	return INVALID_BONE_HANDLE;
}

X_INLINE core::span<const uint8_t>XModel::getTagTree(void) const
{
	return core::make_span(pTagTree_, getNumBones());
}

X_INLINE const XQuatCompressedf& XModel::getBoneAngle(size_t idx) const
{
	return pBoneAngles_[idx];
}

X_INLINE const XQuatCompressedf& XModel::getBoneAngleRel(size_t idx) const
{
	return pBoneAnglesRel_[idx];
}

X_INLINE const Vec3f XModel::getBonePosRel(size_t idx) const
{
	return pBonePosRel_[idx];
}

X_INLINE const Vec3f XModel::getBonePos(size_t idx) const
{
	return pBonePos_[idx];
}

X_INLINE const XModel::MatrixArr& XModel::getInverseBoneMatrix(void) const
{
	return inverseBones_;
}

X_NAMESPACE_END
