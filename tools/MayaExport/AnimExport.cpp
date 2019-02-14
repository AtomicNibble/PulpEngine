#include "stdafx.h"
#include "AnimExport.h"
#include "MayaUtil.h"

#include <IAnimation.h>
#include <IFileSys.h>

#include <maya/MArgList.h>
#include <maya\MSelectionList.h>
#include <maya\MStringArray.h>
#include <maya\MDagPath.h>
#include <maya\MItDag.h>
#include <maya\MMatrix.h>
#include <maya\MTransformationMatrix.h>
#include <maya\MVector.h>
#include <maya\MQuaternion.h>
#include <maya\MAnimControl.h>
#include <maya\MAnimUtil.h>
#include <maya\MPlug.h>
#include <maya\MFnEnumAttribute.h>
#include <maya\MTime.h>
#include <maya\MFnDagNode.h>
#include <maya\MEulerRotation.h>
#include <maya\MFileIO.h>
#include <maya\MGlobal.h>

#include <Compression\LZ4.h>

#include "Profiler.h"
#include "AssetDB.h"

X_NAMESPACE_BEGIN(maya)

// ================================================

PotatoAnimExporter::PotatoAnimExporter(core::MemoryArenaBase* arena) :
    anim::Inter::Anim(arena),
    type_(anim::AnimType::RELATIVE),
    unitOfMeasurement_(MDistance::Unit::kInches),
    bonePaths_(arena)
{
    MayaUtil::MayaPrintMsg("=========== Exporting Anim ===========");
}

PotatoAnimExporter::~PotatoAnimExporter()
{
    MayaUtil::MayaPrintMsg("================= End =================");
    MayaUtil::HideProgressDlg();
}

