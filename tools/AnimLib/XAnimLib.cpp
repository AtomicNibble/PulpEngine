#include "stdafx.h"
#include "XAnimLib.h"

#include "anim_inter.h"
#include "../ModelLib/ModelSkeleton.h"
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
	if (fileData.isEmpty()) {
		X_ERROR("AnimLib", "File data is empty");
		return false;
	}
	if (destPath.isEmpty()) {
		X_ERROR("AnimLib", "Missing 'dest' option");
		return false;
	}

	core::json::Document d;
	d.Parse(args.c_str());

	core::string modelName;
	float posError = AnimCompiler::DEFAULT_POS_ERRR;
	float angError = AnimCompiler::DEFAULT_ANGLE_ERRR;
	bool looping = false;
	AnimType::Enum type = AnimType::RELATIVE;

	if (d.HasMember("model")) {
		modelName = d["model"].GetString();
	}
	if (d.HasMember("posError")) {
		posError = d["posError"].GetFloat();
	}
	if (d.HasMember("angError")) {
		angError = d["angError"].GetFloat();
	}
	if (d.HasMember("looping")) {
		looping = d["looping"].GetBool();
	}
	if (d.HasMember("type")) {
		const char* pType = d["type"].GetString();
		if (core::strUtil::IsEqualCaseInsen(pType, "relative")) {
			type = AnimType::RELATIVE;
		}
		if (core::strUtil::IsEqualCaseInsen(pType, "absolute")) {
			type = AnimType::ABSOLUTE;
		}
		if (core::strUtil::IsEqualCaseInsen(pType, "additive")) {
			type = AnimType::ADDITIVE;
		}
		if (core::strUtil::IsEqualCaseInsen(pType, "delta")) {
			type = AnimType::DELTA;
		}
		else {
			X_ERROR("AnimLib", "Unkown anim type: \"%s\"", pType);
			return false;
		}
	}

	if (modelName.isEmpty()) {
		X_ERROR("AnimLib", "Missing 'model' option");
		return false;
	}

	InterAnim inter(g_AnimLibArena);

	if (!inter.LoadFile(fileData)) {
		X_ERROR("AnimLib", "Failed to load inter anim");
		return false;
	}

	// we now need to load the models skelton.
	model::ModelSkeleton model(g_AnimLibArena);

//	if (!model.LoadSkelton(modelName)) {
//		X_ERROR("AnimLib", "Failed to load skelton for model: \"%s\"", modelName.c_str());
//		return false;
//	}

	// right now it's time to process the anim :S
	AnimCompiler compiler(g_AnimLibArena, inter, model);
	compiler.setLooping(looping);
	compiler.setAnimType(type);

	return compiler.compile(destPath, posError, angError);
}


X_NAMESPACE_END