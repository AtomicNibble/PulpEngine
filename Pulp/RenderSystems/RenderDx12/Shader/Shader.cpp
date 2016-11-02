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


// ----------------------------------------------------


XShaderTechnique::XShaderTechnique(core::MemoryArenaBase* arena) : 
	hwTechs(arena)
{

}


void XShaderTechnique::assignSourceTech(const ShaderSourceFileTechnique& srcTech)
{
	name = srcTech.getName();
	nameHash = core::StrHash(srcTech.getName());
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

}

const IShaderTech* XShader::getTech(const char* pName) const
{
	for (const auto& tech : techs_)
	size_t i, numTecs;
	numTecs = techs_.size();
	for (i = 0; i < numTecs; i++)
	{
		if (tech.name == pName)
		{
			return &tech;
		}
	}

	return nullptr;
}


} // namespace shader

X_NAMESPACE_END