MStatus PotatoAnimExporter::convert(const MArgList& args)
{
    MStatus status = processArgs(args);
    if (status == MS::kFailure) {
        return status;
    }

    int32_t numFrames = getNumFrames();

    MayaUtil::MayaPrintMsg("Exporting: '%s'", fileName_.c_str());
    MayaUtil::MayaPrintMsg("Frames -> start: %" PRIi32 " end: %" PRIi32 " num: %" PRIi32, startFrame_, endFrame_, numFrames);
    MayaUtil::MayaPrintMsg(""); // new line

    // print the error after printing info about frames and the name.
    if (numFrames < 1) {
        MayaUtil::MayaPrintError("Can't export a animation with less than one frame");
        return MS::kFailure;
    }

    // name length check
    if (name_.length() > anim::ANIM_MAX_NAME_LENGTH) {
        MayaUtil::MayaPrintError("Anim name is too long. MAX: %" PRIu32 ", provided: %" PRIuS,
            anim::ANIM_MAX_NAME_LENGTH, name_.length());
        return MS::kFailure;
    }

    status = MayaUtil::ShowProgressDlg(0, 4);

    if (!status) {
        MayaUtil::MayaPrintError("Failed to create progress window: %s", status.errorString().asChar());
        return status;
    }

    PROFILE_MAYA_NAME("Total Export time:");

    {
        MayaUtil::SetProgressText("Loading export list");

        status = getExportObjects();

        if (!status) {
            MayaUtil::MayaPrintError("Failed to collect export objects from maya: %s", status.errorString().asChar());
            return status;
        }

        MayaUtil::SetProgressRange(0, 5 + getNumFrames());
        MayaUtil::SetProgressText("Loading bones");

        status = loadBones();

        if (!status) {
            MayaUtil::MayaPrintError("Failed to get bones: %s", status.errorString().asChar());
            return status;
        }

        MayaUtil::SetProgressText("Loading bone data");

        status = getAnimationData();

        if (!status) {
            MayaUtil::MayaPrintError("Failed to get animation data: %s", status.errorString().asChar());
            return status;
        }

        MayaUtil::SetProgressText("Loading notetrack data");

        status = loadNoteData();

        if (!status) {
            MayaUtil::MayaPrintError("Failed to get notetrack data: %s", status.errorString().asChar());
            return status;
        }
    }

    MString currentFile = MFileIO::currentFile();
    setSourceInfo(core::string(currentFile.asChar()), startFrame_, endFrame_);

    if (exportMode_ == ExpoMode::RAW) {
        PROFILE_MAYA_NAME("Save Raw");

        auto filePath = getFilePath();
        MayaUtil::MayaPrintMsg("Exporting to: '%s'", filePath.c_str());

        if (!saveOS(filePath)) {
            MayaUtil::MayaPrintError("Failed to save file: %s", filePath.c_str());
            return MS::kFailure;
        }
    }
    else if (exportMode_ == ExpoMode::SERVER) {
        MayaUtil::SetProgressText("Getting info from server");

        maya::AssetDB::ConverterInfo info;
        if (!maya::AssetDB::Get()->GetConverterInfo(info)) {
            X_ERROR("Anim", "Failed to get info from server");
            return status;
        }

        MString name(getName().data(), static_cast<int32_t>(getName().length()));

        int32_t assetId, modId;
        status = maya::AssetDB::Get()->AssetExsists(maya::AssetDB::AssetType::ANIM, name, &assetId, &modId);
        if (!status) {
            X_ERROR("Anim", "Failed to get meta from server");
            return status;
        }

        if (assetId == assetDb::INVALID_ASSET_ID || modId == assetDb::INVALID_MOD_ID) {
            X_ERROR("Anim", "Asset is not registerd with server");
            return MS::kFailure;
        }

        maya::AssetDB::Mod mod;
        if (!maya::AssetDB::Get()->GetModInfo(modId, mod)) {
            X_ERROR("Anim", "Failed to get mod info from server");
            return status;
        }

        core::ByteStream interMidiateData(g_arena);

        {
            PROFILE_MAYA_NAME("Write intermidiate:");

            if (!save(interMidiateData)) {
                return status;
            }
        }

        MayaUtil::SetProgressText("Deflating");

        core::Array<uint8_t> compressed(g_arena);
        {
            core::Compression::Compressor<core::Compression::LZ4> comp;
            if (!comp.deflate(g_arena, core::make_span<const char>(interMidiateData.begin(), interMidiateData.end()), compressed, core::Compression::CompressLevel::HIGH)) {
                X_ERROR("Anim", "Failed to defalte inter anim");
                return MS::kFailure;
            }
            else {
                core::HumanSize::Str sizeStr, sizeStr2;
                X_LOG0("Anim", "Defalated %s -> %s",
                    core::HumanSize::toString(sizeStr, interMidiateData.size()),
                    core::HumanSize::toString(sizeStr2, compressed.size()));
            }
        }

        {
            MayaUtil::SetProgressText("Syncing data to AssetServer");
            bool unChanged = false;

            // hellllo asset server :D
            status = maya::AssetDB::Get()->UpdateAsset(maya::AssetDB::AssetType::ANIM,
                name,
                argsToJson(),
                compressed,
                &unChanged);

            if (!status) {
                X_ERROR("Anim", "Failed to connect to server to update AssetDB");
                return status;
            }
        }
    }
    else {
        X_ASSERT_NOT_IMPLEMENTED();
    }

    return status;
}

void PotatoAnimExporter::setFileName(const MString& path)
{
    // we don't replace seperators on the name as asset names
    // have a fixed slash, regardless of native slash of the platform.
    // so we replace slashes only when building file paths.
    fileName_.set(path.asWChar());
    fileName_.trim();

    name_ = core::string(path.asChar(), path.length());
    name_.trim();

    fileName_.setExtension(anim::ANIM_FILE_EXTENSION_W);
}

void PotatoAnimExporter::setOutdir(const MString& path)
{
    outDir_.set(path.asWChar());
    outDir_.trim();
    outDir_.replaceSeprators();
}

core::Path<wchar_t> PotatoAnimExporter::getFilePath(void) const
{
    core::Path<wchar_t> path(outDir_);
    path /= fileName_;
    path.replaceSeprators();
    return path;
}

