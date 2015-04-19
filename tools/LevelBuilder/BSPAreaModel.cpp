#include "stdafx.h"
#include "BSPTypes.h"



AreaModel::AreaModel() :
meshes(g_arena),
verts(g_arena),
indexes(g_arena)
{

}


bool AreaModel::BelowLimits()
{
	if (meshes.size() > bsp::MAP_MAX_MODEL_SURFACES)
	{
		X_ERROR("Bsp", "too many surfaces on worldmodel. num: %i max: %i",
			meshes.size(), bsp::MAP_MAX_MODEL_SURFACES);
		return false;
	}
	if (verts.size() > bsp::MAP_MAX_MODEL_VERTS)
	{
		X_ERROR("Bsp", "too many verts for worldmodel. num: %i max: %i",
			verts.size(), bsp::MAP_MAX_MODEL_VERTS);
		return false;
	}
	if (indexes.size() > bsp::MAP_MAX_MODEL_INDEXES)
	{
		X_ERROR("Bsp", "too many indexes for worldmodel. num: %i max: %i",
			indexes.size(), bsp::MAP_MAX_MODEL_INDEXES);
		return false;
	}

	return true;
}

void AreaModel::BeginModel(const LvlEntity& ent)
{
	meshes.setGranularity(4096);
	verts.setGranularity(4096);
	indexes.setGranularity(4096);
}

void AreaModel::EndModel()
{
	model.streamsFlag = model::StreamType::NORMALS | model::StreamType::COLOR;
	model.numSubMeshes = safe_static_cast<uint32_t, size_t>(meshes.size());
	model.numVerts = safe_static_cast<uint32_t, size_t>(verts.size());
	model.numIndexes = safe_static_cast<uint32_t, size_t>(indexes.size() * 3);

	// build bounds for all the meshes.
	// this could be done in parrell.
	AABB bounds;

	core::Array<model::SubMeshHeader>::ConstIterator it = meshes.begin();
	for (; it != meshes.end(); ++it)
	{
		bounds.add(it->boundingBox);
	}

	model.boundingBox = bounds;
	model.boundingSphere = Sphere(bounds);

	X_LOG_BULLET;
	X_LOG0("AreaModel", "num verts: %i", model.numVerts);
	X_LOG0("AreaModel", "num indexes: %i", model.numIndexes);
	X_LOG0("AreaModel", "num meshes: %i", model.numSubMeshes);
	X_LOG0("AreaModel", "bounds: (%.0f,%.0f,%.0f) to (%.0f,%.0f,%.0f)", bounds.min[0], bounds.min[1], bounds.min[2],
		bounds.max[0], bounds.max[1], bounds.max[2]);
}
