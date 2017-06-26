#pragma once

#include <String\GrowingStringTable.h>

X_NAMESPACE_BEGIN(lvl)

struct LvlBrushSide;

typedef core::GrowingStringTableUnique<256, 16, 4, uint32_t> StringTableType;

// contains all the collision data for a area.
// we will support tri mesh and hieght fields.
struct AreaCollsiion
{
	// a single chunck of collision data.
	struct TriMeshData
	{
		TriMeshData(core::MemoryArenaBase* arena);

		bool cook(physics::IPhysicsCooking* pCooking);

		X_INLINE void AddVert(const level::Vertex& vert) {
			verts.append(vert);
		}
		X_INLINE void AddFace(const model::Face& face) {
			faces.append(face);
		}

		core::Array<level::Vertex> verts;
		core::Array<model::Face> faces;
		core::Array<uint8_t> cookedData;
	};

	// a collection of collision data for a given set of group flags.
	struct GroupBucket
	{
		typedef core::Array<TriMeshData> TriMesgDataArr;

		GroupBucket(physics::GroupFlags groupFlags, core::MemoryArenaBase* arena);

		bool cook(physics::IPhysicsCooking* pCooking);

		physics::GroupFlags getGroupFlags(void) const;
		const TriMesgDataArr& getTriMeshDataArr(void) const;

		TriMeshData& getCurrentTriMeshData(void);
		void beginNewTriMesh(void); // move to a new block of data, used to break collision data up into smaller chuncks for potential performance gains.

	private:
		physics::GroupFlags groupFlags_;
		TriMesgDataArr triMeshData_;
	};

	typedef core::Array<GroupBucket> ColGroupBucketArr;


public:
	AreaCollsiion(core::MemoryArenaBase* arena);

	bool cook(physics::IPhysicsCooking* pCooking);

	size_t numGroups(void) const;
	const ColGroupBucketArr& getGroups(void) const;

	GroupBucket& getBucket(physics::GroupFlags flags);

private:
	ColGroupBucketArr colGroupBuckets_;
};

struct AreaModel
{
	AreaModel();

	bool BelowLimits(void);
	void BeginModel(void);
	void EndModel(void);

	core::Array<model::SubMeshHeader> meshes;
	core::Array<level::Vertex> verts;
	core::Array<model::Face> faces;

	model::MeshHeader model;
};

// used to build up submeshes.
// so faces with same materials are grouped into meshes.
struct AreaSubMesh
{
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
	core::Array<level::Vertex> verts_;
	core::Array<model::Face> faces_;
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
