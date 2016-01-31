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


	model::ModelCompiler model(gEnv->pJobSys, g_ModelLibArena);

	if (!model.LoadRawModel(rawModelPath)) {
		return false;
	}

	return model.CompileModel(destPath);
}

X_NAMESPACE_END