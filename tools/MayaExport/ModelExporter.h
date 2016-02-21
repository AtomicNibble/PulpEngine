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

	MayaBone* findJointReal(const char* pName);

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

	Hierarchy<MayaBone>		mayaHead_;
	Hierarchy<MayaBone>		exportHead_;

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
