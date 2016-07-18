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
	// must be prefixed with IL_ (Input Layout)
	InputLayoutEntry g_ILFlags[] = {
		{ "Normal", ILFlag::Normal },
		{ "BiNornmal", ILFlag::BiNormal },
		{ "Color", ILFlag::Color },
	};


	bool ILFlagFromStr(const char* pStr, Flags<ILFlag>& flagOut)
	{
		const size_t num = sizeof(g_ILFlags) / sizeof(const char*);
		size_t i;
		for (i = 0; i < num; i++)
		{
			if (strUtil::IsEqualCaseInsen(pStr, g_ILFlags[i].name))
			{
				flagOut.Set(g_ILFlags[i].flag);
				return true;
			}
		}
		return false;
	}

	TechFlagEntry g_TechFlags[] = {
		{ "Color", TechFlag::Color },
		{ "Textured", TechFlag::Textured },
		{ "Skinned", TechFlag::Skinned },
		{ "Instanced", TechFlag::Instanced },
	};

	bool TechFlagFromStr(const char* pStr, Flags<TechFlag>& flagOut)
	{
		const size_t num = sizeof(g_TechFlags) / sizeof(const char*);
		size_t i;
		for (i = 0; i < num; i++)
		{
			if (strUtil::IsEqualCaseInsen(pStr, g_TechFlags[i].name))
			{
				flagOut.Set(g_TechFlags[i].flag);
				return true;
			}
		}
		return false;
	}

	PreProEntry g_ProPros[] =
	{
		{ "include", PreProType::Include },
		{ "define", PreProType::Define },
		{ "undef", PreProType::Undef },
		{ "if", PreProType::If },
		{ "ifdef", PreProType::IfDef },
		{ "ifndef", PreProType::IfNDef },
		{ "else", PreProType::Else },
		{ "endif", PreProType::EndIF },
	};

	bool PreProFromStr(core::XLexToken& token, PreProType::Enum& typeOut)
	{
		const size_t num = sizeof(g_ProPros) / sizeof(PreProEntry);
		size_t i;
		for (i = 0; i < num; i++)
		{
			if (strUtil::IsEqualCaseInsen(token.begin(), token.end(), g_ProPros[i].name))
			{
				typeOut = g_ProPros[i].type;
				return true;
			}
		}
		return false;
	}

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

namespace
{



}

// ----------------------------------------------------


XShader* XShaderManager::s_pDefaultShader_ =nullptr;
XShader* XShaderManager::s_pDebugShader_ =nullptr;
XShader* XShaderManager::s_pFixedFunction_ =nullptr;
XShader* XShaderManager::s_pFont_ =nullptr;
XShader* XShaderManager::s_pGui_ =nullptr;
XShader* XShaderManager::s_pDefferedShader_ =nullptr;
XShader* XShaderManager::s_pDefferedShaderVis_ =nullptr;
XShader* XShaderManager::s_pWordShader_ =nullptr;
XShader* XShaderManager::s_pModelShader_ = nullptr;


XShaderManager::XShaderManager() : 
Sourcebin_(nullptr), 
pCrc32_(nullptr)
{


}

XShaderManager::~XShaderManager()
{
	ShaderSourceMap::iterator it = Sourcebin_.begin();

	for (; it != Sourcebin_.end(); ++it)
	{
		X_DELETE(it->second, g_rendererArena);
	}

	Sourcebin_.clear();
}

// -------------------------------------------


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


// -------------------------------------------


XShader::XShader() :
	techs_(g_rendererArena)
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
		techs_[i].release();
	}
}

const int XShader::release()
{ 
	int ref = XBaseAsset::release();
	if (ref == 0)
	{
		X_DELETE(this, g_rendererArena);
	}

	return ref;
}


// --------------------------- Shader Manager --------------------------- 

#if 0

void XShaderManager::Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name)
{
	X_UNUSED(jobSys);

#if 0
	const char* ext;
	if ((ext = core::strUtil::FileExtension(name)) != nullptr)
	{

		// this is just a cache update ignore this.
		if (core::strUtil::IsEqualCaseInsen(ext, "fxcb")) {
			return true;
		}

		// ignore .fxcb.hlsl which are merged sources saved out for debuggin.
		if (core::strUtil::FindCaseInsensitive(name, ".fxcb.hlsl")) {
			return true;
		}

		// is it source?
		bool isSource = true;

		if (core::strUtil::IsEqualCaseInsen(ext, "shader")) {
			isSource = false;
		}


		if (isSource)
		{
			// if source has changed we need to recompile any shader that uses
			// that source file.
			// how to find the shaders that it uses?
			// if i have some sort of reffrence hirarcy
			// i can take any file and know what shaders use it.
			core::Path<char> temp(name);
			temp.toLower(); // all source is lower case

			ShaderSourceMap::const_iterator it = Sourcebin_.find(core::string(temp.fileName()));
			if (it != Sourcebin_.end())
			{
				// reload the source file.
				loadRawSourceFile(temp.fileName(), true);

				SourceFile* src = it->second;
				for (auto refName : src->refrences)
				{
					reloadShader(refName.c_str());
				}
			}
			else
			{
				// log as not found.
				X_WARNING("Shader", "\"%s\" not used, skipping reload", name);
			}
		}
		else
		{
			core::Path<char> temp(name);
			temp.setExtension("");
			temp.toLower();

			reloadShader(temp.fileName());
		}
	}
	return true;
#else
	X_UNUSED(name);
#endif
}



bool XShaderManager::loadCoreShaders(void)
{
	s_pDefaultShader_ =createShader("default");

	if ((s_pFixedFunction_ =forName("ffe")) == nullptr) {
		X_ERROR("Shader", "Failed to load ffe shader");
		return false;
	}
	if ((s_pFont_ =forName("font")) == nullptr) {
		X_ERROR("Shader", "Failed to load font shader");
		return false;
	}
	if ((s_pGui_ =forName("gui")) == nullptr) {
		X_ERROR("Shader", "Failed to load gui shader");
		return false;
	}
	if ((s_pDefferedShader_ =forName("deffered")) == nullptr) {
		X_ERROR("Shader", "Failed to load deffered shader");
		return false;
	}
	if ((s_pDefferedShaderVis_ =forName("defferedVis")) == nullptr) {
		X_ERROR("Shader", "Failed to load defferedVis shader");
		return false;
	}
	if ((s_pWordShader_ =forName("World")) == nullptr) {
		X_ERROR("Shader", "Failed to load World shader");
		return false;
	}

	if ((s_pModelShader_ = forName("Model")) == nullptr) {
		X_ERROR("Shader", "Failed to load Model shader");
		return false;
	}
	return true;
}



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


// -----------------------------------------------

const char* XHWShader::getProfileFromType(ShaderType::Enum type)
{
	switch (type)
	{
		case ShaderType::Vertex:
			return "vs_4_0";
		case ShaderType::Pixel:
			return "ps_4_0";
		case ShaderType::Geometry:
			return "gs_4_0";

		case ShaderType::UnKnown:
			break;
	}

	X_ASSERT_UNREACHABLE();
	return "";
}


} // namespace shader

X_NAMESPACE_END
