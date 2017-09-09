#include "stdafx.h"
#include "AnimExport.h"
#include "MayaUtil.h"

#include <IAnimation.h>

#include <maya/MArgList.h>
#include <maya\MSelectionList.h>
#include <maya\MStringArray.h>
#include <maya\MDagPath.h>
#include <maya\MMatrix.h>
#include <maya\MTransformationMatrix.h>
#include <maya\MVector.h>
#include <maya\MQuaternion.h>
#include <maya\MAnimControl.h>
#include <maya\MTime.h>
#include <maya\MFnDagNode.h>
#include <maya\MEulerRotation.h>
#include <maya\MFileIO.h>

#include <Compression\LZ4.h>

#include "Profiler.h"
#include "AssetDB.h"

X_NAMESPACE_BEGIN(maya)


Bone::Bone() : 
	data(g_arena)
{

}

Bone::~Bone()
{

}


// ================================================

PotatoAnimExporter::PotatoAnimExporter(core::MemoryArenaBase* arena) :
	arena_(arena),
	fps_(anim::ANIM_DEFAULT_FPS),
	type_(anim::AnimType::RELATIVE),
	bones_(arena)
{
	MayaUtil::MayaPrintMsg("=========== Exporting Anim ===========");
}


PotatoAnimExporter::~PotatoAnimExporter()
{
	MayaUtil::MayaPrintMsg("================= End =================");
	MayaUtil::HideProgressDlg();
}


MStatus PotatoAnimExporter::convert(const MArgList &args)
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
	if (name_.length() > anim::ANIM_MAX_NAME_LENGTH)
	{
		MayaUtil::MayaPrintError("Anim name is too long. MAX: %" PRIu32 ", provided: %" PRIuS,
			anim::ANIM_MAX_NAME_LENGTH, name_.length());
		return MS::kFailure;;
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

		MayaUtil::SetProgressRange(0, 4 + getNumFrames() + exportObjects_.length());
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

	}

	core::ByteStream interMidiateData(g_arena);

	{
		PROFILE_MAYA_NAME("Write intermidiate:");

		status = writeIntermidiate_int(interMidiateData);
		if (!status) {
			return status;
		}
	}

	if (exportMode_ == ExpoMode::RAW)
	{
		PROFILE_MAYA_NAME("Save Raw");

		const auto filePath = getFilePath();
		MayaUtil::MayaPrintMsg("Exporting to: '%s'", filePath.c_str());

		FILE* f;
		errno_t err = fopen_s(&f, filePath.c_str(), "wb");
		if (f)
		{
			fwrite(interMidiateData.begin(), 1, interMidiateData.size(), f);
			fclose(f);
		}
		else
		{
			MayaUtil::MayaPrintError("Failed to open file for saving(%" PRIi32 "): %s", err, filePath.c_str());
			return MS::kFailure;
		}
	}
	else if (exportMode_ == ExpoMode::SERVER)
	{
		MayaUtil::SetProgressText("Getting info from server");

		maya::AssetDB::ConverterInfo info;
		if (!maya::AssetDB::Get()->GetConverterInfo(info)) {
			X_ERROR("Anim", "Failed to get info from server");
			return status;
		}

		int32_t assetId, modId;
		status = maya::AssetDB::Get()->AssetExsists(maya::AssetDB::AssetType::ANIM, MString(getName()), &assetId, &modId);
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

		MayaUtil::SetProgressText("Deflating");

		core::Array<uint8_t> compressed(g_arena);
		{

			core::Compression::Compressor<core::Compression::LZ4> comp;
			if (!comp.deflate(g_arena, interMidiateData.begin(), interMidiateData.end(), compressed, core::Compression::CompressLevel::HIGH))
			{
				X_ERROR("Anim", "Failed to defalte inter anim");
				return MS::kFailure;
			}
			else
			{
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
				MString(getName()),
				argsToJson(),
				compressed,
				&unChanged
			);

			if (!status) {
				X_ERROR("Anim", "Failed to connect to server to update AssetDB");
				return status;
			}
		}


		{
			MayaUtil::SetProgressText("Compiling anim");




		}
	}
	else
	{
		X_ASSERT_NOT_IMPLEMENTED();
	}

	return status;
}


void PotatoAnimExporter::setFileName(const MString& path)
{
	// we don't replace seperators on the name as asset names 
	// have a fixed slash, regardless of native slash of the platform.
	// so we replace slashes only when building file paths.

	fileName_.set(path.asChar());
	fileName_.trim();

	name_ = core::string(fileName_.begin(), fileName_.end());

	fileName_.setExtension(anim::ANIM_FILE_EXTENSION);
}

void PotatoAnimExporter::setOutdir(const MString& path)
{
	outDir_.set(path.asChar());
	outDir_.trim();
	outDir_.replaceSeprators();
}

core::Path<char> PotatoAnimExporter::getFilePath(void) const
{
	core::Path<char> path(outDir_);
	path /= fileName_;
	path.replaceSeprators();
	return path;
}


core::string PotatoAnimExporter::getName(void) const
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

	for (x = 0; x < list.length(); x++)
	{
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
	PROFILE_MAYA("get objects");

	return getInputObjects();
}

