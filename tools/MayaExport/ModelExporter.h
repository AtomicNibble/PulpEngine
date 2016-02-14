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

#include <ModelCompiler.h>

class MFnDagNode;

/*
struct ModelStats
{
	ModelStats() :
		droppedBoneNames(g_arena)
	{
		clear();
	}

	void clear(void) {
		totalLods = 0;
		totalMesh = 0;
		totalJoints = 0;
		totalJointsDropped = 0;
		totalVerts = 0;
		totalFaces = 0;
		totalWeightsDropped = 0;

		droppedBoneNames.setArena(g_arena);
		droppedBoneNames.clear();
		droppedBoneNames.setGranularity(16);

		bounds.clear();
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

	AABB bounds;
};
*/

/*
struct LODExportInfo
{
	int idx;
	float distance;
	MString objects;
};

class PotatoOptions
{
public:
	PotatoOptions();

	enum ExpoMode {
		EXPORT_ALL,
		EXPORT_SELECTED,
		EXPORT_INPUT
	};

	enum UnitOfMeasureMent {
		INCHES,
		CM
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

	void setFileName(const MString& path)
	{
		core::StackString<512,char> temp(path.asChar());
		temp.trim();

		filePath_.append(temp.c_str());
		filePath_.setExtension(model::MODEL_FILE_EXTENSION);
	}

	void AddLodInfo(const LODExportInfo& info) {
		lodInfo_.append(info);
	}

	size_t numLods(void) const {
		return lodInfo_.size();
	}

public:
	core::FixedArray<LODExportInfo, model::MODEL_MAX_LODS> lodInfo_;

	core::Path<char> filePath_;
	float scale_;
	float jointThreshold_;
	float uvMergeThreshold_;
	float vertThreshold_;
	bool  zeroOrigin_;
	bool  whiteVertColors_;
	bool  _pad[2];
	MString forceBoneFilters_;
	ExpoMode exportMode_;
	UnitOfMeasureMent unitOfMeasurement_;
};
*/
#if 0

struct MayaBone
{
	MayaBone();
	MayaBone(const MayaBone& oth);

	MayaBone& operator=(const MayaBone &oth);

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

struct MayaWeight 
{
	MayaWeight();

	MayaBone* bone;
	float weight;
};

struct MayaVertex 
{
	MayaVertex();

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
	MayaMesh();
	~MayaMesh();

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

	void getBindPose(const MObject &jointNode, MayaBone *bone, float scale);

	MStatus lodLODs(void);
	MStatus loadBones(void);
	void pruneBones(PotatoOptions& options);
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

#endif

struct MayaBone
{
	MayaBone();
	MayaBone(const MayaBone& oth);

	MayaBone& operator=(const MayaBone &oth);

	size_t getDataSize() const {
		return name.length() + sizeof(XQuatCompressedf) + sizeof(Vec3f) + 2;
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

class ModelExporter : public model::ModelCompiler
{
	struct LODExportInfo
	{
		int idx;
		float distance;
		MString objects;
		MDagPathArray exportObjects;
	};

	X_DECLARE_ENUM(ExpoMode)(EXPORT_ALL, EXPORT_SELECTED, EXPORT_INPUT);
	X_DECLARE_ENUM(UnitOfMeasureMent)(INCHES, CM);

public:
	ModelExporter(core::V2::JobSystem* pJobSys, core::MemoryArenaBase* arena);
	~ModelExporter();

	MStatus convert(const MArgList& args);
	void printStats(void) const;

private:
	void setFileName(const MString& path);
	void setOutdir(const MString& path);

	core::Path<char> getFilePath(void) const;

	MStatus parseArgs(const MArgList& args);
	MStatus loadLODs(void);
	MStatus loadBones(void);

private:
	MStatus getExportObjects(void);
	MStatus getInputObjects(void);

private:
	static MFnDagNode* GetParentBone(MFnDagNode* pBone);
	static MStatus getBindPose(const MObject &jointNode, MayaBone* pBone, float scale);
	static core::StackString<60> getMeshDisplayName(const MString& fullname);
	static bool getMeshMaterial(MDagPath& dagPath, model::RawModel::Material& material);
	static MObject FindShader(MObject& setNode);

private:
	typedef core::FixedArray<LODExportInfo, model::MODEL_MAX_LODS> LodInfoArr;

	core::Path<char> fileName_;
	core::Path<char> outDir_;
	ExpoMode::Enum exportMode_;
	UnitOfMeasureMent::Enum unitOfMeasurement_;

	core::Array<MayaBone>	mayaBones_;
	MayaBone				tagOrigin_;

	Hierarchy<MayaBone>		mayaHead;
	Hierarchy<MayaBone>		exportHead;

	LodInfoArr lodExpoInfo_;
};


class ModelExporterCmd : public MPxCommand
{
public:
	ModelExporterCmd();
	~ModelExporterCmd();

	virtual MStatus doIt(const MArgList &args);

	static void* creator();
};