const core::string& PotatoAnimExporter::getName(void) const
{
    return name_;
}

MStatus PotatoAnimExporter::getInputObjects(void)
{
    // we just want to load all nodes with animations
    uint32_t x;
    MStatus status = MS::kSuccess;

    MDagPathArray pathArry;
    MSelectionList list;
    MStringArray nameArr;

    nodes_.split(' ', nameArr);

    for (x = 0; x < nameArr.length(); x++) {
        list.add(nameArr[x]);
    }

    for (x = 0; x < list.length(); x++) {
        MDagPath path;
        status = list.getDagPath(x, path);
        if (status) {
            pathArry.append(path);
        }
        else {
            MayaUtil::MayaPrintError("getDagPath failed: %s", status.errorString().asChar());
            return MS::kFailure;
        }
    }

    if (pathArry.length() < 1) {
        return MS::kFailure;
    }

    exportObjects_ = pathArry;
    return MS::kSuccess;
}

MStatus PotatoAnimExporter::getExportObjects(void)
{
    PROFILE_MAYA();

    return getInputObjects();
}

MStatus PotatoAnimExporter::loadBones(void)
{
    MDagPath jointDag;

    uint32_t num = exportObjects_.length();
    size_t numFrames = getNumFrames();

    numFrames_ = safe_static_cast<int32_t>(numFrames);
    bonePaths_.reserve(num);
    bones_.reserve(num);

    for (uint32_t i = 0; i < num; i++) {
        jointDag = exportObjects_[i];

        if (jointDag.hasFn(MFn::kJoint)) {
            MFnDagNode node(jointDag);

            MayaUtil::MayaPrintMsg("Bone(%" PRIu32 ") name: %s", i, node.name().asChar());

            auto& bone = bones_.AddOne(arena_);
            bone.name = MayaUtil::RemoveNameSpace(node.name()).asChar();
            bone.data.reserve(numFrames);

            bonePaths_.push_back(jointDag);
        }
        else {
            MayaUtil::MayaPrintMsg("Not a valid joint: %" PRIu32, i);
        }
    }

    return MS::kSuccess;
}

MStatus PotatoAnimExporter::getAnimationData(void)
{
    PROFILE_MAYA_NAME("getAnimationData:");

    MTime time;

    // for each frame load anim data.
    int32_t curFrame = startFrame_;

    MayaUtil::MayaPrintMsg("Loading frames: %" PRIi32 " - %" PRIi32, startFrame_, endFrame_);

    while (curFrame <= endFrame_) {
        time.setValue(curFrame);
        MAnimControl::setCurrentTime(time);

        MayaUtil::MayaPrintVerbose("Loading frame: %" PRIi32, curFrame);
        {
            X_ASSERT(bones_.size() == bonePaths_.size(), "Bone size mismatch")(bones_.size(), bonePaths_.size()); 

            size_t i, num = bones_.size();
            for (i = 0; i < num; i++) {
                anim::Inter::Bone& bone = bones_[i];
                const auto& bonePath = bonePaths_[i];

                MayaUtil::MayaPrintVerbose("Bone(%" PRIuS "): %s", i, bone.name.c_str());

                MTransformationMatrix worldMatrixTrans = bonePath.inclusiveMatrix();
                MMatrix m = worldMatrixTrans.asMatrix();

                Matrix33f worldMatrix = MayaUtil::XMat(m);
                Vec3d pos = MayaUtil::XVec<double>(m);
                pos = convertUnitOfMeasure(pos);

                Vec3f scale;
                scale.x = worldMatrix.getColumn(0).length();
                scale.y = worldMatrix.getColumn(1).length();
                scale.z = worldMatrix.getColumn(2).length();

                // rotation.
                // Quatd quat;
                // worldMatrixTrans.getRotation(quat.v.x, quat.v.y, quat.v.z, quat.w);
                Matrix33f scaleMatrix = Matrix33f::createScale(scale);
                Matrix33f invScaleMatrix = scaleMatrix.inverted();
                Matrix33f rotationMatrix = invScaleMatrix * worldMatrix;

                anim::Inter::FrameData& data = bone.data.AddOne();
                data.scale = scale;
                data.position = pos;
                data.rotation = rotationMatrix;

                if (MayaUtil::IsVerbose()) {
                    const auto& s = data.scale;
                    const auto& q = Quatf(data.rotation);
                    const auto euler = q.getEulerDegrees();

                    MayaUtil::MayaPrintVerbose("pos: (%g,%g,%g) scale(%g,%g,%g)",
                        data.position.x, data.position.y, data.position.z, s.x, s.y, s.z);
                    MayaUtil::MayaPrintVerbose("ang: (%g,%g,%g) quat: (%g,%g,%g,%g)",
                        euler.x, euler.y, euler.z,
                        q.v.x, q.v.y, q.v.z, q.w);
                }
            }
        }
        MayaUtil::IncProcess();
        MayaUtil::MayaPrintVerbose("=========================");

        curFrame++;
    }

    return MS::kSuccess;
}

