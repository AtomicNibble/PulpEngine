#include "stdafx.h"
#include "XAnimLib.h"

#include "anim_inter.h"
#include "ModelSkeleton.h"
#include "AnimCompiler.h"

X_NAMESPACE_BEGIN(anim)

XAnimLib::XAnimLib()
{

}

XAnimLib::~XAnimLib()
{


}


bool XAnimLib::Convert(ConvertArgs& args, const core::Array<uint8_t>& fileData,
	const OutPath& destPath)
{
	core::Path<char> modelPath;

	core::json::Document d;
	d.Parse(args.c_str());

	if (d.HasMember("model")) {
		modelPath = d["model"].GetString();
	}


	if (fileData.isEmpty()) {
		X_ERROR("AnimLib", "File data is empty");
		return false;
	}
	if (destPath.isEmpty()) {
		X_ERROR("AnimLib", "Missing 'dest' option");
		return false;
	}
	if (modelPath.isEmpty()) {
		X_ERROR("AnimLib", "Missing 'model' option");
		return false;
	}


	InterAnim inter(g_AnimLibArena);

	if (!inter.LoadFile(fileData)) {
		return false;
	}

	// we now need to load the models skelton.
	model::ModelSkeleton model(g_AnimLibArena);

	if (!model.LoadSkelton(modelPath)) {
		return false;
	}

	// right now it's time to process the anim :S
	AnimCompiler compiler(g_AnimLibArena, inter, model);

	return compiler.compile(destPath);
}


X_NAMESPACE_END