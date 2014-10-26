#pragma once

#include <maya\MPxFileTranslator.h>
#include <maya\MPxCommand.h>
#include <maya\MDagPathArray.h>


#include "Hierarchy.h"

#include <Containers\Array.h>
#include <String\StackString.h>
#include <String\Path.h>
#include <Math\XAabb.h>

#include <IModel.h>


#include <Memory\BoundsCheckingPolicies\NoBoundsChecking.h>
#include <Memory\MemoryTaggingPolicies\NoMemoryTagging.h>
#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>

typedef core::MemoryArena<core::MallocFreeAllocator, core::SingleThreadPolicy,
	core::NoBoundsChecking, core::NoMemoryTracking, core::NoMemoryTagging> Arena;


extern core::MallocFreeAllocator g_allocator;
extern Arena g_arena;


class MFnDagNode;

struct ModelStats
{
	ModelStats() :
		droppedBoneNames(&g_arena)
	{
	}

	void clear(void) {
		core::zero_this(this);
		droppedBoneNames.setArena(&g_arena);
		droppedBoneNames.setGranularity(16);
	}

	uint32_t totalLods;
	uint32_t totalMesh;
	uint32_t totalJoints;
	uint32_t totalJointsDropped;
	uint32_t totalVerts;
	uint32_t totalFaces;
	uint32_t totalWeightsDropped;

	core::Array<core::StackString<128>>
		droppedBoneNames;
};
extern ModelStats g_stats;

struct LODExportInfo
{
	int idx;
	float distance;
	MString objects;
};

class PotatoOptions
{
public:
	PotatoOptions() :
		lodInfo_(&g_arena)
	{

	}

	enum ExpoMode {
		EXPORT_ALL,
		EXPORT_SELECTED,
		EXPORT_INPUT
	};

	void setcmdArgs(const MArgList &args);
	void reset(void);
	void setMode(MPxFileTranslator::FileAccessMode mode) {
		switch (mode) {
		case MPxFileTranslator::kExportAccessMode:
		case MPxFileTranslator::kSaveAccessMode:
			exportMode_ = EXPORT_ALL;
			break;
		default:
			exportMode_ = EXPORT_SELECTED;
			break;
		}
	}

	void setFileName(MString path) {
		filePath_.append(path.asChar());
		filePath_.setExtension(".model");
	}

	void AddLodInfo(const LODExportInfo& info) {
		lodInfo_.append(info);
	}

	size_t numLods(void) const {
		return lodInfo_.size();
	}

public:
	core::Array<LODExportInfo> lodInfo_;

	core::Path filePath_;
	float scale_;
	float jointThreshold_;
	bool  zeroOrigin_;
	bool  whiteVertColors_;
	MString forceBoneFilters_;
	MString progressCntl_;
	ExpoMode exportMode_;
};


struct MayaBone
{
	MayaBone() : dagnode(nullptr) {
		mayaNode.setOwner(this);
		exportNode.setOwner(this);

		keep = false;
		bindpos = Vec3f::zero();
		bindm33 = Matrix33f::identity();
	}

	MayaBone(const MayaBone& oth)  {
		name = oth.name;
		dagnode = oth.dagnode;
		index = oth.index;

		bindpos = oth.bindpos;
		bindm33 = oth.bindm33;

		mayaNode = oth.mayaNode;
		exportNode = oth.exportNode;

		keep = oth.keep;

		mayaNode.setOwner(this);
		exportNode.setOwner(this);
	}

	MayaBone& operator=(const MayaBone &oth) {
		name = oth.name;
		dagnode = oth.dagnode;
		index = oth.index;

		bindpos = oth.bindpos;
		bindm33 = oth.bindm33;

		mayaNode = oth.mayaNode;
		exportNode = oth.exportNode;

		keep = oth.keep;

		mayaNode.setOwner(this);
		exportNode.setOwner(this);
		return *this;
	}

	size_t getDataSize() const {
		return name.length() + sizeof(XQuatCompressedf) + sizeof(Vec3f)+2;
	}

	core::StackString<128> name;
	MFnDagNode* dagnode;

	uint32_t index;
	uint32_t exportIdx;

	Vec3f		bindpos;
	Matrix33f	bindm33;

	Hierarchy<MayaBone> mayaNode;
	Hierarchy<MayaBone> exportNode;

