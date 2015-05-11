#pragma once

#ifndef X_LVL_BUILDER_H_
#define X_LVL_BUILDER_H_


#include "LvlTypes.h"

class LvlBuilder
{
	typedef core::Array<LvlEntity> LvlEntsArr;
	typedef core::Array<LvlArea> LvlAreaArr;
public:
	LvlBuilder();

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
	void MakeStructuralFaceList(LvlEntity& ent);
	void calculateLvlBounds(void);
		
	bool ProcessModel(LvlEntity& ent);
	bool ProcessWorldModel(LvlEntity& ent);

private:
	LvlEntsArr	entities_;
	LvlAreaArr	areas_;

	core::GrowingStringTableUnique<256, 16, 4, uint32_t> stringTable_;

	//	BSPData		data_;
	XPlaneSet	planes;
	AABB		mapBounds;
	Vec3f		blockSize_;

	struct Stats
	{
		Stats()  {
			core::zero_this(this);
		}
		int32_t	numEntities;
		int32_t	numPatches;
		int32_t	numBrushes;
		int32_t	numAreaPortals;
		int32_t	numFaceLeafs;
	};

	Stats stats_;
};



#endif // !X_LVL_BUILDER_H_