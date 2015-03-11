#include "stdafx.h"
#include "XShader.h"

X_NAMESPACE_BEGIN(shader)

render::XRenderResourceContainer* XHWShader::pHWshaders = nullptr;


XHWShader::XHWShader() :
	type(ShaderType::UnKnown),
	pShader(nullptr),
	sourceCrc32(0)
{

}



X_NAMESPACE_END