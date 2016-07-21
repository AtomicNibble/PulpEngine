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


namespace 
{
	

	template<typename TFlags>
	void AppendFlagTillEqual(const Flags<TFlags>& srcflags, Flags<TFlags>& dest)
	{
		if (srcflags.IsAnySet() && srcflags.ToInt() != dest.ToInt())
		{
			for (size_t i = 0; i < 32; i++)
			{
				TFlags::Enum flag = static_cast<TFlags::Enum>(1 << i);
				if (srcflags.IsSet(flag) && !dest.IsSet(flag)) {
					dest.Set(flag);
					return;
				}
			}
		}
	}

}


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




// ----------------------------------------------------


XShader::XShader(core::MemoryArenaBase* arena) :
	techs_(arena)
{
	sourceCrc32_ = 0;
	hlslSourceCrc32_ = 0;

	vertexFmt_ = VertexFormat::P3F_T2F_C4B;

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

const int32_t XShader::release(void)
{
	int32_t ref = XBaseAsset::release();
	if (ref == 0)
	{
		X_DELETE(this, g_rendererArena);
	}

	return ref;
}






#if 0

XShaderTechnique& XShaderTechnique::operator=(const ShaderSourceFile::Technique& srcTech)
{
	name = srcTech.name_;
	nameHash = core::StrHash(srcTech.name_);
	// Blend info
	src = srcTech.src_;
	dst = srcTech.dst_;
	// State
	state = srcTech.state_;
	// Cullmode
	cullMode = srcTech.cullMode_;

	// blend info
	src = srcTech.src_;
	dst = srcTech.dst_;

	// hw tech
//	pCurHwTech = srcTech.pCurHwTech;
//	hwTechs = srcTech.hwTechs;

	// compileflags
	techFlags = srcTech.techFlags_;

	return *this;
}

#endif

// -------------------------------------------


// --------------------------- Shader Manager --------------------------- 

#if 0



XShaderResources* XShaderManager::createShaderResources(const XInputShaderResources& input)
{
	int i;

	XShaderResources* pRes = X_NEW_ALIGNED(XShaderResources, g_rendererArena, "ShaderResource", 8);

	pRes->diffuse = input.material.diffuse;
	pRes->emissive = input.material.emissive;
	pRes->spec = input.material.specular;
	pRes->specShine = input.material.specShininess;
	pRes->opacity = input.opacity;

	for (i = 0; i < shader::ShaderTextureIdx::ENUM_COUNT; i++)
	{
		const XTextureResource& res = input.textures[i];

		if (!res.name.isEmpty())
		{
			X_ASSERT_NOT_IMPLEMENTED();
		//	pRes->pTextures[i] = textureResourceForName(res.name);
		}
	}

	return pRes;
}


#endif 


// -----------------------------------------------

#if 0

XShaderResources::XShaderResources()
{
	core::zero_object(pTextures);

	specShine = 0.f;
	glow = 0.f;
	opacity = 0.f;
}


XShaderResources::~XShaderResources()
{
	freeAssets();
}

void XShaderResources::release(void)
{
	X_ASSERT_NOT_IMPLEMENTED();
}

// called from render thread.
void XShaderResources::RT_Release(void)
{
	freeAssets();

	X_DELETE(this,g_rendererArena);
}

void XShaderResources::freeAssets(void)
{
	int i;

	for (i = 0; i < ShaderTextureIdx::ENUM_COUNT; i++)
	{
		if (pTextures[i]) {
			core::Mem::DeleteAndNull(pTextures[i], g_rendererArena);
		}
	}
}

#endif

// -----------------------------------------------



} // namespace shader

X_NAMESPACE_END
