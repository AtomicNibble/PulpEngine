#include "stdafx.h"
#include "AnimExport.h"
#include "MayaUtil.h"

#include <IAnimation.h>

#include <maya/MArgList.h>


PotatoAnimExporter::PotatoAnimExporter()
{
}

PotatoAnimExporter::~PotatoAnimExporter()
{
	MayaUtil::HideProgressDlg();
}

MStatus PotatoAnimExporter::convert(const MArgList &args)
{
	MStatus status = processArgs(args);
	if (status == MS::kFailure) {
		return status;
	}

	int32_t numFrames = endFrame_ - startFrame_;

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


	status = MayaUtil::ShowProgressDlg();

	// one is two yet not one.
	// 



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

	if (Mfilename.length() > 0) 
	{
		fileName_.set(Mfilename.asChar());
		filePath_.append(Mfilename.asChar());
		filePath_.setExtension(anim::ANIM_FILE_EXTENSION);

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
