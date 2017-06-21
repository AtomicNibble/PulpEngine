#pragma once

#include "LvlTypes.h"

X_NAMESPACE_BEGIN(lvl)

class ModelCache;
class MatManager;

struct SourceStats
{
	SourceStats() {
		core::zero_this(this);
	}

	size_t numEntities;
	size_t numPatches;
	size_t numBrushes;
};


class LvlSource
{
public:
	LvlSource(core::MemoryArenaBase* arena, ModelCache& modelCache, MatManager& matMan);
	virtual ~LvlSource();


protected:
	void calculateLvlBounds(void);

	X_INLINE int32_t findFloatPlane(const Planef& plane);


protected:
	core::MemoryArenaBase* arena_;
	ModelCache& modelCache_;
	MatManager& matMan_;

	LvlEntsArr	entities_;
	XPlaneSet	planes_;
	AABB		mapBounds_;

	SourceStats stats_;
};

X_NAMESPACE_END


#include "LvlSource.inl"