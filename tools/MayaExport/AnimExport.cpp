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


#include "Profiler.h"



Bone::Bone() : data(g_arena)
{

}

Bone::~Bone()
{

}


// ================================================

PotatoAnimExporter::PotatoAnimExporter() : 
	fps_(anim::ANIM_DEFAULT_FPS),
	intermidiate_(false),
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
			
			MayaUtil::MayaPrintMsg("Bone(%i) name: %s", i, node.name().asChar());

			Bone& bone = bones_.AddOne();
			bone.dag = jointDag;
			bone.name = node.name();
			bone.data.reserve(numFrames);
		}
		else
		{
			MayaUtil::MayaPrintMsg("Not a valid joint: %i", i);
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

	MayaUtil::MayaPrintMsg("Loading frames: %i - %i", startFrame_, endFrame_);

	while (curFrame <= endFrame_)
	{
		time.setValue(curFrame);
		MAnimControl::setCurrentTime(time);

		MayaUtil::MayaPrintMsg("Loading frame: %i", curFrame);
		{
			MVector position;

			size_t i, num = bones_.size();
			for (i = 0; i < num; i++)
			{
				Bone& bone = bones_[i];

				MayaUtil::MayaPrintMsg("Bone(%i): %s", i, bone.name.asChar());


				MTransformationMatrix worldMatrix = bone.dag.inclusiveMatrix();
				FrameData& data = bone.data.AddOne();

				data.position.x = worldMatrix.asMatrix()(3, 0);
				data.position.y = worldMatrix.asMatrix()(3, 1);
				data.position.z = worldMatrix.asMatrix()(3, 2);

				// rotation.
				double qx, qy, qz, qw;
				worldMatrix.getRotationQuaternion(qx, qy, qz, qw);

				data.rotation = MQuaternion(qx, qy, qz, qw);

#if 0
				{
					MEulerRotation euler = data.rotation.asEulerRotation();
					const MQuaternion& q = data.rotation;

					MayaUtil::MayaPrintMsg("pos: (%g,%g,%g)",
						data.position.x, data.position.y, data.position.z);
					MayaUtil::MayaPrintMsg("ang: (%g,%g,%g) quat: (%g,%g,%g,%g)",
						::toDegrees(euler.x), ::toDegrees(euler.y), ::toDegrees(euler.z),
						q.x, q.y, q.z, q.w);
				}
#endif
			}
		}
		MayaUtil::IncProcess();
		MayaUtil::MayaPrintMsg("=========================");

		curFrame++;
	}

	return MS::kSuccess;
}

MStatus PotatoAnimExporter::writeIntermidiate(void)
{
	PROFILE_MAYA_NAME("Write intermidiate:");
	FILE* f;


	errno_t err = fopen_s(&f, filePath_.c_str(), "wb");
	if (f)
	{
		// going to be writing lots of text out.
		// building the text in memory will be better.
		// how big to make the buffer tho?
		// what about making it big enougth to store any frame.

		// I will store each tags data.
		// for each frame there will be a position and a quat.
		const int32_t numBones = safe_static_cast<int32_t,size_t>(bones_.size());
		const int32_t numFrames = getNumFrames();
		const int32_t fps = static_cast<int32_t>(fps_);

		MString cuirrentFile = MFileIO::currentFile();

		fprintf(f, "// Potato intermidiate animation format\n");
		fprintf(f, "// Source: \"%s\"\n", cuirrentFile.asChar());
		fprintf(f, "// TimeLine range: %i <-> %i\n", startFrame_, endFrame_);
		fprintf(f, "\n");
		fprintf(f, "VERSION %i\n", anim::ANIM_INTER_VERSION);
		fprintf(f, "BONES %i\n", numBones);
		fprintf(f, "FRAMES %i\n", numFrames);
		fprintf(f, "FPS %i\n", fps);
		fprintf(f, "\n");

		// list the bones.
		for (const auto& bone : bones_)
		{
			const char* pName = bone.name.asChar();
			fprintf(f, "BONE \"%s\"\n", pName);
		}

		fprintf(f, "\n");

		// work out max possible size bufffer needed.
		const size_t maxFloatBytes = 24; // -6.4
		const size_t numFloatsPerEntry = 3 + 3 + 4;
		const size_t tagSizes = sizeof("POS ") + sizeof("SCALE ") + sizeof("ANG ");
		const size_t paddingSize = 10 * 6; // more than needed 
		const size_t maxSizePerEntry = (maxFloatBytes * numFloatsPerEntry) +
			tagSizes + paddingSize;

		const size_t requiredSize = maxSizePerEntry * numFrames;


		core::ByteStream stream(g_arena);
		stream.resize(requiredSize);

		// write each bones data.
		for (const auto& bone : bones_)
		{
			stream.reset();

			fprintf(f, "BONE_DATA\n");

			for (int32_t i = 0; i < numFrames; i++)
			{
				const FrameData& data = bone.data[i];

				core::StackString<maxSizePerEntry> buf;

				buf.append("POS ");
				buf.appendFmt("%.4g %.4g %.4g\n", 
					data.position.x,
					data.position.y,
					data.position.z);

				buf.append("SCALE ");
				buf.appendFmt("%.4g %.4g %.4g\n",
					data.scale.x,
					data.scale.y,
					data.scale.z);

				buf.append("ANG ");
				buf.appendFmt("%.4g %.4g %.4g %.4g\n",
					data.rotation.x,
					data.rotation.y,
					data.rotation.z,
					data.rotation.w);

				buf.append("\n");

				stream.write(buf.c_str(), buf.length());
			}

			// save stream
			fwrite(stream.begin(), 1, stream.size(), f);

			MayaUtil::IncProcess();
		}

		fclose(f);
		return MS::kSuccess;
	}
	else
	{
		MayaUtil::MayaPrintError("Failed to open file for saving(%i): %s", err, filePath_.c_str());
	}

	return MS::kFailure;
}

MStatus PotatoAnimExporter::convert(const MArgList &args)
{
	MStatus status = processArgs(args);
	if (status == MS::kFailure) {
		return status;
	}

	int32_t numFrames = getNumFrames();

	MayaUtil::MayaPrintMsg("Exporting to: '%s'", filePath_.c_str());
	MayaUtil::MayaPrintMsg("Frames -> start: %i end: %i num: %i", startFrame_, endFrame_, numFrames); 
	MayaUtil::MayaPrintMsg(""); // new line

	// print the error after printing info about frames and the name.
	if (numFrames < 1) {
		MayaUtil::MayaPrintError("Can't export a animation with less than one frame");
		return MS::kFailure;
	}

	// name length check
	if (strlen(filePath_.fileName()) > anim::ANIM_MAX_NAME_LENGTH)
	{
		MayaUtil::MayaPrintError("Anim name is too long. MAX: %i, provided: %i",
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