	bool keep;
};

struct MayaWeight {
	MayaBone* bone;
	float weight;
};

struct MayaVertex {
	MayaVertex() : numWeights(0), startWeightIdx(0) {}

	Vec3f		pos;
	Vec3f		normal;
	Vec3f		tangent;
	Vec3f		binormal;
	Vec2f		uv;
	Color		col;

	int32		startWeightIdx;
	int32		numWeights;
};



class MayaMesh
{
public:
	MayaMesh()  :
		verts(&g_arena),
		faces(&g_arena),
		weights(&g_arena)
	{
		core::zero_object(CompBinds);
	}
	void clear(void);
	void merge(MayaMesh *mesh); // merge other mesh.
	void shareVerts(void);
	void calBoundingbox(void);

public:
	core::StackString<60> name;
	core::StackString<60> displayName;
	core::StackString<64> material;

	AABB boundingBox;

	model::CompbindInfo CompBinds;

	// vert, tris, and uv's baby!
	core::Array<MayaVertex>		verts;
	core::Array<Vec3<int>>		faces;
	core::Array<MayaWeight>		weights;

	bool						hasBinds;
};

class MayaModel;
class MayaLOD
{
public:
	MayaLOD();
	~MayaLOD();

	void setModel(MayaModel* model, int idx) { pModel = model; lodIdx_ = idx; }
	void calBoundingbox(void);
	void pruneBones(void);

	size_t numMeshes(void) const { return meshes_.size(); }
	size_t getSubDataSize(const Flags8<model::StreamType>& streams);

	MStatus LoadMeshes(void);
	void MergeMeshes(void);

	uint32_t totalVerts(void) const {
		uint32_t total = 0;
		for (auto m : meshes_)
			total += safe_static_cast<uint32_t,size_t>(m->verts.size());
		return total;
	}
	uint32_t totalIndexs(void) const {
		uint32_t total = 0;
		for (auto m : meshes_)
			total += safe_static_cast<uint32_t, size_t>(m->faces.size());
		return total * 3;
	}

public:
	MDagPathArray			exportObjects_;
	core::Array<MayaMesh*>	meshes_;
	AABB					boundingBox;
	MayaModel*				pModel;
	int						lodIdx_;
};

class MayaModel
{
public:
	MayaModel();
	~MayaModel();

	void getBindPose(MObject &jointNode, MayaBone *bone, float scale);

	MStatus lodLODs(void);
	MStatus loadBones(void);
	void pruneBones(void);
	void MergeMeshes(void);

	void printStats(PotatoOptions& options);
	void calculateBoundingBox(void);
	uint32_t calculateTagNameDataSize(void);
	uint32_t calculateMaterialNameDataSize(void);
	uint32_t calculateSubDataSize(const Flags8<model::StreamType>& streams);
	uint32_t calculateBoneDataSize(void);

	MayaBone *findJointReal(const char *name);

public:
	size_t totalMeshes(void) const {
		size_t i, size = 0;
		for (i = 0; i < 4; i++)
			size += lods_[i].numMeshes();
		return size;
	}

public:
	bool save(const char *filename);

public:
	MayaLOD					lods_[4];
	core::Array<MayaBone>	bones_;
	
	int						numExportJoints_;
	MayaBone				tagOrigin_;

	Hierarchy<MayaBone>		mayaHead;
	Hierarchy<MayaBone>		exportHead;

	AABB					boundingBox;
};


class PotatoExporter
{
public:
	PotatoExporter();
	~PotatoExporter();

	MStatus convert();

	static MStatus ShowProgressDlg();
	static MStatus HideProgressDlg();
	static void SetProgressText(MString str);
private:
	MStatus getExportObjects(void);
	MStatus getInputObjects(void);
	

private:
	MayaModel  model_;

	static bool s_progressActive;
};


class ModelExporter : public MPxFileTranslator
{
public:
	ModelExporter();
	~ModelExporter();

	MStatus writer(const MFileObject& file, const MString& optionsString, FileAccessMode mode);

	bool haveWriteMethod() const;
	MString defaultExtension() const;

	static void* creator();

};


class ModelExporterCmd : public MPxCommand
{
public:
	ModelExporterCmd();
	~ModelExporterCmd();

	virtual MStatus doIt(const MArgList &args);

	static void* creator();
};
