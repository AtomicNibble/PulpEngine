#include "stdafx.h"
#include "LvlTypes.h"



AreaCollsiion::TriMeshData::TriMeshData(core::MemoryArenaBase* arena) :
	verts(arena),
	faces(arena),
	cookedData(arena)
{

}

bool AreaCollsiion::TriMeshData::cook(physics::IPhysicsCooking* pCooking)
{
	using namespace physics;
	
	IPhysicsCooking::CookFlags flags;

	static_assert(sizeof(decltype(faces)::Type::value_type) == 2, "No longer using 16bit indicies?");
	flags.Set(IPhysicsCooking::CookFlag::INDICES_16BIT);

	TriangleMeshDesc desc;
	desc.points.pData = verts.data();
	desc.points.stride = sizeof(decltype(verts)::Type);
	desc.points.count = safe_static_cast<uint32>(verts.size());
	desc.triangles.pData = faces.data();
	desc.triangles.stride = sizeof(decltype(faces)::Type);
	desc.triangles.count = safe_static_cast<uint32>(faces.size());

	if (!pCooking->cookTriangleMesh(desc, cookedData, flags))
	{
		X_ERROR("TriMesh", "Failed to cook area collision");
		return false;
	}

	if (cookedData.size() > level::MAP_MAX_AREA_COL_DATA_SIZE)
	{
		X_ERROR("TriMesh", "cooked mesh is too big: %" PRIuS " bytes. max: %" PRIu32, cookedData.size(), level::MAP_MAX_AREA_COL_DATA_SIZE);
		return false;
	}

	return true;
}

// ==========================================


AreaCollsiion::GroupBucket::GroupBucket(physics::GroupFlags groupFlags, core::MemoryArenaBase* arena) :
	groupFlags_(groupFlags),
	triMeshData_(arena)
{

}

bool AreaCollsiion::GroupBucket::cook(physics::IPhysicsCooking* pCooking)
{
	for (auto& tri : triMeshData_)
	{
		if (!tri.cook(pCooking)) {
			return false;
		}
	}

	return true;
}

physics::GroupFlags AreaCollsiion::GroupBucket::getGroupFlags(void) const
{
	return groupFlags_;
}

const AreaCollsiion::GroupBucket::TriMesgDataArr& AreaCollsiion::GroupBucket::getTriMeshDataArr(void) const
{
	return triMeshData_;
}


AreaCollsiion::TriMeshData& AreaCollsiion::GroupBucket::getCurrentTriMeshData(void)
{
	if (triMeshData_.isEmpty()) {
		beginNewTriMesh();
	}

	return triMeshData_.back();
}

void AreaCollsiion::GroupBucket::beginNewTriMesh(void)
{
	triMeshData_.emplace_back(triMeshData_.getArena());
}

// ==========================================

AreaCollsiion::AreaCollsiion(core::MemoryArenaBase* arena) :
	colGroupBuckets_(arena)
{

}

bool AreaCollsiion::cook(physics::IPhysicsCooking* pCooking)
{
	for (auto& b : colGroupBuckets_)
	{
		if (!b.cook(pCooking)) {
			return false;
		}
	}

	return true;
}

size_t AreaCollsiion::numGroups(void) const
{
	return colGroupBuckets_.size();
}

const AreaCollsiion::ColGroupBucketArr& AreaCollsiion::getGroups(void) const
{
	return colGroupBuckets_;
}

AreaCollsiion::GroupBucket& AreaCollsiion::getBucket(physics::GroupFlags flags)
{
	for (auto& b : colGroupBuckets_)
	{
		if (b.getGroupFlags() == flags)
		{
			return b;
		}
	}

	// add a new one :O
	colGroupBuckets_.emplace_back(flags, colGroupBuckets_.getArena());

	return colGroupBuckets_.back();
}

// ----------------------------


AreaModel::AreaModel() :
	meshes(g_arena),
	verts(g_arena),
	faces(g_arena)
{

}


bool AreaModel::BelowLimits(void)
{
	if (meshes.size() > level::MAP_MAX_MODEL_SURFACES)
	{
		X_ERROR("AreaModel", "too many surfaces on AreaModel. num: %i max: %i",
			meshes.size(), level::MAP_MAX_MODEL_SURFACES);
		return false;
	}
	if (verts.size() > level::MAP_MAX_MODEL_VERTS)
	{
		X_ERROR("AreaModel", "too many verts for AreaModel. num: %i max: %i",
			verts.size(), level::MAP_MAX_MODEL_VERTS);
		return false;
	}
	if ((faces.size()*3) > level::MAP_MAX_MODEL_INDEXES)
	{
		X_ERROR("AreaModel", "too many indexes for AreaModel. num: %i max: %i",
			faces.size() * 3, level::MAP_MAX_MODEL_INDEXES);
		return false;
	}

	return true;
}

void AreaModel::BeginModel(void)
{
	meshes.setGranularity(4096);
	verts.setGranularity(4096);
	faces.setGranularity(4096);
}