MStatus PotatoAnimExporter::loadNoteData(void)
{
    if (noteTrack_.length() == 0 || noteTrack_ == "None") {
        return MS::kSuccess;
    }

    MayaUtil::MayaPrintMsg("Loading notes from: %s", noteTrack_.asChar());

    // need to basically find a locator with the name.
    MStatus status;

    MItDag it(MItDag::kDepthFirst, MFn::kTransform, &status);
    if (!status) {
        return status;
    }

    if (it.isDone()) {
        return MS::kFailure;
    }

    MDagPath path;
    while (1) {
        status = it.getPath(path);
        if (!status) {
            return status;
        }

        MFnDagNode node(path);
        const auto& name = node.name();
        if (name == noteTrack_) {
            break;
        }

        status = it.next();
        if (!status) {
            return status;
        }

        if (it.isDone()) {
            return MS::kFailure;
        }
    }

    // wwe found it.
    if (!path.isValid()) {
        return MS::kFailure;
    }

    bool animated = MAnimUtil::isAnimated(path, false, &status);
    if (!status) {
        return MS::kFailure;
    }

    if (!animated) {
        MayaUtil::MayaPrintWarning("Notetrack is not animated");
        return MS::kSuccess;
    }

    MPlugArray plugs;
    if (!MAnimUtil::findAnimatedPlugs(path, plugs)) {
        return MS::kFailure;
    }

    if (plugs.length() == 0) {
        MayaUtil::MayaPrintError("Notetrack is animated, but failed to get plugs");
        return MS::kFailure;
    }

    for (uint32_t i = 0; i < plugs.length(); i++) {
        const auto& plug = plugs[i];

        auto fullName = plug.name();
        auto name = plug.partialName();

        if (name != "NoteValue" && name != "MainNote") {
            continue;
        }

        // don't see a nicer way to get this currently :(
        MString cmd = "addAttr -q -enumName ";
        cmd += fullName;

        MString result;
        MGlobal::executeCommand(cmd, result, false, false);

        MStringArray values;
        result.split(':', values);

        MFnAnimCurve curve(plug);

        auto numKeys = curve.numKeys();

        for (uint32_t k = 0; k < numKeys; k++) {
            MTime time = curve.time(k, &status);
            double val = curve.value(k, &status);

            int32_t frame = static_cast<int32_t>(time.value());
            uint32_t idx = static_cast<uint32_t>(val);

            if (idx > values.length()) {
                MayaUtil::MayaPrintError("Notetrack has a invalid value on frame: %" PRIi32, frame);
                return MS::kFailure;
            }

            core::string value(values[idx].asChar());

            if (frame < startFrame_ || frame > endFrame_) {
                MayaUtil::MayaPrintVerbose("Skipping note \"%s\" frame %" PRIi32 " out of range", value.c_str(), frame);
                continue;
            }

            X_ASSERT(frame >= startFrame_, "Frame below start")(frame, startFrame_); 

            anim::Inter::Note note;
            note.value = value;
            note.frame = (frame - startFrame_);

            MayaUtil::MayaPrintMsg("Note: \"%s\" Frame %" PRIi32, note.value.c_str(), note.frame);

            notes_.emplace_back(note);
        }
    }

    return MS::kSuccess;
}

