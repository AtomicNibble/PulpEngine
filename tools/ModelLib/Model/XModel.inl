
X_NAMESPACE_BEGIN(model)


X_INLINE const int32_t XModel::getID(void) const
{
	return id_;
}

X_INLINE void XModel::setID(int32_t id)
{
	id_ = id;
}

X_INLINE core::LoadStatus::Enum XModel::getStatus(void) const
{
	return status_;
}

X_INLINE bool XModel::isLoaded(void) const
{
	return status_ == core::LoadStatus::Complete;
}

X_INLINE bool XModel::loadFailed(void) const
{
	return status_ == core::LoadStatus::Error;
}

X_INLINE void XModel::setStatus(core::LoadStatus::Enum status)
{
	status_ = status;
}

X_INLINE const core::string& XModel::getName(void) const
{
	return name_;
}

X_INLINE int32_t XModel::numLods(void) const
{
	return hdr_.numLod;
}

X_INLINE int32_t XModel::numBones(void) const
{
	return hdr_.numBones;
}

X_INLINE int32_t XModel::numBlankBones(void) const
{
	return hdr_.numBlankBones;
}

X_INLINE int32_t XModel::numMeshTotal(void) const
{
	return hdr_.numMesh;
}

X_INLINE int32_t XModel::numVerts(size_t lodIdx) const
{
	X_ASSERT(lodIdx < static_cast<size_t>(numLods()), "invalid lod index")(numLods(), lodIdx);

	return hdr_.lodInfo[lodIdx].numVerts;
}

X_INLINE bool XModel::hasLods(void) const
{
	return numLods() > 1;
}


X_INLINE bool XModel::hasPhys(void) const
{
	return hdr_.flags.IsSet(ModelFlag::PHYS_DATA);
}

X_INLINE bool XModel::isAnimated(void) const
{
	return hdr_.flags.IsSet(ModelFlag::ANIMATED);
}


X_INLINE size_t XModel::lodIdxForDistance(float distance) const
{
	// we select the lowest level lod that is visible
	for (int32_t i = 0; i < numLods(); i++)
	{
		if (hdr_.lodInfo[i].lodDistance > distance) {
			return i;
		}
	}

	// lowest lod
	return numLods() - 1;
}

X_INLINE const AABB& XModel::bounds(void) const
{
	return hdr_.boundingBox;
}

X_INLINE const AABB& XModel::bounds(size_t lodIdx) const
{
	return hdr_.lodInfo[lodIdx].boundingBox;
}

X_INLINE const Sphere& XModel::boundingSphere(size_t lodIdx) const
{
	return hdr_.lodInfo[lodIdx].boundingSphere;
}


X_INLINE const LODHeader& XModel::getLod(size_t idx) const
{
	X_ASSERT(idx < static_cast<size_t>(numLods()), "invalid lod index")(numLods(), idx);
	return hdr_.lodInfo[idx];
}

X_INLINE const MeshHeader& XModel::getLodMeshHdr(size_t idx) const
{
	X_ASSERT(idx < static_cast<size_t>(numLods()), "invalid lod index")(numLods(), idx);
	return hdr_.lodInfo[idx];
}

X_INLINE const SubMeshHeader& XModel::getMeshHead(size_t idx) const
{
	X_ASSERT(idx < static_cast<size_t>(numMeshTotal()), "invalid mesh index")(numMeshTotal(), idx);
	X_ASSERT_NOT_NULL(pMeshHeads_);
	return pMeshHeads_[idx];
}


X_NAMESPACE_END
