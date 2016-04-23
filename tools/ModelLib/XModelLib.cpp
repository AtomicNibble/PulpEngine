#include "stdafx.h"
#include "XModelLib.h"

#include "RawModel.h"
#include "ModelCompiler.h"

X_NAMESPACE_BEGIN(model)

XModelLib::XModelLib()
{

}

XModelLib::~XModelLib()
{


}

bool XModelLib::Convert(ConvertArgs& args)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pJobSys);

	core::Path<char> rawModelPath;
	core::Path<char> destPath;
	ModelCompiler::CompileFlags flags;


	core::json::Document d;
	d.Parse(args.c_str());

	if (d.HasMember("raw_model")) {
		rawModelPath = d["raw_model"].GetString();
	}
	if (d.HasMember("dest")) {
		destPath = d["dest"].GetString();
	}

	if (rawModelPath.isEmpty()) {
		X_ERROR("ModelLib", "Missing 'raw_model' option");
	}
	if (destPath.isEmpty()) {
		X_ERROR("ModelLib", "Missing 'dest' option");
	}
	if (rawModelPath.isEmpty() || destPath.isEmpty()) {
		return false;
	}


	if (d.HasMember("zero_origin")) {
		if (d["zero_origin"].GetBool()) {
			flags.Set(ModelCompiler::CompileFlag::ZERO_ORIGIN);
		}
	}
	if (d.HasMember("white_vert_col")) {
		if (d["white_vert_col"].GetBool()) {
			flags.Set(ModelCompiler::CompileFlag::WHITE_VERT_COL);
		}
	}
	if (d.HasMember("merge_mesh")) {
		if (d["merge_mesh"].GetBool()) {
			flags.Set(ModelCompiler::CompileFlag::MERGE_MESH);
		}
	}
	if (d.HasMember("merge_verts")) {
		if (d["merge_verts"].GetBool()) {
			flags.Set(ModelCompiler::CompileFlag::MERGE_VERTS);
		}
	}
	if (d.HasMember("ext_weights")) {
		if (d["ext_weights"].GetBool()) {
			flags.Set(ModelCompiler::CompileFlag::EXT_WEIGHTS);
		}
	}

	ModelCompiler model(gEnv->pJobSys, g_ModelLibArena);
	model.setFlags(flags);

	// overrides.
	if (d.HasMember("scale")) {
		model.SetScale(d["scale"].GetFloat());
	}
	if (d.HasMember("weight_thresh")) {
		model.SetScale(d["weight_thresh"].GetFloat());
	}
	if (d.HasMember("uv_merge_thresh")) {
		model.SetScale(d["uv_merge_thresh"].GetFloat());
	}
	if (d.HasMember("vert_merge_thresh")) {
		model.SetScale(d["vert_merge_thresh"].GetFloat());
	}

	if (!model.LoadRawModel(rawModelPath)) {
		return false;
	}

	return model.CompileModel(destPath);
}

X_NAMESPACE_END