MStatus PotatoAnimExporter::processArgs(const MArgList& args)
{
    MStatus status = MS::kFailure;
    uint32_t idx;

    // args:
    // f: filename
    // Start:
    // End:
    // Nodes:
    // dir:

    idx = args.flagIndex("f");
    if (idx == MArgList::kInvalidArgIndex) {
        MayaUtil::MayaPrintError("missing file argument");
        return MS::kFailure;
    }

    // required
    {
        MString Mfilename;

        status = args.get(++idx, Mfilename);
        if (!status) {
            MayaUtil::MayaPrintError("missing filename");
            return status;
        }

        setFileName(Mfilename);
    }

    idx = args.flagIndex("verbose");
    if (idx != MArgList::kInvalidArgIndex) {
        MayaUtil::SetVerbose(true);
    }
    else {
        MayaUtil::SetVerbose(false);
    }

    idx = args.flagIndex("mode");
    if (idx != MArgList::kInvalidArgIndex) {
        MString modeStr;
        if (!args.get(++idx, modeStr)) {
            MayaUtil::MayaPrintWarning("failed to get export mode");
        }
        else {
            if (core::strUtil::IsEqualCaseInsen(modeStr.asChar(), "Raw")) {
                exportMode_ = ExpoMode::RAW;
            }
            else if (core::strUtil::IsEqualCaseInsen(modeStr.asChar(), "Server")) {
                exportMode_ = ExpoMode::SERVER;
            }
            else {
                MayaUtil::MayaPrintWarning("Unknown export mode: \"%s\"",
                    modeStr.asChar());
            }
        }
    }
    else {
        exportMode_ = ExpoMode::SERVER;
    }

    idx = args.flagIndex("type");
    if (idx != MArgList::kInvalidArgIndex) {
        MString typeStr;
        if (!args.get(++idx, typeStr)) {
            MayaUtil::MayaPrintWarning("failed to get type");
        }
        else {
            static_assert(anim::AnimType::ENUM_COUNT == 4, "More animTypes? this needs updating");

            if (core::strUtil::IsEqualCaseInsen(typeStr.asChar(), "Relative")) {
                type_ = anim::AnimType::RELATIVE;
            }
            else if (core::strUtil::IsEqualCaseInsen(typeStr.asChar(), "Absolute")) {
                type_ = anim::AnimType::ABSOLUTE;
            }
            else if (core::strUtil::IsEqualCaseInsen(typeStr.asChar(), "Additive")) {
                type_ = anim::AnimType::ADDITIVE;
            }
            else if (core::strUtil::IsEqualCaseInsen(typeStr.asChar(), "Delta")) {
                type_ = anim::AnimType::DELTA;
            }
            else {
                MayaUtil::MayaPrintWarning("Unknown type: \"%s\"",
                    typeStr.asChar());
            }
        }
    }
    else {
        type_ = anim::AnimType::RELATIVE;
    }

    idx = args.flagIndex("dir");
    if (idx != MArgList::kInvalidArgIndex) {
        MString dir;
        if (!args.get(++idx, dir)) {
            MayaUtil::MayaPrintWarning("failed to get dir flag");
        }
        else {
            setOutdir(dir);
        }
    }

    idx = args.flagIndex("Start");
    if (idx != MArgList::kInvalidArgIndex) {
        if (!args.get(++idx, startFrame_)) {
            MayaUtil::MayaPrintWarning("failed to get Start frame");
        }
    }

    idx = args.flagIndex("End");
    if (idx != MArgList::kInvalidArgIndex) {
        if (!args.get(++idx, endFrame_)) {
            MayaUtil::MayaPrintWarning("failed to get End frame");
        }
    }

    idx = args.flagIndex("Nodes");
    if (idx != MArgList::kInvalidArgIndex) {
        if (!args.get(++idx, nodes_)) {
            MayaUtil::MayaPrintWarning("failed to get nodes");
        }
    }

    idx = args.flagIndex("notetrack");
    if (idx != MArgList::kInvalidArgIndex) {
        if (!args.get(++idx, noteTrack_)) {
            MayaUtil::MayaPrintWarning("failed to get notetrack");
        }
    }

    MString progressCntl;
    idx = args.flagIndex("progress");
    if (idx != MArgList::kInvalidArgIndex) {
        if (!args.get(++idx, progressCntl)) {
            MayaUtil::MayaPrintWarning("failed to get progress cntl flag");
        }
    }

    MayaUtil::SetProgressCtrl(progressCntl);

    return status;
}

