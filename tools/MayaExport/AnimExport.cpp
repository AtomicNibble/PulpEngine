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


Bone::Bone() : data(g_arena)
{

}

Bone::~Bone()
{

}


// ================================================

PotatoAnimExporter::PotatoAnimExporter() : 
	intermidiate_(false),
	fps_(anim::ANIM_DEFAULT_FPS),
	type_(anim::AnimType::RELATIVE),
	bones_(g_arena)
{
	MayaUtil::MayaPrintMsg("=========== Exporting Anim ===========");
}


PotatoAnimExporter::~PotatoAnimExporter()
{
	MayaUtil::MayaPrintMsg("================= End =================");
	MayaUtil::HideProgressDlg();
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
			MVector position;

			size_t i, num = bones_.size();
			for (i = 0; i < num; i++)
			{
				Bone& bone = bones_[i];

				MayaUtil::MayaPrintVerbose("Bone(%" PRIuS "): %s", i, bone.name.asChar());


				MTransformationMatrix worldMatrix = bone.dag.inclusiveMatrix();
				FrameData& data = bone.data.AddOne();

				data.position.x = worldMatrix.asMatrix()(3, 0);
				data.position.y = worldMatrix.asMatrix()(3, 1);
				data.position.z = worldMatrix.asMatrix()(3, 2);

				// rotation.
				double qx, qy, qz, qw;
				worldMatrix.getRotationQuaternion(qx, qy, qz, qw);

				data.rotation = MQuaternion(qx, qy, qz, qw);

				data.scale = MVector::one;


				if(MayaUtil::IsVerbose())
				{
					MEulerRotation euler = data.rotation.asEulerRotation();
					const MQuaternion& q = data.rotation;

					MayaUtil::MayaPrintVerbose("pos: (%g,%g,%g)",
						data.position.x, data.position.y, data.position.z);
					MayaUtil::MayaPrintVerbose("ang: (%g,%g,%g) quat: (%g,%g,%g,%g)",
						::toDegrees(euler.x), ::toDegrees(euler.y), ::toDegrees(euler.z),
						q.x, q.y, q.z, q.w);
				}
			}
		}
		MayaUtil::IncProcess();
		MayaUtil::MayaPrintVerbose("=========================");

		curFrame++;
	}

	return MS::kSuccess;
}

MStatus PotatoAnimExporter::writeIntermidiate(void)
{
	PROFILE_MAYA_NAME("Write intermidiate:");

	core::Array<uint8_t> anim(g_arena);

	MStatus status = writeIntermidiate_int(anim);

	if (status)
	{
		if (exportMode_ == ExpoMode::SERVER)
		{
			// compress the data.
			core::Array<uint8_t> compressed(g_arena);
			bool unChanged = false;


			core::Compression::Compressor<core::Compression::LZ4> comp;
			if (!comp.deflate(g_arena, anim, compressed, core::Compression::CompressLevel::HIGH))
			{
				X_ERROR("Anim", "Failed to defalte inter anim");
				return MS::kFailure;
			}

			// hellllo asset server :D
			status = maya::AssetDB::Get()->UpdateAsset(maya::AssetDB::AssetType::ANIM,
				MString(fileName_.c_str()),
				argsToJson(),
				compressed,
				&unChanged
			);

			if (!status) {
				X_ERROR("Anim", "Failed to connect to server to update AssetDB");
				return status;
			}
		}
		else
		{
			// write the file.
			FILE* f;
			errno_t err = fopen_s(&f, filePath_.c_str(), "wb");
			if (f)
			{
				fwrite(anim.begin(), 1, anim.size(), f);
				fclose(f);
			}
			else
			{
				MayaUtil::MayaPrintError("Failed to open file for saving(%" PRIi32 "): %s", err, filePath_.c_str());
				return MS::kFailure;
			}
		}
	}

	return status;
}

