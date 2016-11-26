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

#include <../../tools/ShaderLib/ShaderSourceTypes.h>

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


	XShaderTechniqueHW::XShaderTechniqueHW(core::MemoryArenaBase* arena) :
		cbLinks(arena)
	{
		cbLinks.setGranularity(2);

		IlFmt = InputLayoutFormat::Invalid;
		pVertexShader = nullptr;
		pPixelShader = nullptr;
		pGeoShader = nullptr;
		pHullShader = nullptr;
		pDomainShader = nullptr;
	}

	bool XShaderTechniqueHW::canDraw(void) const
	{
		bool canDraw = true;

		if (pVertexShader) {
			canDraw &= pVertexShader->getStatus() == ShaderStatus::ReadyToRock;
		}
		if (pPixelShader) {
			canDraw &= pPixelShader->getStatus() == ShaderStatus::ReadyToRock;
		}
		if (pGeoShader) {
			canDraw &= pGeoShader->getStatus() == ShaderStatus::ReadyToRock;
		}
		if (pHullShader) {
			canDraw &= pHullShader->getStatus() == ShaderStatus::ReadyToRock;
		}
		if (pDomainShader) {
			canDraw &= pDomainShader->getStatus() == ShaderStatus::ReadyToRock;
		}

		return canDraw;
	}



	bool XShaderTechniqueHW::tryCompile(bool forceSync)
	{

		if (pVertexShader && pVertexShader->getStatus() == ShaderStatus::NotCompiled) {
			if (!pVertexShader->compile(forceSync)) {
				return false;
			}

			addCbufs(pVertexShader);

			IlFmt = pVertexShader->getILFormat();
		}
		if (pPixelShader && pPixelShader->getStatus() == ShaderStatus::NotCompiled) {
			if (!pPixelShader->compile(forceSync)) {
				return false;
			}

			addCbufs(pPixelShader);
		}

		return true;
	}

	const XShaderTechniqueHW::CBufLinksArr& XShaderTechniqueHW::getCbufferLinks(void) const
	{
		return cbLinks;
	}

	void XShaderTechniqueHW::addCbufs(XHWShader* pShader)
	{
		auto& cbufs = pShader->getCBuffers();
		for (auto& cb : cbufs)
		{
			// we match by name and size currently.
			for (auto& link : cbLinks)
			{
				if (link.pCBufer->isEqual(cb))
				{
					link.stages.Set(pShader->getType());
					goto skip_outer_loop;
				}
			}

			cbLinks.emplace_back(pShader->getType(), &cb);

		skip_outer_loop:;
		}
	}



// ----------------------------------------------------


XShaderTechnique::XShaderTechnique(core::MemoryArenaBase* arena) : 
	hwTechs(arena)
{
	hwTechs.setGranularity(6);
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

bool XShaderTechnique::tryCompile(bool forceSync)
{
	for (auto& tech : hwTechs)
	{
		if (tech.tryCompile(forceSync))
		{
			return false;
		}
	}

	return true;
}


IShaderPermatation* XShaderTechnique::getPermatation(VertexFormat::Enum vertexFmt)
{
	InputLayoutFormat::Enum ilFmt = Util::ILfromVertexFormat(vertexFmt);

	for (auto& tech : hwTechs)
	{
		if (tech.IlFmt == ilFmt)
		{
			return &tech;
		}
	}

	return nullptr;
}

// ----------------------------------------------------


XShader::XShader(core::MemoryArenaBase* arena) :
	techs_(arena)
{
	sourceCrc32_ = 0;
	hlslSourceCrc32_ = 0;
	pHlslFile_ = nullptr;
}

XShader::~XShader()
{

}

IShaderTech* XShader::getTech(const char* pName)
{
	for (auto& tech : techs_)
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
