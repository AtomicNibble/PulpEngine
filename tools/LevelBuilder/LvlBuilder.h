#pragma once

#ifndef X_LVL_BUILDER_H_
#define X_LVL_BUILDER_H_

X_NAMESPACE_DECLARE(physics,
	struct IPhysicsCooking;
);

X_NAMESPACE_DECLARE(lvl,
	class ModelCache;
);

#include "MaterialManager.h"
#include "LvlTypes.h"

#include <array>


class LvlBuilder
{
	typedef core::Array<level::FileStaticModel> StaticModelsArr;
	typedef std::array<core::Array<level::MultiAreaEntRef>, 
		level::MAP_MAX_MULTI_REF_LISTS> MultiRefArr;

public:
	LvlBuilder(physics::IPhysicsCooking* pPhysCooking, core::MemoryArenaBase* arena);
	~LvlBuilder();

	bool init(void);

	bool LoadFromMap(mapfile::XMapFile* map);
	bool ProcessModels(void);

	bool save(const char* filename);

private:
	int32_t FindFloatPlane(const Planef& plane);

	bool processMapEntity(LvlEntity& ent, mapfile::XMapEntity* mapEnt);
	bool processBrush(LvlEntity& ent, mapfile::XMapBrush* brush, size_t entIdx);
	bool processPatch(LvlEntity& ent, mapfile::XMapPatch* brush, size_t entIdx);

	bool removeDuplicateBrushPlanes(LvlBrush& pBrush);

private:	
	void calculateLvlBounds(void);

	bool ProcessModel(LvlEntity& ent);
	bool ProcessWorldModel(LvlEntity& ent);

private:
	int SelectSplitPlaneNum(bspNode* node, bspFace* faces);
	void BuildFaceTree_r(bspNode* node, bspFace* faces, size_t& numLeafs);

private:
	void SplitBrush(LvlBrush* brush, int32_t planenum, LvlBrush** front, LvlBrush** back);

private:
	bool PutPrimitivesInAreas(LvlEntity& ent);
	
	void PutWindingIntoAreas_r(LvlEntity& ent, XWinding* pWinding,
		LvlBrushSide& side, bspNode* pNode);

	bool AddMapTriToAreas(LvlEntity& worldEnt, XPlaneSet& planeSet, const LvlTris& tri);
	void AddTriListToArea(int32_t areaIdx, int32_t planeNum, const LvlTris& tris);

	bool CreateEntAreaRefs(LvlEntity& worldEnt);

	void AddAreaRefs_r(core::Array<int32_t>& areaList, const Sphere& sphere,
		const Vec3f boundsPoints[8], bspNode* pNode);

	// void CollectStaticModels(void);

private:
	core::MemoryArenaBase* arena_;

	StaticModelsArr staticModels_;

	LvlEntsArr	entities_;
	LvlAreaArr	areas_;

	MultiRefArr multiRefEntLists_;
	MultiRefArr multiModelRefLists_;

	core::GrowingStringTableUnique<256, 16, 4, uint32_t> stringTable_;

	//	BSPData		data_;
	XPlaneSet	planes_;
	AABB		mapBounds_;
	Vec3f		blockSize_;
	
	mapfile::XMapFile* pMap_;
	physics::IPhysicsCooking* pPhysCooking_;
	lvl::ModelCache* pModelCache_;
	lvl::MatManager matMan_;

	LvlStats stats_;
};


#endif // !X_LVL_BUILDER_H_