#pragma once

#include <String\GrowingStringTable.h>

X_NAMESPACE_BEGIN(lvl)

struct LvlBrushSide;

typedef core::GrowingStringTableUnique<256, 16, 4, uint32_t> StringTableType;


// Data for a single TriangleMesh. it's one mesh once cooked.
struct ColTriMeshData
{
	typedef core::Array<uint8_t> DataArr;
	typedef core::Array<level::Vertex> VertArr;
	typedef core::Array<model::Face> FacesArr;

public:
	ColTriMeshData(core::MemoryArenaBase* arena);

	bool cook(physics::IPhysicsCooking* pCooking);

	X_INLINE void addVert(const level::Vertex& vert) {
		verts.append(vert);
	}
	X_INLINE void addFace(const model::Face& face) {
		faces.append(face);
	}

public:
	VertArr verts;
	FacesArr faces;
	DataArr cookedData;
};


// a collision group bucket, stores all the physics data for a given physics group.
// so for a area we may have multiple collision buckets.
struct ColGroupBucket
{
	typedef core::Array<ColTriMeshData> ColTriMesgDataArr;
	typedef core::Array<AABB> AABBArr;

public:
	ColGroupBucket(physics::GroupFlags groupFlags, core::MemoryArenaBase* arena);

	bool cook(physics::IPhysicsCooking* pCooking);

	X_INLINE void addAABB(const AABB& bounds) {
		aabbData_.append(bounds);
	}

	physics::GroupFlags getGroupFlags(void) const;
	const ColTriMesgDataArr& getTriMeshDataArr(void) const;
	const AABBArr& getAABBData(void) const;

	ColTriMeshData& getCurrentTriMeshData(void);
	void beginNewTriMesh(void); // starts a new mesh.

private:
	physics::GroupFlags groupFlags_;

	ColTriMesgDataArr triMeshData_;
	AABBArr aabbData_;
};

// contains all the collision data for a area.
struct AreaCollsiion
{
	typedef core::Array<uint8_t> DataArr;
	typedef core::Array<ColGroupBucket> ColGroupBucketArr;

public:
	AreaCollsiion(core::MemoryArenaBase* arena);

	bool cook(physics::IPhysicsCooking* pCooking);

	size_t numGroups(void) const;
	const ColGroupBucketArr& getGroups(void) const;

	ColGroupBucket& getBucket(physics::GroupFlags flags);

private:
	ColGroupBucketArr colGroupBuckets_;
};

struct AreaModel
{
	typedef core::Array<model::SubMeshHeader> MeshArr;
	typedef core::Array<level::Vertex> VertArr;
	typedef core::Array<model::Face> FacesArr;
public:
	AreaModel();

	void writeToStream(core::ByteStream& stream) const;
	size_t serializeSize(void) const;


	bool belowLimits(void);
	void beginModel(void);
	void endModel(void);

	MeshArr meshes;
	VertArr verts;
	FacesArr faces;

	model::MeshHeader meshHeader;
};

// used to build up submeshes.
// so faces with same materials are grouped into meshes.
struct AreaSubMesh
{
	typedef core::Array<level::Vertex> VertArr;
	typedef core::Array<model::Face> FacesArr;

public:
	AreaSubMesh();

	X_INLINE void AddVert(const level::Vertex& vert) {
		verts_.append(vert);
	}
	X_INLINE void AddFace(const model::Face& face) {
		faces_.append(face);
	}

public:
	MaterialName matName_;
	uint32_t matNameID_;

	// index's for this sub mesh.
	// merged into AreaModel list at end.
	VertArr verts_;
	FacesArr faces_;
};


class LvlArea
{
	typedef core::HashMap<core::string, AreaSubMesh> AreaMeshMap;
	typedef core::Array<uint32_t> AreaRefs;

public:
	LvlArea();

	void AreaBegin(void);
	void AreaEnd(void);
	
	void addWindingForSide(const XPlaneSet& planes, const LvlBrushSide& side, Winding* pWinding);

	AreaSubMesh* MeshForSide(const LvlBrushSide& side, StringTableType& stringTable);
	AreaSubMesh* MeshForMat(const core::string& matName, StringTableType& stringTable);

private:
	AreaSubMesh* meshForSide(const LvlBrushSide& side);


public:
	// area has one model.
	AreaModel model;
	AreaCollsiion collision;

	AreaMeshMap areaMeshes;
	AreaRefs entRefs;
	AreaRefs modelsRefs;

	// copy of the model values.
	AABB boundingBox;
	Sphere boundingSphere;
};


X_NAMESPACE_END
