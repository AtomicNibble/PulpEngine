#include "stdafx.h"
#include "XShader.h"

#include <ICore.h>
#include <IFileSys.h>
#include <IConsole.h>

#include <String\StrRef.h>
#include <String\StringTokenizer.h>
#include <String\StringRange.h>

#include <Hashing\crc32.h>


// #include <../Common/Textures/XTexture.h>
#include "../Textures/XTexture.h"
#include "../XRender.h"

X_NAMESPACE_BEGIN(shader)


using namespace core;

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
shaders_(nullptr, 256), 
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

void Cmd_ListShaders(core::IConsoleCmdArgs* pArgs)
{
	X_UNUSED(pArgs);
	render::gRenDev->ShaderMan_.listShaders();
}

void Cmd_ListShaderSources(core::IConsoleCmdArgs* pArgs)
{
	X_UNUSED(pArgs);
	render::gRenDev->ShaderMan_.listShaderSources();
}

bool XShaderManager::Init(void)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pCore);
	X_ASSERT_NOT_NULL(gEnv->pHotReload);
	X_ASSERT_NOT_NULL(g_rendererArena);

	pCrc32_ = gEnv->pCore->GetCrc32();

	Sourcebin_.setArena(g_rendererArena, 64);
	shaders_.setArena(g_rendererArena, 256);

	texture::XTexture::init();

	if (!loadCoreShaders())
		return false;

	// hotreload support.
	gEnv->pHotReload->addfileType(this, "hlsl");
	gEnv->pHotReload->addfileType(this, "inc");
	gEnv->pHotReload->addfileType(this, "shader");
	gEnv->pHotReload->addfileType(this, "fxcb");

	ADD_COMMAND("shaderList", Cmd_ListShaders, core::VarFlag::SYSTEM, "lists the loaded shaders");
	ADD_COMMAND("shaderListsourcebin", Cmd_ListShaderSources, core::VarFlag::SYSTEM, "lists the loaded shaders sources");

	// alternate names
	ADD_COMMAND("listShaders", Cmd_ListShaders, core::VarFlag::SYSTEM, "lists the loaded shaders");
	ADD_COMMAND("listShaderSource", Cmd_ListShaderSources, core::VarFlag::SYSTEM, "lists the loaded shaders sources");

	return true;
}

bool XShaderManager::Shutdown(void)
{
	X_LOG0("ShadersManager", "Shutting Down");
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pHotReload);

	gEnv->pHotReload->addfileType(nullptr, "hlsl");
	gEnv->pHotReload->addfileType(nullptr, "inc");
	gEnv->pHotReload->addfileType(nullptr, "shader");
	gEnv->pHotReload->addfileType(nullptr, "fxcb");
	
	freeCoreShaders();
	freeSourcebin();

	// free the shaders.
	XResourceContainer::ResourceItor it = shaders_.begin();
	XResourceContainer::ResourceItor end = shaders_.end();
	for (; it != end; )
	{
		XShader* pShader = static_cast<XShader*>(it->second);

		++it;

		if (!pShader)
			continue;

		X_WARNING("ShadersManager", "\"%s\" was not deleted", pShader->getName());

	//	pTex->forceRelease();
	}

	shaders_.free();

	return true;
}


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

