#include "stdafx.h"
#include "XAnimLib.h"

#include "anim_inter.h"

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
	X_UNUSED(pAnimInter);
	X_UNUSED(pModel);
	X_UNUSED(pDest);

	core::Path<char> interPath(pAnimInter);

	InterAnim inter(g_AnimLibArena);

	if (!inter.LoadFile(interPath)) {
		return false;
	}


	return false;
}


X_NAMESPACE_END