MStatus PotatoAnimExporter::writeIntermidiate_int(core::Array<uint8_t>& anim)
{
	// I will store each tags data.
	// for each frame there will be a position and a quat.
	const int32_t numBones = safe_static_cast<int32_t, size_t>(bones_.size());
	const int32_t numFrames = getNumFrames();
	const int32_t fps = static_cast<int32_t>(fps_);

	MString cuirrentFile = MFileIO::currentFile();

	// work out max possible size bufffer needed.
	const size_t maxFloatBytes = 24; // -6.4
	const size_t numFloatsPerEntry = 3 + 3 + 4;
	const size_t tagSizes = sizeof("POS ") + sizeof("SCALE ") + sizeof("ANG ");
	const size_t paddingSize = 10 * 6; // more than needed 
	const size_t maxSizePerEntry = (maxFloatBytes * numFloatsPerEntry) +
		tagSizes + paddingSize;

	const size_t headerSize = 8096; // plenty for shiz
	const size_t requiredSize = ((maxSizePerEntry * numFrames) * numBones) + headerSize;

	core::ByteStream stream(g_arena);
	stream.reserve(requiredSize);

	core::StackString<4096> buf;
	buf.clear();
	buf.appendFmt("// " X_ENGINE_NAME " intermidiate animation format\n");
	buf.appendFmt("// Source: \"%s\"\n", cuirrentFile.asChar());
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

		for (int32_t i = 0; i < numFrames; i++)
		{
			const FrameData& data = bone.data[i];

			buf.clear();
			buf.append("POS ");
			buf.appendFmt("( %f %f %f )\n",
				data.position.x,
				data.position.y,
				data.position.z);

			buf.append("SCALE ");
			buf.appendFmt("( %.4g %.4g %.4g )\n",
				data.scale.x,
				data.scale.y,
				data.scale.z);

			buf.append("ANG ");
			buf.appendFmt("( %.8g %.8g %.4g %.8g )\n",
				data.rotation.x,
				data.rotation.y,
				data.rotation.z,
				data.rotation.w);

			buf.append("\n");

			stream.write(buf.c_str(), buf.length());
		}

		MayaUtil::IncProcess();
	}

	anim.resize(stream.size());
	memcpy(anim.ptr(), stream.begin(), stream.size());
	return MS::kSuccess;
}



MStatus PotatoAnimExporter::convert(const MArgList &args)
{
	MStatus status = processArgs(args);
	if (status == MS::kFailure) {
		return status;
	}

	int32_t numFrames = getNumFrames();

	MayaUtil::MayaPrintMsg("Exporting to: '%s'", filePath_.c_str());
	MayaUtil::MayaPrintMsg("Frames -> start: %" PRIi32 " end: %" PRIi32 " num: %" PRIi32, startFrame_, endFrame_, numFrames);
	MayaUtil::MayaPrintMsg(""); // new line

	// print the error after printing info about frames and the name.
	if (numFrames < 1) {
		MayaUtil::MayaPrintError("Can't export a animation with less than one frame");
		return MS::kFailure;
	}

	// name length check
	if (strlen(filePath_.fileName()) > anim::ANIM_MAX_NAME_LENGTH)
	{
		MayaUtil::MayaPrintError("Anim name is too long. MAX: %" PRIu32 ", provided: %" PRIuS,
			anim::ANIM_MAX_NAME_LENGTH, filePath_.length());
		return MS::kFailure;;
	}


	status = MayaUtil::ShowProgressDlg(0, 4);

	if (!status) {
		MayaUtil::MayaPrintError("Failed to create progress window: %s", status.errorString().asChar());
		return status;
	}
	// one is two yet not one.
	// 
	{
		PROFILE_MAYA_NAME("Total Export time:");

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

		if (intermidiate_)
		{
			// we dump everything.
			status =  writeIntermidiate();
			if (!status) {
				MayaUtil::MayaPrintError("Failed to write intermidiate file: %s", status.errorString().asChar());
				return status;
			}
		}
		else
		{
			X_ASSERT_NOT_IMPLEMENTED();
		}
	}


	return status;
}

MStatus PotatoAnimExporter::processArgs(const MArgList &args)
{
	MStatus status = MS::kFailure;
	MString Mfilename;
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

	status = args.get(++idx, Mfilename);
	if (!status) {
		MayaUtil::MayaPrintError("missing filename");
		return status;
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
			filePath_.append(dir.asChar());
			filePath_.replaceSeprators();
			filePath_.ensureSlash();
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

	intermidiate_ = true;

	if (Mfilename.length() > 0) 
	{
		fileName_.set(Mfilename.asChar());
		filePath_.append(Mfilename.asChar());

		if (intermidiate_) {
			filePath_.setExtension(anim::ANIM_INTER_FILE_EXTENSION);
		}
		else {
			filePath_.setExtension(anim::ANIM_FILE_EXTENSION);
		}

		status = MS::kSuccess;
	}

	return status;
}

MString PotatoAnimExporter::argsToJson(void) const
{
	core::json::StringBuffer s;
	core::json::Writer<core::json::StringBuffer> writer(s);

	writer.SetMaxDecimalPlaces(5);

	writer.StartObject();
	writer.Key("verbose");
	writer.Bool(MayaUtil::IsVerbose());
	writer.Key("mode");
	writer.String(ExpoMode::ToString(exportMode_));
	writer.Key("type");
	writer.String(anim::AnimType::ToString(type_));
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
	PotatoAnimExporter animExport;

	return animExport.convert(args);
}

void* AnimExporterCmd::creator(void)
{
	return new AnimExporterCmd;
}


X_NAMESPACE_END