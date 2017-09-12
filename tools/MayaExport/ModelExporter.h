#pragma once

#include <maya\MPxFileTranslator.h>
#include <maya\MPxCommand.h>
#include <maya\MDagPathArray.h>
#include <maya\MDistance.h>


#include "Hierarchy.h"

#include <Containers\Array.h>
#include <String\StackString.h>
#include <String\Path.h>
#include <Math\XAabb.h>

#include <IModel.h>

#include <RawModel.h>

class MFnDagNode;

struct MayaBone
{
	X_NO_COPY(MayaBone);
	X_NO_ASSIGN(MayaBone);
public:
	MayaBone();
	MayaBone(MayaBone&& oth);

	MayaBone& operator=(MayaBone&& oth);

	size_t getDataSize() const {
		return name.length() + sizeof(XQuatCompressedf) + sizeof(Vec3f) + 2;
	}

	core::StackString<128> name;
	core::UniquePointer<MFnDagNode> dagnode;

	uint32_t index;

	Vec3f		scale;
	Vec3f		bindpos;
	Matrix33f	bindRotation;

	Hierarchy<MayaBone> mayaNode;
};

class ModelExporter : public model::RawModel::Model
{
	struct LODExportInfo
	{
		int idx;
		float distance;
		MString objects;
		MDagPathArray exportObjects;
	};

	X_DECLARE_ENUM(ExpoMode)(SERVER, RAW);
	X_DECLARE_ENUM(MeshExpoMode)(EXPORT_ALL, EXPORT_SELECTED, EXPORT_INPUT);

	typedef core::StackString<60> MeshNameStr;
	typedef core::FixedArray<LODExportInfo, model::MODEL_MAX_LODS> LodInfoArr;

public:
	ModelExporter(core::V2::JobSystem* pJobSys, core::MemoryArenaBase* arena);
	~ModelExporter();

	MStatus convert(const MArgList& args);
	void printStats(void) const;

private:
	void setFileName(const MString& path);
	void setOutdir(const MString& path);

	core::Path<char> getFilePath(void) const;
	const core::string& getName(void) const;

	MStatus parseArgs(const MArgList& args);
	MStatus loadLODs(void);
	MStatus loadBones(void);

private:
	MStatus getExportObjects(void);
	MStatus getInputObjects(void);

	MayaBone* findJointReal(const char* pName);

private:
	double convertUnitOfMeasure(double value) const;
	Vec3d convertUnitOfMeasure(const Vec3d& vec) const;
	Vec3f convertUnitOfMeasure(const Vec3f& vec) const;

private:
	MStatus getBindPose(MayaBone& bone) const;
	static void getLocalIndex(MIntArray& getVertices, MIntArray& getTriangle, core::FixedArray<uint32_t, 8>& indexOut);
	static core::UniquePointer<MFnDagNode> getParentBone(MFnDagNode* pBone);
	static MeshNameStr getMeshDisplayName(const MString& fullname);
	static bool getMeshMaterial(MDagPath& dagPath, model::RawModel::Material& material);
	static MObject findShader(const MObject& setNode);

private:
	float scale_;
	core::string name_;
	core::Path<char> fileName_;
	core::Path<char> outDir_;
	ExpoMode::Enum exportMode_;
	MeshExpoMode::Enum meshExportMode_;
	MDistance::Unit unitOfMeasurement_;

	core::Array<MayaBone>	mayaBones_;
	MayaBone				tagOrigin_;

	Hierarchy<MayaBone>		mayaHead_;

	LodInfoArr lodExpoInfo_;
};


class ModelExporterCmd : public MPxCommand
{
public:
	ModelExporterCmd();
	~ModelExporterCmd();

	virtual MStatus doIt(const MArgList &args) X_FINAL;
	virtual bool hasSyntax(void) const X_FINAL;

	static MSyntax newSyntax();
	static void* creator();
};
