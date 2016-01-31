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


bool XAnimLib::Convert(ConvertArgs& args)
{
	core::Path<wchar_t> interPath;
	core::Path<wchar_t> modelPath;
	core::Path<wchar_t> destPath;

	{
		{
			const wchar_t* pInterPath = args.getOption(L"inter_anim");
			if (pInterPath) {
				interPath = pInterPath;
			}
			else {
				X_ERROR("AnimLib", "Missing 'inter_anim' option");
				return false;
			}
		}
		{
			const wchar_t* pModelPath = args.getOption(L"model");
			if (pModelPath) {
				modelPath = pModelPath;
			}
			else {
				X_ERROR("AnimLib", "Missing 'model' option");
				return false;
			}
		}
		{
			const wchar_t* pDest = args.getOption(L"dest");
			if (pDest) {
				destPath = pDest;
			}
			else {
				X_ERROR("AnimLib", "Missing 'dest' option");
				return false;
			}
		}
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