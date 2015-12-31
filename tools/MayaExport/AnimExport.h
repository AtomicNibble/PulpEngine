#pragma once

#include <maya\MPxFileTranslator.h>
#include <maya\MPxCommand.h>
#include <maya\MDagPathArray.h>
#include <maya\MDagPath.h>
#include <maya\MVector.h>
#include <maya\MQuaternion.h>

#include <String\Path.h>

#include <Containers\Array.h>


struct FrameData
{
	MVector position;
	MVector scale;
	MQuaternion rotation;
};

struct Bone
{
	Bone();
	~Bone();

	MDagPath dag;
	MString name;
	core::Array<FrameData> data;
};

class PotatoAnimExporter
{
public:
	PotatoAnimExporter();
	~PotatoAnimExporter();

	MStatus convert(const MArgList &args);

private:
	MStatus getInputObjects(void);
	MStatus getExportObjects(void);
	MStatus loadBones(void);
	MStatus getAnimationData(void);

	MStatus writeIntermidiate(void);

	MStatus processArgs(const MArgList &args);

private:
	const int32_t getNumFrames(void) const {
		return endFrame_ - startFrame_;
	}

private:
	bool intermidiate_;

	int32_t startFrame_;
	int32_t endFrame_;
	uint32_t fps_;
	MString nodes_;

	core::Path<char> fileName_;
	core::Path<char> filePath_;

	MDagPathArray exportObjects_;
	core::Array<Bone> bones_;
};



class AnimExporter : public MPxFileTranslator
{
public:
	AnimExporter();
	~AnimExporter();

	MStatus writer(const MFileObject& file, const MString& optionsString, FileAccessMode mode);

	bool haveWriteMethod() const;
	MString defaultExtension() const;

	static void* creator();

};


class AnimExporterCmd : public MPxCommand
{
public:
	AnimExporterCmd();
	~AnimExporterCmd();

	virtual MStatus doIt(const MArgList &args);

	static void* creator(void);
};
