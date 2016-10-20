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

const char* XAnimLib::getOutExtension(void) const
{
	return anim::ANIM_FILE_EXTENSION;
}

bool XAnimLib::Convert(IConverterHost& host, int32_t assetId, ConvertArgs& args, const OutPath& destPath)
{
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
	else {	
		X_ERROR("AnimLib", "Missing 'model' option");
		return false;
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
		else if (core::strUtil::IsEqualCaseInsen(pType, "absolute")) {
			type = AnimType::ABSOLUTE;
		}
		else if (core::strUtil::IsEqualCaseInsen(pType, "additive")) {
			type = AnimType::ADDITIVE;
		}
		else if (core::strUtil::IsEqualCaseInsen(pType, "delta")) {
			type = AnimType::DELTA;
		}
		else {
			X_ERROR("AnimLib", "Unknown anim type: \"%s\"", pType);
			return false;
		}
	}

	if (modelName.isEmpty()) {
		X_ERROR("AnimLib", "Src 'model' asset name is empty");
		return false;
	}


	// load file data
	core::Array<uint8_t> fileData(host.getScratchArena());
	if (!host.GetAssetData(assetId, fileData)) {
		X_ERROR("AnimLib", "Failed to get asset data");
		return false;
	}

	if (fileData.isEmpty()) {
		X_ERROR("AnimLib", "File data is empty");
		return false;
	}

	InterAnim inter(g_AnimLibArena);
	if (!inter.LoadFile(fileData)) {
		X_ERROR("AnimLib", "Failed to load inter anim");
		return false;
	}

	// we now need to load the models skelton.
	core::Array<uint8_t> modelFile(g_AnimLibArena);
	if (!host.GetAssetData(modelName, assetDb::AssetType::MODEL, modelFile)) {
		X_ERROR("AnimLib", "Failed to load model for skelton: \"%s\"", modelName.c_str());
		return false;
	}

	model::ModelSkeleton skelton(g_AnimLibArena);
	if (!skelton.LoadRawModelSkelton(modelFile)) {
		X_ERROR("AnimLib", "Failed to load skelton for model: \"%s\"", modelName.c_str());
		return false;
	}

	// right now it's time to process the anim :S
	AnimCompiler compiler(g_AnimLibArena, inter, skelton);
	compiler.setLooping(looping);
	compiler.setAnimType(type);

	return compiler.compile(destPath, posError, angError);
}


X_NAMESPACE_END