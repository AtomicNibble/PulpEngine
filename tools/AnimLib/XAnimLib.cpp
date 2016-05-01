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
	const OutPath& destPath_)
{
	core::Path<char> interPath;
	core::Path<char> modelPath;
	core::Path<char> destPath;


	core::json::Document d;
	d.Parse(args.c_str());

	if (d.HasMember("inter_anim")) {
		interPath = d["inter_anim"].GetString();
	}
	if (d.HasMember("model")) {
		modelPath = d["model"].GetString();
	}
	if (d.HasMember("dest")) {
		destPath = d["dest"].GetString();
	}


	// log all that are missing then return.
	if(interPath.isEmpty()) {
		X_ERROR("AnimLib", "Missing 'inter_anim' option");
	}
	if (modelPath.isEmpty()) {
		X_ERROR("AnimLib", "Missing 'model' option");
	}
	if (destPath.isEmpty()) {
		X_ERROR("AnimLib", "Missing 'dest' option");
	}

	if (interPath.isEmpty() || interPath.isEmpty() || interPath.isEmpty()) {
		return false;
	}


	InterAnim inter(g_AnimLibArena);

	if (!inter.LoadFile(interPath)) {
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