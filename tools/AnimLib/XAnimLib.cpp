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


bool XAnimLib::ConvertAnim(const char* pAnimInter,
	const char* pModel, const char* pDest)
{
	core::Path<char> interPath(pAnimInter);
	core::Path<char> modelPath(pModel);
	core::Path<char> destPath(pDest);

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