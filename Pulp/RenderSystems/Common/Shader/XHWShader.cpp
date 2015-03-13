#include "stdafx.h"
#include "XShader.h"

X_NAMESPACE_BEGIN(shader)

render::XRenderResourceContainer* XHWShader::pHWshaders = nullptr;


XHWShader::XHWShader() :
	sourceCrc32(0),
	type(ShaderType::UnKnown),
	IlFmt_(InputLayoutFormat::POS_UV),
	numRenderTargets_(0),
	numSamples_(0),
	numConstBuffers_(0),
{

}



X_NAMESPACE_END