#include "stdafx.h"
#include "Shader.h"

#include <ICore.h>
#include <IFileSys.h>
#include <IConsole.h>

#include <String\StrRef.h>
#include <String\StringTokenizer.h>
#include <String\StringRange.h>

#include <Hashing\crc32.h>

#include "XRender.h"

#include "ShaderSourceTypes.h"

X_NAMESPACE_BEGIN(render)

using namespace core;

namespace shader
{

// flags for the diffrent input types that can be present in a shader.
//
//	base is awlways:
//		
//		POS, UV
//
//		so we need omes for:
//
//		Normals
//		BiNornmals & Tangents (always together)
//		Color
//
//		Nolte we have diffrent types for some.
//		like there is UV that is 3 floats.
//		other uv's are 2 16bit floats.
//
//		but this is worked out in reflection.
//		since i can work out what type they are.
//
//		this bit needs to be done pre compiel tho since i need to know what inputs type it even supports.
//



BlendType::Enum BlendType::typeFromStr(const char* _str)
{
	core::StackString<256> str(_str, _str + strlen(_str));

	str.toLower();

	if (str.isEqual("zero"))
		return ZERO;
	if (str.isEqual("one"))
		return ONE;

	if (str.isEqual("src_color"))
		return SRC_COLOR;
	if (str.isEqual("inv_src_color"))
		return INV_SRC_COLOR;

	if (str.isEqual("src_alpha"))
		return SRC_ALPHA;
	if (str.isEqual("inv_src_alpha"))
		return INV_SRC_ALPHA;

	if (str.isEqual("dest_alpha"))
		return DEST_ALPHA;
	if (str.isEqual("inv_dest_alpha"))
		return INV_DEST_ALPHA;

	if (str.isEqual("dest_color"))
		return DEST_COLOR;
	if (str.isEqual("inv_dest_color"))
		return INV_DEST_COLOR;

	if (str.isEqual("src_alpha_Sat"))
		return SRC_ALPHA_SAT;

	if (str.isEqual("blend_factor"))
		return BLEND_FACTOR;
	if (str.isEqual("inv_blend_factor"))
		return INV_BLEND_FACTOR;

	if (str.isEqual("src1_color"))
		return SRC1_COLOR;
	if (str.isEqual("inv_src1_color"))
		return INV_SRC1_COLOR;
	if (str.isEqual("src1_alpha"))
		return SRC1_ALPHA;
	if (str.isEqual("inv_src1_alpha"))
		return INV_SRC1_ALPHA;

	// O'Deer
	return INVALID;
}



// ----------------------------------------------------


XShaderTechnique::XShaderTechnique(core::MemoryArenaBase* arena) : 
	hwTechs(arena)
{

}


void XShaderTechnique::assignSourceTech(const ShaderSourceFileTechnique& srcTech)
{
	name = srcTech.getName();
	nameHash = core::StrHash(srcTech.getName());
	// Blend info
	src = srcTech.getSrcBlendInfo();
	dst = srcTech.getDstBlendInfo();
	// State
	state = srcTech.getStateFlag();
	// Cullmode
	cullMode = srcTech.getCullMode();

	// compileflags
	techFlags = srcTech.getTechFlags();
}

void XShaderTechnique::append(const XShaderTechniqueHW& hwTech)
{
	hwTechs.emplace_back(hwTech);
}

// ----------------------------------------------------


XShader::XShader(core::MemoryArenaBase* arena) :
	techs_(arena)
{
	sourceCrc32_ = 0;
	hlslSourceCrc32_ = 0;

	// vertexFmt_ = VertexFormat::P3F_T2F_C4B;

	pHlslFile_ = nullptr;
}

XShader::~XShader()
{
	size_t i, numTecs;
	numTecs = techs_.size();
	for (i = 0; i < numTecs; i++)
	{
		//		techs_[i].release();
	}
}



} // namespace shader

X_NAMESPACE_END