XShader* XShaderManager::reloadShader(const char* name)
{
	X_ASSERT_NOT_NULL(name);

	XShader* shader = nullptr;
	ShaderSourceFile* source = nullptr;
	size_t i, x, numTecs;

	// already loaded?
	shader = static_cast<XShader*>(shaders_.findAsset(name));

	if (shader)
	{
		
		// reload the shader file.
		source = loadShaderFile(name, true);
		if (source)
		{
			// we don't reload the .shader if the source is the same.
			if (shader->sourceCrc32_ != source->sourceCrc32_)
			{
				X_LOG0("Shader", "reloading shader: %s", name);

				numTecs = source->numTechs();

				// might be more techs etc..
				shader->techs_.resize(numTecs);
				shader->sourceCrc32_ = source->sourceCrc32_;
				shader->hlslSourceCrc32_ = source->hlslSourceCrc32_;

				for (i = 0; i < numTecs; i++)
				{
					XShaderTechnique& tech = shader->techs_[i];
					ShaderSourceFile::Technique& srcTech = source->techniques_[i];

					tech.hwTechs.clear();
					tech = srcTech;
					// tech flags may have changed.
					// IL flags won't have tho.
					Flags<TechFlag> techFlags;
					Flags<ILFlag> ILFlags;
					Flags<ILFlag> ILFlagSrc = source->pHlslFile_->ILFlags;

					// for every input layout we compile all the techFlags 
					// plus one without flags passed.
					uint32_t numILFlags = core::bitUtil::CountBits(ILFlagSrc.ToInt());
					uint32_t numTechFlags = core::bitUtil::CountBits(tech.techFlags.ToInt());
					uint32_t j;
					for (x = 0; x < numILFlags + 1; x++)
					{
						techFlags.Clear();

						for (j = 0; j < numTechFlags + 1; j++)
						{
							XShaderTechniqueHW hwTech;

							// create the hardware shaders.
							hwTech.pVertexShader = XHWShader::forName(name, 
								srcTech.vertex_func_,
								source->pHlslFile_->fileName.c_str(), techFlags,
								ShaderType::Vertex, ILFlags, source->pHlslFile_->sourceCrc32);

							hwTech.pPixelShader = XHWShader::forName(name, 
								srcTech.pixel_func_,
								source->pHlslFile_->fileName.c_str(), techFlags,
								ShaderType::Pixel, ILFlags, source->pHlslFile_->sourceCrc32);

							hwTech.techFlags = techFlags;
							hwTech.ILFlags = ILFlags;
							hwTech.IlFmt = hwTech.pVertexShader->getILFormat();
							tech.append(hwTech);

							// add tech flag
							AppendFlagTillEqual(tech.techFlags, techFlags);
						}

						// add in the next flag.
						AppendFlagTillEqual(ILFlagSrc, ILFlags);
					}

					tech.resetCurHWTech();

				}
			}
			else if (shader->hlslSourceCrc32_ != source->hlslSourceCrc32_)
			{
				X_LOG0("Shader", "reloading shader source: %s", 
					shader->pHlslFile_->name.c_str());
				if (source)
				{
					numTecs = shader->numTechs();

					// update crc
					shader->hlslSourceCrc32_ = source->hlslSourceCrc32_;

					for (i = 0; i < numTecs; i++)
					{
						XShaderTechnique& tech = shader->techs_[i];

						for (x = 0; x < tech.hwTechs.size(); x++)
						{
							XShaderTechniqueHW& hwTech = tech.hwTechs[x];

							const char* vertEntry = hwTech.pVertexShader->getEntryPoint();
							const char* pixelEntry = hwTech.pPixelShader->getEntryPoint();

							TechFlags techFlags = hwTech.techFlags;
							ILFlags ILFlags = hwTech.ILFlags;


							hwTech.pVertexShader = XHWShader::forName(name, vertEntry,
								source->pHlslFile_->fileName.c_str(), techFlags,
								ShaderType::Vertex, ILFlags, source->pHlslFile_->sourceCrc32);

							hwTech.pPixelShader = XHWShader::forName(name, pixelEntry,
								source->pHlslFile_->fileName.c_str(), techFlags,
								ShaderType::Pixel, ILFlags, source->pHlslFile_->sourceCrc32);
						}					
					}
				}
			}
			else
			{		
				uint32_t lastCrc32 = shader->pHlslFile_->sourceCrc32;

				SourceFile* Hlslsource = loadRawSourceFile(shader->pHlslFile_->fileName.c_str(), true);

				if (Hlslsource)
				{
					if (lastCrc32 != Hlslsource->sourceCrc32)
					{
						X_LOG0("Shader", "reloading shader source: %s", shader->pHlslFile_->name.c_str());

						// the shaders source has changed.
						// we end up here typically when a source file included by 
						// this .shader main .hlsl forcing a reload of the .shader
						// so that the main source can be checked.
						// if we are in this scope we need to recompile all the hardware shaders.
						numTecs = shader->numTechs();

						for (i = 0; i < numTecs; i++)
						{
							XShaderTechnique& tech = shader->techs_[i];
							for (x = 0; x < tech.hwTechs.size(); x++)
							{
								XShaderTechniqueHW& hwTech = tech.hwTechs[x];
							
								const char* vertEntry = hwTech.pVertexShader->getEntryPoint();
								const char* pixelEntry = hwTech.pPixelShader->getEntryPoint();

								TechFlags techFlags = hwTech.techFlags;
								ILFlags ILFlags = hwTech.ILFlags;

								hwTech.pVertexShader = XHWShader::forName(name, vertEntry,
									source->pHlslFile_->fileName.c_str(), techFlags,
									ShaderType::Vertex, ILFlags, source->pHlslFile_->sourceCrc32);

								hwTech.pPixelShader = XHWShader::forName(name, pixelEntry,
									source->pHlslFile_->fileName.c_str(), techFlags,
									ShaderType::Pixel, ILFlags, source->pHlslFile_->sourceCrc32);
							}
							
						}
					}
					else
					{
						X_LOG0("Shader", "shader source: %s has not changed, reload skipped", 
							shader->pHlslFile_->name.c_str());
					}
				}
			}

			X_DELETE( source, g_rendererArena);
		}
	}	
	else
	{
#if X_DEBUG
		X_LOG0("Shader", "'%s' not currently used skipping reload", name);
#endif // !X_DEBUG
	}

	return shader;
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



bool XShaderManager::freeCoreShaders(void)
{
	if (s_pDefaultShader_)
		s_pDefaultShader_->release();
//	if (s_pDebugShader_)
//		s_pDebugShader_->release();
	if (s_pFixedFunction_)
		s_pFixedFunction_->release();
	if (s_pFont_)
		s_pFont_->release();
	if (s_pGui_)
		s_pGui_->release();
	if (s_pDefferedShader_)
		s_pDefferedShader_->release();
	if (s_pDefferedShaderVis_)
		s_pDefferedShaderVis_->release();

	if (s_pWordShader_)
		s_pWordShader_->release();
	if (s_pModelShader_)
		s_pModelShader_->release();

	return true;
}

bool XShaderManager::freeSourcebin(void)
{
	ShaderSourceMap::iterator it = Sourcebin_.begin();;
	for (; it != Sourcebin_.end(); ++it)
	{
		X_DELETE(it->second, g_rendererArena);
	}

	Sourcebin_.free();
	return true;
}

void XShaderManager::listShaders(void)
{
	render::XRenderResourceContainer::ResourceConstItor it = shaders_.begin();
	XShader* pShader;

	X_LOG0("Shader", "------------- ^8Shaders(%" PRIuS ")^7 -------------", shaders_.size());

	for (; it != shaders_.end(); ++it)
	{
		pShader = static_cast<XShader*>(it->second);

		X_LOG0("Shader", "Name: ^2\"%s\"^7 tecs: %" PRIuS " crc: ^10x%08x^7 vertexFmt: %s",
			pShader->name_.c_str(),
			pShader->techs_.size(),
			pShader->sourceCrc32_,
			VertexFormat::toString(pShader->vertexFmt_));
	}
	X_LOG0("Shader", "------------ ^8Shaders End^7 -------------");
}

void XShaderManager::listShaderSources(void)
{
	ShaderSourceMap::const_iterator it = Sourcebin_.begin();
	const SourceFile* pSource;

	X_LOG0("Shader", "--------- ^8Shader Sources(%" PRIuS ")^7 ---------", Sourcebin_.size());

	for (; it != Sourcebin_.end(); ++it)
	{
		pSource = it->second;

		X_LOG0("Shader", "Name: ^2\"%s\"^7 crc: ^10x%08x^7",
			pSource->fileName.c_str(),
			pSource->sourceCrc32);
	}

	X_LOG0("Shader", "--------- ^8Shader Sources End^7 ---------");
}




XShader* XShaderManager::loadShader(const char* name)
{
	X_ASSERT_NOT_NULL(name);

	XShader* shader = nullptr;

	// already loaded?
	shader = static_cast<XShader*>(shaders_.findAsset(name));

	if (shader)
		return shader;
	
	// load the source file containing the techniques info.
	ShaderSourceFile* source = loadShaderFile(name);

	if (source)
	{
		size_t j, numTecs;

		numTecs = source->numTechs();

		shader = createShader(name);
		shader->techs_.resize(source->numTechs());
		shader->sourceCrc32_ = source->sourceCrc32_;
		shader->hlslSourceCrc32_ = source->hlslSourceCrc32_;
		shader->pHlslFile_ = source->pHlslFile_;

		// I might use only the HLSL crc, do .shader changes matter?
		// for now use hlsl + .shader
		// uint32_t crc = source->pHlslFile->sourceCrc32;
		Flags<TechFlag> techFlags;
		Flags<ILFlag> ILFlags;
		Flags<ILFlag> ILFlagSrc = source->pHlslFile_->ILFlags;

		for (j = 0; j < numTecs; j++)
		{
			XShaderTechnique& tech = shader->techs_[j];
			ShaderSourceFile::Technique& srcTech = source->techniques_[j];
			tech = srcTech;

			// for every input layout we compile all the techFlags 
			// plus one without flags passed.
			uint32_t numILFlags = core::bitUtil::CountBits(ILFlagSrc.ToInt());
			uint32_t numTechFlags = core::bitUtil::CountBits(tech.techFlags.ToInt());
			uint32_t i, x;
			for (i = 0; i < numILFlags + 1; i++)
			{
				techFlags.Clear();

				for (x = 0; x < numTechFlags + 1; x++)
				{
					XShaderTechniqueHW hwTech;

					// create the hardware shaders.
					hwTech.pVertexShader = XHWShader::forName(name, 
						srcTech.vertex_func_,
						source->pHlslFile_->fileName.c_str(), techFlags,
						ShaderType::Vertex, ILFlags, source->pHlslFile_->sourceCrc32);

					hwTech.pPixelShader = XHWShader::forName(name,
						srcTech.pixel_func_,
						source->pHlslFile_->fileName.c_str(), techFlags,
						ShaderType::Pixel, ILFlags, source->pHlslFile_->sourceCrc32);

					hwTech.techFlags = techFlags;
					hwTech.ILFlags = ILFlags;
					hwTech.IlFmt = hwTech.pVertexShader->getILFormat();
					tech.append(hwTech);

					// add tech flag
					AppendFlagTillEqual(tech.techFlags, techFlags);
				}

				// add in the next flag.
				AppendFlagTillEqual(ILFlagSrc, ILFlags);
			}
			
			tech.resetCurHWTech();
		}

		X_DELETE(source,g_rendererArena);
	}

	return shader;
}


// -------------------------------------------


XShader* XShaderManager::createShader(const char* name)
{
	X_ASSERT_NOT_NULL(name);

	XShader* pShader;

	// check if this shader already exsists.
	pShader = static_cast<XShader*>(shaders_.findAsset(name));
	
	if (pShader)
	{
		pShader->addRef();
	}
	else
	{
		pShader = X_NEW_ALIGNED(XShader, g_rendererArena, "Shader", 16);
		pShader->name_ = name;
		shaders_.AddAsset(name, pShader);
	}

	return pShader;
}



bool BlendInfo::ParseBlendInfo(const char* name,
	const core::StackString512& key, const core::StackString512& value)
{
	X_ASSERT_NOT_NULL(name);

	core::StackString<64> color_str(name);
	core::StackString<64> alpha_str(name);

	color_str.append("_color");
	alpha_str.append("_alpha");

	if (color_str.isEqual(key.c_str()))
	{
		color = BlendType::typeFromStr(value.c_str());
		if (color == BlendType::INVALID) {
			X_ERROR("Shader", "invalid %s type: %s", color_str.c_str(), value.c_str());
			return false; // invalid
		}
	}
	else if (alpha_str.isEqual(key.c_str()))
	{
		alpha = BlendType::typeFromStr(value.c_str());
		if (alpha == BlendType::INVALID) {
			X_ERROR("Shader", "invalid %s type: %s", alpha_str.c_str(), value.c_str());
			return false; // invalid
		}
	}
	else
		return false;

	return true;
}

ShaderSourceFile::Technique::Technique()
{
	depth_write_ = true;
}

bool ShaderSourceFile::Technique::parse(core::XLexer& lex)
{
	using namespace render;

	core::XLexToken token;

	flags_.Clear();

	src_.color = BlendType::SRC_ALPHA;
	dst_.color = BlendType::INV_SRC_ALPHA;

	// lots of pairs :D !
	while (lex.ReadToken(token))
	{
		if (token.isEqual("}"))
			break;

		core::StackString512 key, value;

		// parse a key / value pair
		key.append(token.begin(), token.end());
		if (!lex.ReadTokenOnLine(token)) {
			X_ERROR("Shader", "unexpected EOF while reading technique, Line: %i", token.GetLine());
			return false;
		}
		value.append(token.begin(), token.end());

		// humm we only have a fixed number of valid values.
		// so lets just check them !
		if (key.isEqual("name"))
		{
			name_ = value.c_str();
			flags_.Set(TechniquePrams::NAME);
		}
		else if (key.isEqual("vertex_shader"))
		{
			vertex_func_ = value.c_str();
			flags_.Set(TechniquePrams::VERTEX_FNC);
		}
		else if (key.isEqual("pixel_shader"))
		{
			pixel_func_ = value.c_str();
			flags_.Set(TechniquePrams::PIXEL_FNC);
		}
		else if (key.isEqual("cull_mode"))
		{
			// none, front, back
			if (value.isEqual("none"))
				this->cullMode_ = CullMode::NONE;
			else if (value.isEqual("front"))
				this->cullMode_ = CullMode::FRONT;
			else if (value.isEqual("back"))
				this->cullMode_ = CullMode::BACK;
			else {
				X_WARNING("Shader", "invalid 'cull_mode' value, possible values: none/front/back");
			}
		}
		else if (key.isEqual("depth_test"))
		{
			//   NEVER, LESS, EQUAL, LESS_EQUAL, GREATER, NOT_EQUAL, GREATER_EQUAL, ALWAYS 
			if (value.isEqual("less_equal"))
				state_ |= States::DEPTHFUNC_LEQUAL;
			else if (value.isEqual("equal"))
				state_ |= States::DEPTHFUNC_EQUAL;
			else if (value.isEqual("greater"))
				state_ |= States::DEPTHFUNC_GREAT;
			else if (value.isEqual("less"))
				state_ |= States::DEPTHFUNC_LESS;
			else if (value.isEqual("greater_equal"))
				state_ |= States::DEPTHFUNC_GEQUAL;
			else if (value.isEqual("not_equal"))
				state_ |= States::DEPTHFUNC_NOTEQUAL;
			else if (value.isEqual("always"))
			{
				state_ |= States::NO_DEPTH_TEST;
			}
			else
			{
				X_WARNING("Shader", "invalid 'depth_test' value, possible values: "
					"less_equal/equal/greater/less/greater_equal/not_equal");
			}
		}
		else if (key.isEqual("depth_write"))
		{
			// true false.
			if (value.isEqual("true"))
				this->depth_write_ = true;
			else if (value.isEqual("false"))
				this->depth_write_ = false;
			else {
				X_WARNING("Shader", "invalid 'depth_write' value, possible values: true/false");
			}
		}
		else if (key.isEqual("wireframe"))
		{
			if (value.isEqual("true")) {
				state_ |= States::WIREFRAME;
			}
		}

		// we could have blend functions.
		else if (src_.ParseBlendInfo("src_blend", key, value) ||
			dst_.ParseBlendInfo("dst_blend", key, value))
		{
			// valid.
		}
		else
		{
			X_ERROR("Shader", "unknown technique param: %s", key.c_str());
		}
	}

	// check we have all the required shit.
	// they can be in any order so we check now.
	if (!flags_.IsSet(TechniquePrams::NAME)) {
		X_ERROR("Shader", "technique missing required param: name");
		return false;
	}
	if (!flags_.IsSet(TechniquePrams::VERTEX_FNC)) {
		X_ERROR("Shader", "technique missing required param: vertex_shader");
		return false;
	}
	if (!flags_.IsSet(TechniquePrams::PIXEL_FNC)) {
		X_ERROR("Shader", "technique missing required param: pixel_shader");
		return false;
	}


	if (depth_write_) {
		state_.Set(render::States::DEPTHWRITE);
	}

	// defaults
	if (src_.color == BlendType::INVALID) {
		src_.color = BlendType::SRC_ALPHA;
	}

	if (dst_.color == BlendType::INVALID) {
		dst_.color = BlendType::INV_SRC_ALPHA;
	}

	// build the state.
	switch (src_.color)
	{
		case BlendType::ZERO:
			state_.Set(render::States::BLEND_SRC_ZERO);
			break;
		case BlendType::ONE:
			state_.Set(render::States::BLEND_SRC_ONE);
			break;
		case BlendType::DEST_COLOR:
			state_.Set(render::States::BLEND_SRC_DEST_COLOR);
			break;
		case BlendType::INV_DEST_COLOR:
			state_.Set(render::States::BLEND_SRC_INV_DEST_COLOR);
			break;
		case BlendType::SRC_ALPHA:
			state_.Set(render::States::BLEND_SRC_SRC_ALPHA);
			break;
		case BlendType::INV_SRC_ALPHA:
			state_.Set(render::States::BLEND_SRC_INV_SRC_ALPHA);
			break;
		case BlendType::DEST_ALPHA:
			state_.Set(render::States::BLEND_SRC_DEST_ALPHA);
			break;
		case BlendType::INV_DEST_ALPHA:
			state_.Set(render::States::BLEND_SRC_INV_DEST_ALPHA);
			break;
		case BlendType::SRC_ALPHA_SAT:
			state_.Set(render::States::BLEND_SRC_ALPHA_SAT);
			break;

		default:
			X_NO_SWITCH_DEFAULT_ASSERT;
	}

	switch (dst_.color)
	{
		case BlendType::ZERO:
			state_.Set(render::States::BLEND_DEST_ZERO);
			break;
		case BlendType::ONE:
			state_.Set(render::States::BLEND_DEST_ONE);
			break;
		case BlendType::SRC_COLOR:
			state_.Set(render::States::BLEND_DEST_SRC_COLOR);
			break;
		case BlendType::INV_SRC_COLOR:
			state_.Set(render::States::BLEND_DEST_INV_SRC_COLOR);
			break;
		case BlendType::SRC_ALPHA:
			state_.Set(render::States::BLEND_DEST_SRC_ALPHA);
			break;
		case BlendType::INV_SRC_ALPHA:
			state_.Set(render::States::BLEND_DEST_INV_SRC_ALPHA);
			break;
		case BlendType::DEST_ALPHA:
			state_.Set(render::States::BLEND_DEST_DEST_ALPHA);
			break;
		case BlendType::INV_DEST_ALPHA:
			state_.Set(render::States::BLEND_DEST_INV_DEST_ALPHA);
			break;

		default:
			X_NO_SWITCH_DEFAULT_ASSERT;
	}

	// did we reach EOF before close brace?
	if (!token.isEqual("}")) {
		X_ERROR("Shader", "technique missing closing brace");
		return false;
	}

	return processName();
}


bool ShaderSourceFile::Technique::processName(void)
{
	const char* pBrace, *pCloseBrace;
	if ((pBrace = name_.find('(')) != nullptr)
	{
		// if we find a () 
		// we have diffrent compile macro's for the 
		// technique
		pCloseBrace = name_.find(')');
		if (pCloseBrace < pBrace)
		{
			X_ERROR("Shader", "invalid name for shader: %s", name_.c_str());
			return false;
		}

		core::StackString512 flags(pBrace + 1, pCloseBrace);
		core::StackString512 temp;

		if (flags.isNotEmpty())
		{
			core::StringTokenizer<char> tokens(flags.begin(), flags.end(), ',');
			core::StringRange<char> flagName(nullptr, nullptr);

			while (tokens.ExtractToken(flagName))
			{
				// valid tech flag?
				temp.set(flagName.GetStart(), flagName.GetEnd());
				if (!TechFlagFromStr(temp.c_str(), techFlags_))
				{
					X_WARNING("Shader", "not a valid tech flag: %s", temp.c_str());
				}
			}
		}

		// fix name.
		name_ = name_.substr(nullptr, pBrace);

	}
	return true;
}


ShaderSourceFile* XShaderManager::loadShaderFile(const char* name, bool reload)
{
	X_ASSERT_NOT_NULL(name);

	SourceFile* pfile;
	ShaderSourceFile* pShaderSource = nullptr;
	core::StackString512 sourceFileName;

	if ((pfile = loadRawSourceFile(name, reload)) != nullptr)
	{
		core::XLexer lex(pfile->fileData.begin(), pfile->fileData.end());
		core::XLexToken token;

		lex.setFlags(LexFlag::ALLOWPATHNAMES);

		if (lex.SkipUntilString("shader"))
		{
			if (!lex.ReadToken(token))
			{
				X_ERROR("Shader", "unexpected EOF");
				return nullptr;
			}

			if (!token.isEqual("{"))
			{
				X_ERROR("Shader", "expected { on line: %i", token.GetLine());
			}

			{
				if (!lex.ExpectTokenString("source"))
					return nullptr;

				// read the source file name.
				if (!lex.ReadToken(token))
				{
					X_ERROR("Shader", "unexpected EOF");
					return nullptr;
				}

				sourceFileName = StackString512(token.begin(), token.end());
			}

			// valid?
			if (sourceFileName.isEmpty())
			{
				X_ERROR("Shader", "invalid source name Line: %i", token.GetLine());
				return nullptr;
			}
			else
			{
				sourceFileName.toLower();
			}

			{
				if (!lex.ExpectTokenString("techniques"))
					return nullptr;

				if (!lex.ExpectTokenString("{"))
					return nullptr;

				{
					pShaderSource = X_NEW_ALIGNED(ShaderSourceFile, g_rendererArena, "ShaderSourceFile", 8);

					while (lex.ReadToken(token))
					{
						if (token.isEqual("}"))
							break;
						if (!token.isEqual("{")) {
							X_ERROR("Shader", "expected { on line: %i", token.GetLine());
							X_DELETE(pShaderSource,g_rendererArena);
							return nullptr;
						}

						// read a technique
						ShaderSourceFile::Technique tech;
						
						if (!tech.parse(lex))
						{
							X_ERROR("Shader", "failed to parse tech");
							X_DELETE(pShaderSource, g_rendererArena);
							return nullptr;
						}


						pShaderSource->techniques_.append(tech);
					}
				}

				if (!lex.ExpectTokenString("}")) {
					X_ERROR("Shader", "missing closing brace");
					X_DELETE( pShaderSource, g_rendererArena);
					pShaderSource = nullptr;
				}
			}

		}
		else
		{
			X_ERROR("Shader", "missing 'shader' decariation");
		}

	}

	if (pShaderSource)
	{
		pShaderSource->pHlslFile_ = loadRawSourceFile(sourceFileName.c_str());

		if (!pShaderSource->pHlslFile_) {
			X_DELETE( pShaderSource, g_rendererArena);
			return nullptr;
		}

		// add the refrences.
		for (auto f : pShaderSource->pHlslFile_->includedFiles) {
			f->refrences.insert(core::string(name));
		}
		pShaderSource->pHlslFile_->refrences.insert(core::string(name));


		pShaderSource->pFile_ = pfile;
		pShaderSource->name_ = name;
		// don't combine these, I want to check if just the .shader has changed.
		// seprate to the .hlsl source.
		pShaderSource->sourceCrc32_ = pfile->sourceCrc32;
		pShaderSource->hlslSourceCrc32_ = pCrc32_->Combine(pfile->sourceCrc32,
			pShaderSource->pHlslFile_->sourceCrc32, 
			safe_static_cast<uint32_t,size_t>(pShaderSource->pHlslFile_->fileData.size()));

	}

	return pShaderSource;
}

bool XShaderManager::sourceToString(core::string& str, const char* name)
{
	SourceFile* file = loadRawSourceFile(name);

	if (file)
	{
		for (auto f : file->includedFiles)
		{
			str.append(f->fileData);
			str.append("\r\n");
		}

		str.append(file->fileData);

		return true;
	}
	return false;
}

XShader* XShaderManager::forName(const char* name)
{
	core::Path<char> temp(name);
	temp.toLower();
	return loadShader(temp.c_str());
}

SourceFile* XShaderManager::loadRawSourceFile(const char* name, bool reload)
{
	X_ASSERT_NOT_NULL(name);

	// already loded o.o?
	
	ShaderSourceMap::iterator it = Sourcebin_.find(X_CONST_STRING(name));
	SourceFile* pfile = nullptr;

	if (it != Sourcebin_.end())
	{
		pfile = it->second;

		if (!reload) {
			return pfile;
		}
	}

	// fixed relative folder.
	core::Path<char> path("shaders/");
	path.setFileName(name);
	if(path.extension() == path.begin())
		path.setExtension("shader");

	core::XFileScoped file;

	if (file.openFile(path.c_str(), fileMode::READ))
	{
		size_t size = safe_static_cast<size_t, uint64_t>(file.remainingBytes());

		// load into a string for now!
		core::string str;
		str.resize(size);

		uint32_t str_len = safe_static_cast<uint32_t, size_t>(size);

		if (file.read((void*)str.data(), str_len) == str_len)
		{
			// tickle my pickle?
			// check the crc.
			uint32_t crc32 = pCrc32_->GetCRC32(str.data());

			if (pfile)
			{
				// if we reloaded the file and crc32 is same
				// don't both reparsing includes
				if (pfile->sourceCrc32 == crc32)
					return pfile;

				pfile->sourceCrc32 = crc32;
				pfile->includedFiles.clear();
				pfile->fileData = str;

				// load any files it includes.
				ParseIncludesAndPrePro_r(pfile, pfile->includedFiles, reload);

				return pfile;
			}
			else
			{

				SourceFile* data = X_NEW_ALIGNED(SourceFile,g_rendererArena, "SourceFile", 8);
				data->name = name;
				data->fileName = path.fileName();
				data->fileData = str;
				data->sourceCrc32 = crc32;

				Sourcebin_.insert(std::make_pair(data->fileName, data));
			
				// load any files it includes.
				ParseIncludesAndPrePro_r(data, data->includedFiles);
				
				return data;
			}
		}
	}

	return nullptr;
}

void XShaderManager::ParseIncludesAndPrePro_r(SourceFile* file,
	core::Array<SourceFile*>& includedFiles, bool reload)
{
	X_ASSERT_NOT_NULL(file);

	core::string::const_str pos = nullptr;
	pos = file->fileData.find("#include");

	if (pos)
	{
		core::XLexer lexer(file->fileData.begin(), file->fileData.end());
		core::XLexToken token;
		core::StackString512 fileName; 

		lexer.setFlags(LexFlag::ALLOWPATHNAMES);

		while (lexer.SkipUntilString("#"))
		{
			fileName.clear();
			PrePro prepro;

			if (lexer.ReadTokenOnLine(token))
			{
				// check if it's a valid prepro type.
				if (PreProFromStr(token, prepro.type))
				{
					if (prepro.type == PreProType::Include)
					{
						const char* start = token.begin() - 1;
						if (lexer.ReadTokenOnLine(token))
						{
							// get the file name, then remove it from the buffer
							// to stop Dx compiler trying to include it.
							fileName.set(token.begin(), token.end());
							memset((char*)start, ' ', (token.end() - start) + 1);
						}

						// you silly hoe!
						if (fileName.isEmpty())
						{
							X_WARNING("Shader", "invalid #include in: \"%s\" line: %i", 
								file->name.c_str(), token.GetLine());
							return;
						}

						// all source names tolower for reload reasons.
						fileName.toLower();

						// load it PLZ.
						SourceFile* childFile = loadRawSourceFile(fileName.c_str(), reload);
						if (childFile)
						{
							// is this file already included in the tree?
							if (std::find(includedFiles.begin(), includedFiles.end(), childFile)
								== includedFiles.end())
							{
								// check if for includes.
								ParseIncludesAndPrePro_r(childFile, includedFiles);

								// add the include files crc to this one.
								// only after parsing for child includes so that
								// they are included.
								file->sourceCrc32 = pCrc32_->Combine(file->sourceCrc32,
									childFile->sourceCrc32,
									safe_static_cast<uint32_t, size_t>(childFile->fileData.length()));


								includedFiles.append(childFile);
							}
							else
							{
								X_ERROR("Shader", "Recursive file #include for: \"%s\" in shader \"%s\" line: %i", 
									fileName.c_str(), file->name.c_str(), token.GetLine());
							}
						}
						else
						{
							X_WARNING("Shader", "File not found: \"%s\"", fileName.c_str());
						}


					}
					else
					{
						// which ones do i care about :|
						// ifdef only tbh, for IL
						if (prepro.type == PreProType::IfDef)
						{
							core::StackString512 ifDefValue;
							if (lexer.ReadTokenOnLine(token))
							{
								if (token.length() > 3) // IL_
								{
									ifDefValue.set(token.begin(), token.end());
									ifDefValue.trim(); // remove white spaces
									// starts with IL_
									if (ifDefValue.findCaseInsen("IL_") == ifDefValue.begin())
									{
										if (!ILFlagFromStr(ifDefValue.begin() + 3, file->ILFlags))
										{
											X_ERROR("Shader", "invalid InputLayout prepro in shader: % value: %s",
												file->name.c_str(), ifDefValue.c_str());
										}
									}
								}
								else
								{
									// dont care about these.
								}
							}
						}
					}
				}
				else
				{
					// just make use of this buffer
					fileName.set(token.begin(), token.end()); 
					X_ERROR("Shader", "Invalid prepro in shader source: %s", fileName.c_str());
					continue;
				}
			}

		}

	}
}


void XShaderManager::writeSourceToFile(core::XFile* f, const SourceFile* source)
{
	f->printf("\n// ======== %s ========\n\n", source->fileName.c_str());
	f->write(source->fileData.c_str(), (uint32_t)source->fileData.length());
}



XTextureResource* textureResourceForName(const char* name)
{
	XTextureResource* pTex = X_NEW_ALIGNED(XTextureResource, g_rendererArena, "ShaderTextureRes", 8);

	pTex->name = name;
	pTex->pITex = texture::XTexture::FromName(name, texture::TextureFlags::DONT_STREAM);

	return pTex;
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
			pRes->pTextures[i] = textureResourceForName(res.name);
		}
	}

	return pRes;
}

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
	if (render::gRenDev)
	{
		this->RT_Release();
	}
	else
	{
		X_ASSERT_UNREACHABLE();
	}
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

/*





*/


X_NAMESPACE_END
