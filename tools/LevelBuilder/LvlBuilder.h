#pragma once

#ifndef X_LVL_BUILDER_H_
#define X_LVL_BUILDER_H_

X_NAMESPACE_DECLARE(physics,
                    struct IPhysicsCooking);

#include "Material/MaterialManager.h"
#include "LvlEntity.h"
#include "LvlArea.h"

X_NAMESPACE_BEGIN(level)

namespace mapFile
{
    class XMapFile;
    class XMapEntity;
    class XMapBrush;
    class XMapPatch;
} // namespace mapFile

#if 0 
class ModelCache;

class LvlBuilder
{
	typedef core::Array<LvlArea> LvlAreaArr;
	typedef core::Array<LvlEntity> LvlEntsArr;

	typedef core::Array<level::FileStaticModel> StaticModelsArr;
	typedef std::array<core::Array<level::MultiAreaEntRef>, 
		level::MAP_MAX_MULTI_REF_LISTS> MultiRefArr;

public:
	LvlBuilder(physics::IPhysicsCooking* pPhysCooking, core::MemoryArenaBase* arena);
	~LvlBuilder();

	bool init(void);

	bool LoadFromMap(mapFile::XMapFile* map);
	bool ProcessModels(void);

	bool save(const char* filename);

private:
	int32_t FindFloatPlane(const Planef& plane);

	bool processMapEntity(LvlEntity& ent, mapFile::XMapEntity* mapEnt);
	bool processBrush(LvlEntity& ent, mapFile::XMapBrush* brush, size_t entIdx);
	bool processPatch(LvlEntity& ent, mapFile::XMapPatch* brush, size_t entIdx);

	bool removeDuplicateBrushPlanes(LvlBrush& pBrush);

private:	
	void calculateLvlBounds(void);

	bool ProcessModel(LvlEntity& ent);
	bool ProcessWorldModel(LvlEntity& ent);

private:
	int SelectSplitPlaneNum(bspNode* node, bspFace* faces);
	void BuildFaceTree_r(bspNode* node, bspFace* faces, size_t& numLeafs);

private:
	bool PutPrimitivesInAreas(LvlEntity& ent);
	
	void PutWindingIntoAreas_r(LvlEntity& ent, Winding* pWinding,
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

	StringTableType stringTable_;

	//	BSPData		data_;
	XPlaneSet	planes_;
	AABB		mapBounds_;
	Vec3f		blockSize_;
	
	mapFile::XMapFile* pMap_;
	physics::IPhysicsCooking* pPhysCooking_;
	lvl::ModelCache* pModelCache_;
	lvl::MatManager matMan_;

	LvlStats stats_;
};

#endif

X_NAMESPACE_END

#endif // !X_LVL_BUILDER_H_