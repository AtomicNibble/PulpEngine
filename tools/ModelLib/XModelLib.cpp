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

	core::Path<wchar_t> rawModelPath;
	core::Path<wchar_t> destPath;

	{
		{
			const wchar_t* pModelPath = args.getOption(L"raw_model");
			if (pModelPath) {
				rawModelPath = pModelPath;
			}
			else {
				X_ERROR("ModelLib", "Missing 'raw_model' option");
				return false;
			}
		}
		{
			const wchar_t* pDest = args.getOption(L"dest");
			if (pDest) {
				destPath = pDest;
			}
			else {
				X_ERROR("ModelLib", "Missing 'dest' option");
				return false;
			}
		}
	}


	ModelCompiler model(gEnv->pJobSys, g_ModelLibArena);

	// check for optional args.
	{
		ModelCompiler::CompileFlags flags;

		if (args.hasFlag(L"zero_origin")) {
			flags.Set(ModelCompiler::CompileFlag::ZERO_ORIGIN);
		}
		if (args.hasFlag(L"white_vert_col")) {
			flags.Set(ModelCompiler::CompileFlag::WHITE_VERT_COL);
		}
		if (args.hasFlag(L"merge_mesh")) {
			flags.Set(ModelCompiler::CompileFlag::MERGE_MESH);
		}
		if (args.hasFlag(L"merge_verts")) {
			flags.Set(ModelCompiler::CompileFlag::MERGE_VERTS);
		}
		if (args.hasFlag(L"ext_weights")) {
			flags.Set(ModelCompiler::CompileFlag::EXT_WEIGHTS);
		}

		model.setFlags(flags);

		{
			const wchar_t* pScale = args.getOption(L"scale");
			if (pScale) {
				float scale = core::strUtil::StringToFloat<float>(pScale);

				model.SetScale(scale);
			}
		}
		{
			const wchar_t* pWeightThresh = args.getOption(L"weight_thresh");
			if (pWeightThresh) {
				float thresh = core::strUtil::StringToFloat<float>(pWeightThresh);

				model.SetJointWeightThreshold(thresh);
			}
		}
		{
			const wchar_t* pUvMergeThresh = args.getOption(L"uv_merge_thresh");
			if (pUvMergeThresh) {
				float thresh = core::strUtil::StringToFloat<float>(pUvMergeThresh);

				model.SetVertexElipson(thresh);
			}
		}
		{
			const wchar_t* pVertMergeThresh = args.getOption(L"vert_merge_thresh");
			if (pVertMergeThresh) {
				float thresh = core::strUtil::StringToFloat<float>(pVertMergeThresh);

				model.SetTexCoordElipson(thresh);
			}
		}

	}

	if (!model.LoadRawModel(rawModelPath)) {
		return false;
	}

	return model.CompileModel(destPath);
}

X_NAMESPACE_END