MStatus PotatoAnimExporter::loadBones(void)
{
	MDagPath jointDag;

	uint32_t num = exportObjects_.length();
	size_t numFrames = getNumFrames();

	bones_.reserve(num);

	for (uint32_t i = 0; i < num; i++)
	{
		jointDag = exportObjects_[i];

		if (jointDag.hasFn(MFn::kJoint))
		{
			MFnDagNode node(jointDag);
			
			MayaUtil::MayaPrintMsg("Bone(%" PRIu32 ") name: %s", i, node.name().asChar());

			Bone& bone = bones_.AddOne();
			bone.dag = jointDag;
			bone.name = MayaUtil::RemoveNameSpace(node.name());
			bone.data.reserve(numFrames);
		}
		else
		{
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

	while (curFrame <= endFrame_)
	{
		time.setValue(curFrame);
		MAnimControl::setCurrentTime(time);

		MayaUtil::MayaPrintVerbose("Loading frame: %" PRIi32, curFrame);
		{
			size_t i, num = bones_.size();
			for (i = 0; i < num; i++)
			{
				Bone& bone = bones_[i];

				MayaUtil::MayaPrintVerbose("Bone(%" PRIuS "): %s", i, bone.name.asChar());

				MTransformationMatrix worldMatrix = bone.dag.inclusiveMatrix();
				
				// don't use this for rotation it's scaled.
				MMatrix scaledMatrix = worldMatrix.asMatrix();
				
				// rotation.
				Quatd quat;
				worldMatrix.getRotationQuaternion(quat.v.x, quat.v.y, quat.v.z, quat.w);

				FrameData& data = bone.data.AddOne();
				data.scale = Vec3f::one();
				data.position = MayaUtil::XVec(scaledMatrix);
				data.rotation = quat;


				if(MayaUtil::IsVerbose())
				{
					const auto& q = data.rotation;
					const auto euler = q.getEulerDegrees();
					
					MayaUtil::MayaPrintVerbose("pos: (%g,%g,%g)",
						data.position.x, data.position.y, data.position.z);
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

MStatus PotatoAnimExporter::writeIntermidiate_int(core::ByteStream& stream)
{
	// I will store each tags data.
	// for each frame there will be a position and a quat.
	const int32_t numBones = safe_static_cast<int32_t>(bones_.size());
	const int32_t numFrames = getNumFrames();
	const int32_t fps = static_cast<int32_t>(fps_);

	MString currentFile = MFileIO::currentFile();

	// work out max possible size bufffer needed.
	const size_t maxFloatBytes = 24; // -6.4
	const size_t numFloatsPerEntry = 3 + 3 + 4;
	const size_t tagSizes = sizeof("POS ") + sizeof("SCALE ") + sizeof("ANG ");
	const size_t paddingSize = 10 * 6; // more than needed 
	const size_t maxSizePerEntry = (maxFloatBytes * numFloatsPerEntry) +
		tagSizes + paddingSize;

	const size_t headerSize = 8096; // plenty for shiz
	const size_t requiredSize = ((maxSizePerEntry * numFrames) * numBones) + headerSize;

	stream.reset();
	stream.reserve(requiredSize);

	core::StackString<4096> buf;
	buf.clear();
	buf.appendFmt("// " X_ENGINE_NAME " intermidiate animation format\n");
	buf.appendFmt("// Source: \"%s\"\n", currentFile.asChar());
	buf.appendFmt("// TimeLine range: %" PRIi32 " <-> %" PRIi32 "\n", startFrame_, endFrame_);
	buf.appendFmt("\n");
	buf.appendFmt("VERSION %" PRIu32 "\n", anim::ANIM_INTER_VERSION);
	buf.appendFmt("BONES %" PRIi32 "\n", numBones);
	buf.appendFmt("FRAMES %" PRIi32 "\n", numFrames);
	buf.appendFmt("FPS %" PRIi32 "\n", fps);
	buf.appendFmt("\n");

	// list the bones.
	for (const auto& bone : bones_) {
		const char* pName = bone.name.asChar();
		buf.appendFmt("BONE \"%s\"\n", pName);
	}

	buf.append("\n");

	stream.write(buf.c_str(), buf.length());
	buf.clear();

	// write each bones data.
	for (const auto& bone : bones_)
	{
		buf.append("BONE_DATA\n");
		stream.write(buf.c_str(), buf.length());

		X_ASSERT(static_cast<int32_t>(bone.data.size()) == numFrames, "Don't have bone data for all frames")(bone.data.size(), numFrames);

		for (int32_t i = 0; i < numFrames; i++)
		{
			const FrameData& data = bone.data[i];

			buf.clear();
			buf.appendFmt("POS ( %f %f %f )\n",
				data.position.x,
				data.position.y,
				data.position.z);

			buf.appendFmt("SCALE ( %.4g %.4g %.4g )\n",
				data.scale.x,
				data.scale.y,
				data.scale.z);

			buf.appendFmt("ANG ( %.8g %.8g %.8g %.8g )\n",
				data.rotation.v.x,
				data.rotation.v.y,
				data.rotation.v.z,
				data.rotation.w);
		
			buf.append("\n");

			stream.write(buf.c_str(), buf.length());
		}

		MayaUtil::IncProcess();
	}

	return MS::kSuccess;
}


MStatus PotatoAnimExporter::processArgs(const MArgList &args)
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
	return "anim";
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

MStatus AnimExporterCmd::doIt(const MArgList &args)
{
	PotatoAnimExporter animExport(g_arena);

	return animExport.convert(args);
}

void* AnimExporterCmd::creator(void)
{
	return new AnimExporterCmd;
}


X_NAMESPACE_END