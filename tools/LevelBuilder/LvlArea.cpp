#include "stdafx.h"
#include "LvlArea.h"
#include "LvlEntity.h"

X_NAMESPACE_BEGIN(lvl)

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
	for (const auto& it : areaMeshes)
	{
		const auto& subMesh = it.second;

		model::SubMeshHeader mesh;
		mesh.boundingBox.clear();

		for (const auto& vert : subMesh.verts_) {
			mesh.boundingBox.add(vert.pos);
		}

		mesh.boundingSphere = Sphere(mesh.boundingBox);
		mesh.numIndexes = safe_static_cast<uint16_t, size_t>(subMesh.faces_.size() * 3);
		mesh.numVerts = safe_static_cast<uint16_t, size_t>(subMesh.verts_.size());
		mesh.startIndex = safe_static_cast<uint32_t, size_t>(model.faces.size()  * 3);
		mesh.startVertex = safe_static_cast<uint32_t, size_t>(model.verts.size());
		mesh.streamsFlag = model::StreamType::COLOR | model::StreamType::NORMALS;

		mesh.materialName = subMesh.matNameID_;
		
		X_LOG1("SubMesh", "Mat: ^3%s^7 verts: %i indexs: %i", 
			subMesh.matName_.c_str(), mesh.numVerts, mesh.numIndexes);

		// faces
		model.faces.append(subMesh.faces_);
		// add verts
		model.verts.append(subMesh.verts_);
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

void LvlArea::addWindingForSide(const XPlaneSet& planes, const LvlBrushSide& side, Winding* pWinding)
{
	const auto& plane = planes[side.planenum];

	AreaSubMesh* pSubMesh = meshForSide(side);

	const size_t startVert = pSubMesh->verts_.size();
	const size_t numPoints = pWinding->getNumPoints();
	const model::Index offset = safe_static_cast<model::Index>(startVert);

	for (size_t i = 2; i < numPoints; i++)
	{
		for (size_t j = 0; j < 3; j++)
		{
			level::Vertex vert;

			if (j == 0) {
				const Vec5f vec = pWinding->at(0);
				vert.pos = vec.asVec3();
				vert.texcoord[0] = Vec2f(vec.s, vec.t);
			}
			else if (j == 1) {
				const Vec5f vec = pWinding->at(i - 1);
				vert.pos = vec.asVec3();
				vert.texcoord[0] = Vec2f(vec.s, vec.t);
			}
			else
			{
				const Vec5f vec = pWinding->at(i);
				vert.pos = vec.asVec3();
				vert.texcoord[0] = Vec2f(vec.s, vec.t);
			}

			// copy normal
			vert.normal = plane.getNormal();
			vert.color = Col_White;

			pSubMesh->AddVert(vert);
		}

		model::Index localOffset = safe_static_cast<model::Index>((i - 2) * 3);

		pSubMesh->faces_.emplace_back(
			offset + localOffset + 0_ui16,
			offset + localOffset + 1_ui16,
			offset + localOffset + 2_ui16
		);
	}
}



AreaSubMesh* LvlArea::MeshForSide(const LvlBrushSide& side, StringTableType& stringTable)
{
	auto it = areaMeshes.find(X_CONST_STRING(side.matInfo.name.c_str()));
	if (it != areaMeshes.end()) {
		return &it->second;
	}
	// add new.
	AreaSubMesh newMesh;

	// add mat name to string table.
	newMesh.matNameID_ = stringTable.addStringUnqiue(side.matInfo.name.c_str());
	newMesh.matName_ = side.matInfo.name;

	auto newIt = areaMeshes.insert(
		AreaMeshMap::value_type(core::string(side.matInfo.name.c_str()), newMesh)
	);

	return &newIt.first->second;
}

AreaSubMesh* LvlArea::MeshForMat(const core::string& matName, StringTableType& stringTable)
{
	auto it = areaMeshes.find(matName);
	if (it != areaMeshes.end()) {
		return &it->second;
	}
	// add new.
	AreaSubMesh newMesh;

	// add mat name to string table.
	newMesh.matNameID_ = stringTable.addStringUnqiue(matName);
	newMesh.matName_ = MaterialName(matName.c_str());

	auto newIt = areaMeshes.insert(
		AreaMeshMap::value_type(matName, newMesh)
	);

	return &newIt.first->second;
}

AreaSubMesh* LvlArea::meshForSide(const LvlBrushSide& side)
{
	auto it = areaMeshes.find(X_CONST_STRING(side.matInfo.name.c_str()));
	if (it != areaMeshes.end()) {
		return &it->second;
	}

	AreaSubMesh newMesh;
	newMesh.matNameID_ = 0;
	newMesh.matName_ = side.matInfo.name;

	core::string matStr(side.matInfo.name.begin(), side.matInfo.name.end());

	auto newIt = areaMeshes.insert({ matStr, newMesh });

	return &newIt.first->second;
}

X_NAMESPACE_END