void AreaModel::EndModel(void)
{
	model.streamsFlag = model::StreamType::NORMALS | model::StreamType::COLOR;
	model.numSubMeshes = safe_static_cast<uint32_t, size_t>(meshes.size());
	model.numVerts = safe_static_cast<uint32_t, size_t>(verts.size());
	model.numIndexes = safe_static_cast<uint32_t, size_t>(faces.size() * 3);

	// build bounds for all the meshes.
	// this could be done in parrell.
	AABB bounds;

	bounds.clear();

	core::Array<model::SubMeshHeader>::ConstIterator it = meshes.begin();
	for (; it != meshes.end(); ++it)
	{
		bounds.add(it->boundingBox);
	}

	model.boundingBox = bounds;
	model.boundingSphere = Sphere(bounds);

	X_LOG_BULLET;
	X_LOG1("AreaModel", "num verts: %i", model.numVerts);
	X_LOG1("AreaModel", "num indexes: %i", model.numIndexes);
	X_LOG1("AreaModel", "num meshes: %i", model.numSubMeshes);
	X_LOG1("AreaModel", "bounds: (%.0f,%.0f,%.0f) to (%.0f,%.0f,%.0f)", bounds.min[0], bounds.min[1], bounds.min[2],
		bounds.max[0], bounds.max[1], bounds.max[2]);
}


// ----------------------------

AreaSubMesh::AreaSubMesh() : 
	verts_(g_arena), 
	faces_(g_arena)
{
	verts_.setGranularity(4096);
	faces_.setGranularity(4096);
}


// ----------------------------


LvlArea::LvlArea() :
	collision(g_arena),
	areaMeshes(g_arena),
	entRefs(g_arena),
	modelsRefs(g_arena)
{
	areaMeshes.reserve(2048);
	entRefs.reserve(256);
}

void LvlArea::AreaBegin(void)
{
	areaMeshes.clear();

	model.BeginModel();
}

void LvlArea::AreaEnd(void)
{
	AreaMeshMap::const_iterator it = areaMeshes.begin();
	for (; it != areaMeshes.end(); ++it)
	{
		const AreaSubMesh& aSub = it->second;

		model::SubMeshHeader mesh;
		mesh.boundingBox.clear();

		core::Array<level::Vertex>::ConstIterator vertIt = aSub.verts_.begin();
		for (; vertIt != aSub.verts_.end(); ++vertIt) {
			mesh.boundingBox.add(vertIt->pos);
		}

		mesh.boundingSphere = Sphere(mesh.boundingBox);
		mesh.numIndexes = safe_static_cast<uint16_t, size_t>(aSub.faces_.size() * 3);
		mesh.numVerts = safe_static_cast<uint16_t, size_t>(aSub.verts_.size());
		mesh.startIndex = safe_static_cast<uint32_t, size_t>(model.faces.size()  * 3);
		mesh.startVertex = safe_static_cast<uint32_t, size_t>(model.verts.size());
		mesh.streamsFlag = model::StreamType::COLOR | model::StreamType::NORMALS;

		mesh.materialName = aSub.matNameID_;
		
		X_LOG1("SubMesh", "Mat: ^3%s^7 verts: %i indexs: %i", 
			aSub.matName_.c_str(), mesh.numVerts, mesh.numIndexes);

		// faces
		model.faces.append(aSub.faces_);
		// add verts
		model.verts.append(aSub.verts_);
		// add the mesh
		model.meshes.append(mesh);
	}

	model.EndModel();
	// not needed anymore.
	areaMeshes.clear();

	// copy bounds.
	boundingBox = model.model.boundingBox;
	boundingSphere = model.model.boundingSphere;
}



AreaSubMesh* LvlArea::MeshForSide(const LvlBrushSide& side, StringTableType& stringTable)
{
	AreaMeshMap::iterator it = areaMeshes.find(X_CONST_STRING(side.matInfo.name.c_str()));
	if (it != areaMeshes.end()) {
		return &it->second;
	}
	// add new.
	AreaSubMesh newMesh;

	// add mat name to string table.
	newMesh.matNameID_ = stringTable.addStringUnqiue(side.matInfo.name.c_str());
	newMesh.matName_ = side.matInfo.name;

	std::pair<AreaMeshMap::iterator, bool> newIt = areaMeshes.insert(
		AreaMeshMap::value_type(core::string(side.matInfo.name.c_str()), newMesh)
	);

	return &newIt.first->second;
}

AreaSubMesh* LvlArea::MeshForMat(const core::string& matName, StringTableType& stringTable)
{
	AreaMeshMap::iterator it = areaMeshes.find(matName);
	if (it != areaMeshes.end()) {
		return &it->second;
	}
	// add new.
	AreaSubMesh newMesh;

	// add mat name to string table.
	newMesh.matNameID_ = stringTable.addStringUnqiue(matName);
	newMesh.matName_ = core::StackString<level::MAP_MAX_MATERIAL_LEN>(matName.c_str());

	std::pair<AreaMeshMap::iterator, bool> newIt = areaMeshes.insert(
		AreaMeshMap::value_type(matName, newMesh)
	);

	return &newIt.first->second;
}