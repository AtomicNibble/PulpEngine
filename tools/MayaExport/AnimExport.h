#pragma once

#include <maya\MPxFileTranslator.h>
#include <maya\MPxCommand.h>
#include <maya\MDagPathArray.h>
#include <maya\MDagPath.h>
#include <maya\MVector.h>
#include <maya\MQuaternion.h>

#include <IAnimation.h>

#include <anim_inter.h>

X_NAMESPACE_BEGIN(maya)

class PotatoAnimExporter : public anim::Inter::Anim
{
    X_DECLARE_ENUM(ExpoMode)
    (SERVER, RAW);

    typedef core::Array<MDagPath> MDagPathArr;

public:
    PotatoAnimExporter(core::MemoryArenaBase* arena);
    ~PotatoAnimExporter();

    MStatus convert(const MArgList& args);

private:
    void setFileName(const MString& path);
    void setOutdir(const MString& path);

    core::Path<wchar_t> getFilePath(void) const;
    core::string getName(void) const;

    MStatus getInputObjects(void);
    MStatus getExportObjects(void);
    MStatus loadBones(void);
    MStatus getAnimationData(void);
    MStatus loadNoteData(void);

    MStatus processArgs(const MArgList& args);
    MString argsToJson(void) const;

    const int32_t getNumFrames(void) const;

private:
    double convertUnitOfMeasure(double value) const;
    Vec3d convertUnitOfMeasure(const Vec3d& vec) const;
    Vec3f convertUnitOfMeasure(const Vec3f& vec) const;

private:
    int32_t startFrame_;
    int32_t endFrame_;
    anim::AnimType::Enum type_;
    MString nodes_;
    MString noteTrack_;

    core::string name_;
    core::Path<wchar_t> fileName_; // name + extension.
    core::Path<wchar_t> outDir_;

    ExpoMode::Enum exportMode_;
    MDistance::Unit unitOfMeasurement_;
    MDagPathArray exportObjects_;

    MDagPathArr bonePaths_;
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

    virtual MStatus doIt(const MArgList& args);

    static void* creator(void);
};

X_NAMESPACE_END