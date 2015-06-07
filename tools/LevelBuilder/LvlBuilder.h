#pragma once

#ifndef X_LVL_BUILDER_H_
#define X_LVL_BUILDER_H_

#include "MaterialManager.h"
#include "LvlTypes.h"

class LvlBuilder
{
	typedef core::Array<mapfile::XMapEntity*> MapEntArr;
public:
	LvlBuilder();
	~LvlBuilder();

	bool LoadFromMap(mapfile::XMapFile* map);
	bool ProcessModels(void);

	bool save(const char* filename);

private:
	int32_t FindFloatPlane(const Planef& plane);

	bool processMapEntity(LvlEntity& ent, mapfile::XMapEntity* mapEnt);
	bool processBrush(LvlEntity& ent, mapfile::XMapBrush* brush, int32_t entIdx);
	bool processPatch(LvlEntity& ent, mapfile::XMapPatch* brush, int32_t entIdx);

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

private:
	LvlEntsArr	entities_;
	LvlAreaArr	areas_;

	core::GrowingStringTableUnique<256, 16, 4, uint32_t> stringTable_;

	//	BSPData		data_;
	XPlaneSet	planes;
	AABB		mapBounds;
	Vec3f		blockSize_;
	
	mapfile::XMapFile* map_;

	lvl::MatManager matMan_;

	LvlStats stats_;
};


#endif // !X_LVL_BUILDER_H_