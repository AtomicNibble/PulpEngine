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

const char* XModelLib::getOutExtension(void) const
{
	return model::MODEL_FILE_EXTENSION;
}


bool XModelLib::Convert(IConverterHost& host, ConvertArgs& args, 
	const core::Array<uint8_t>& fileData, const OutPath& destPath)
{
	X_UNUSED(host);
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pJobSys);

	ModelCompiler::CompileFlags flags;

	core::json::Document d;
	d.Parse(args.c_str());


	if (fileData.isEmpty()) {
		X_ERROR("ModelLib", "File data is empty");
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

	if (!model.LoadRawModel(fileData)) {
		return false;
	}

	// check materials.
	for (size_t i = 0; i < model.numLods(); i++)
	{
		const auto& lod = model.getLod(i);

		for (size_t m = 0; m < lod.numMeshes(); m++)
		{
			const auto& mesh = lod.getMesh(m);
			const auto& material = mesh.getMaterial();

			if (!host.AssetExists(material.name_.c_str(), assetDb::AssetType::MATERIAL))
			{
				// using a material we don't have yet..
				X_WARNING("Model", "Model lod: %" PRIuS " mesh: %" PRIuS " has a unknown material: \"%s\"",
					i, m, material.name_.c_str());
			}
		}
	}

	// set lod distances.
	for (size_t i = 0; i < model::MODEL_MAX_LODS; i++)
	{
		core::StackString<32> buf;
		buf.appendFmt("lod%i_dis", i);

		if (d.HasMember(buf.c_str())) {
			model.setLodDistance(d[buf.c_str()].GetFloat(), i);
		}
	}

	return model.CompileModel(destPath);
}

X_NAMESPACE_END