MString PotatoAnimExporter::argsToJson(void) const
{
    core::json::StringBuffer s;
    core::json::Writer<core::json::StringBuffer> writer(s);

    writer.SetMaxDecimalPlaces(5);

    core::StackString<128> typeStr(anim::AnimType::ToString(type_));
    core::StackString<128> modeStr(ExpoMode::ToString(exportMode_));

    typeStr.toLower();
    modeStr.toLower();

    writer.StartObject();
    writer.Key("verbose");
    writer.Bool(MayaUtil::IsVerbose());
    writer.Key("mode");
    writer.String(modeStr.c_str(), safe_static_cast<core::json::SizeType>(modeStr.length()));
    writer.Key("type");
    writer.String(typeStr.c_str(), safe_static_cast<core::json::SizeType>(typeStr.length()));
    writer.Key("start");
    writer.Int(startFrame_);
    writer.Key("end");
    writer.Int(endFrame_);

    writer.EndObject();

    return MString(s.GetString());
}

X_INLINE const int32_t PotatoAnimExporter::getNumFrames(void) const
{
    return (endFrame_ + 1) - startFrame_;
}

X_INLINE double PotatoAnimExporter::convertUnitOfMeasure(double value) const
{
    MDistance d(value);
    return d.as(unitOfMeasurement_);
}

X_INLINE Vec3d PotatoAnimExporter::convertUnitOfMeasure(const Vec3d& vec) const
{
    Vec3d ret;
    ret.x = convertUnitOfMeasure(vec.x);
    ret.y = convertUnitOfMeasure(vec.y);
    ret.z = convertUnitOfMeasure(vec.z);
    return ret;
}

X_INLINE Vec3f PotatoAnimExporter::convertUnitOfMeasure(const Vec3f& vec) const
{
    Vec3f ret;
    ret.x = static_cast<float>(convertUnitOfMeasure(vec.x));
    ret.y = static_cast<float>(convertUnitOfMeasure(vec.y));
    ret.z = static_cast<float>(convertUnitOfMeasure(vec.z));
    return ret;
}

// ======================================================

AnimExporter::AnimExporter()
{
}
AnimExporter::~AnimExporter()
{
}

MStatus AnimExporter::writer(const MFileObject& file, const MString& optionsString, FileAccessMode mode)
{
    X_UNUSED(file);
    X_UNUSED(optionsString);
    X_UNUSED(mode);

    return MS::kFailure;
}

bool AnimExporter::haveWriteMethod() const
{
    return true;
}

MString AnimExporter::defaultExtension() const
{
    return anim::ANIM_INTER_FILE_EXTENSION;
}

void* AnimExporter::creator()
{
    return new AnimExporter;
}

// ======================================================

AnimExporterCmd::AnimExporterCmd()
{
}

AnimExporterCmd::~AnimExporterCmd()
{
}

MStatus AnimExporterCmd::doIt(const MArgList& args)
{
    PotatoAnimExporter animExport(g_arena);

    return animExport.convert(args);
}

void* AnimExporterCmd::creator(void)
{
    return new AnimExporterCmd;
}

X_NAMESPACE_END