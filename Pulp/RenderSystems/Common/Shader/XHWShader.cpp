#include "stdafx.h"
#include "XShader.h"

X_NAMESPACE_BEGIN(shader)

render::XRenderResourceContainer* XHWShader::pHWshaders = nullptr;


XHWShader::XHWShader() :
	type(ShaderType::UnKnown),
	pShader(nullptr),
	sourceCrc32(0)
{
	// set as default layout.
	vertexFmt = VertexFormat::P3F_T2S;
}



X_NAMESPACE_END