#pragma once

#include <maya\MPxFileTranslator.h>
#include <maya\MPxCommand.h>
#include <maya\MDagPathArray.h>
#include <maya\MDagPath.h>
#include <maya\MVector.h>
#include <maya\MQuaternion.h>

#include <String\Path.h>

#include <Containers\Array.h>

#include <IAnimation.h>

X_NAMESPACE_BEGIN(maya)


struct FrameData
{
	Vec3f scale;
	Vec3d position;
	Matrix33f rotation;
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
	X_DECLARE_ENUM(ExpoMode)(SERVER, RAW);

public:
	PotatoAnimExporter(core::MemoryArenaBase* arena);
	~PotatoAnimExporter();

	MStatus convert(const MArgList &args);

private:
	void setFileName(const MString& path);
	void setOutdir(const MString& path);

	core::Path<char> getFilePath(void) const;
	core::string getName(void) const;

	MStatus getInputObjects(void);
	MStatus getExportObjects(void);
	MStatus loadBones(void);
	MStatus getAnimationData(void);

	MStatus writeIntermidiate_int(core::ByteStream& stream);

	MStatus processArgs(const MArgList &args);
	MString argsToJson(void) const;

private:
	X_INLINE const int32_t getNumFrames(void) const {
		return (endFrame_ + 1) - startFrame_;
	}

private:
	double convertUnitOfMeasure(double value) const;
	Vec3d convertUnitOfMeasure(const Vec3d& vec) const;
	Vec3f convertUnitOfMeasure(const Vec3f& vec) const;

private:
	core::MemoryArenaBase* arena_;

	int32_t startFrame_;
	int32_t endFrame_;
	uint32_t fps_;
	anim::AnimType::Enum type_;
	MString nodes_;

	core::string name_;
	core::Path<char> fileName_; // name + extension.
	core::Path<char> outDir_;

	ExpoMode::Enum exportMode_;
	MDistance::Unit unitOfMeasurement_;
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

X_NAMESPACE_END