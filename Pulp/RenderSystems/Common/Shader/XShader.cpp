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


XShader* XShaderManager::m_DefaultShader = nullptr;
XShader* XShaderManager::m_DebugShader = nullptr;
XShader* XShaderManager::m_FixedFunction = nullptr;
XShader* XShaderManager::m_Font = nullptr;
XShader* XShaderManager::m_Gui = nullptr;
XShader* XShaderManager::m_DefferedShader = nullptr;
XShader* XShaderManager::m_DefferedShaderVis = nullptr;


XShaderManager::XShaderManager() : 
Sourcebin(nullptr),
shaders(nullptr, 256), 
pCrc32(nullptr)
{


}

XShaderManager::~XShaderManager()
{
	ShaderSourceMap::iterator it = Sourcebin.begin();

	for (; it != Sourcebin.end(); ++it)
	{
		X_DELETE(it->second, g_rendererArena);
	}

	Sourcebin.clear();
}

// -------------------------------------------


XShader::XShader() :
	techs(g_rendererArena)
{
	sourceCrc32 = 0;
	vertexFmt = VertexFormat::P3F_T2F_C4B;

	pHlslFile = nullptr;
}

XShader::~XShader()
{
	size_t i, numTecs;
	numTecs = techs.size();
	for (i = 0; i < numTecs; i++)
	{
		techs[i].pPixelShader->release();
		techs[i].pVertexShader->release();
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
	render::gRenDev->m_ShaderMan.listShaders();
}

void Cmd_ListShaderSources(core::IConsoleCmdArgs* pArgs)
{
	render::gRenDev->m_ShaderMan.listShaderSources();
}

bool XShaderManager::Init(void)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pCore);
	X_ASSERT_NOT_NULL(gEnv->pHotReload);
	X_ASSERT_NOT_NULL(g_rendererArena);

	pCrc32 = gEnv->pCore->GetCrc32();

	Sourcebin.setArena(g_rendererArena, 64);
	shaders.setArena(g_rendererArena, 256);

	texture::XTexture::init();

	loadCoreShaders();

	// hotreload support.
	gEnv->pHotReload->addfileType(this, "hlsl");
	gEnv->pHotReload->addfileType(this, "inc");
	gEnv->pHotReload->addfileType(this, "shader");
	gEnv->pHotReload->addfileType(this, "fxcb");

	ADD_COMMAND("shader_list", Cmd_ListShaders, core::VarFlag::SYSTEM, "lists the loaded shaders");
	ADD_COMMAND("shader_listsourcebin", Cmd_ListShaderSources, core::VarFlag::SYSTEM, "lists the loaded shaders sources");

	return true;
}

bool XShaderManager::Shutdown(void)
{
	X_LOG0("ShadersManager", "Shutting down");

	gEnv->pHotReload->addfileType(nullptr, "hlsl");
	gEnv->pHotReload->addfileType(nullptr, "inc");
	gEnv->pHotReload->addfileType(nullptr, "shader");
	gEnv->pHotReload->addfileType(nullptr, "fxcb");
	
	freeCoreShaders();
	freeSourcebin();

	// free the shaders.
	XResourceContainer::ResourceItor it = shaders.begin();
	XResourceContainer::ResourceItor end = shaders.end();
	for (; it != end; )
	{
		XShader* pShader = (XShader*)it->second;

		++it;

		if (!pShader)
			continue;

		X_WARNING("ShadersManager", "\"%s\" was not deleted", pShader->getName());

	//	pTex->forceRelease();
	}

	shaders.free();

	return true;
}


bool XShaderManager::OnFileChange(const char* name)
{
	const char* ext = core::strUtil::FileExtension(name);

	// this is just a cache update ignore this.
	if (core::strUtil::IsEqualCaseInsen(ext, "fxcb")) {
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
		core::Path temp(name);
		temp.toLower(); // all source is lower case

		ShaderSourceMap::const_iterator it = Sourcebin.find(temp.fileName());
		if (it != Sourcebin.end())
		{
			// reload the source file.
			loadRawSourceFile(temp.fileName(), true);

			SourceFile* src = it->second;
			for (auto name : src->refrences)
			{
				reloadShader(name.c_str());
			}
		}
		else
		{
			// log as not found.
			X_WARNING("Shader", "\"%s\" not used, skippin reload", name);
		}
	}
	else
	{
		core::Path temp(name);
		temp.setExtension("");
		temp.toLower();

		reloadShader(temp.fileName());
	}

	return true;
}

XShader* XShaderManager::reloadShader(const char* name)
{
	X_ASSERT_NOT_NULL(name);

	XShader* shader = nullptr;
	ShaderSourceFile* source = nullptr;
	size_t i, numTecs;

	// already loaded?
	shader = (XShader*)shaders.findAsset(name);

	if (shader)
	{
		
		// reload the shader file.
		source = loadShaderFile(name, true);
		if (source)
		{
			// we don't reload the .shader if the source is the same.
			if (shader->sourceCrc32 != source->sourceCrc32)
			{
				X_LOG0("Shader", "reloading shader: %s", name);

				numTecs = source->numTechs();

				// might be more techs etc..
				shader->techs.resize(numTecs);
				shader->sourceCrc32 = source->sourceCrc32;
				shader->hlslSourceCrc32 = source->hlslSourceCrc32;

				for (i = 0; i < numTecs; i++)
				{
					XShaderTechnique& tech = shader->techs[i];
					ShaderSourceFile::Technique& srcTech = source->techniques[i];

					tech.name = srcTech.name;
					tech.nameHash = core::StrHash(srcTech.name);
					// Blend info
					tech.src = srcTech.src;
					tech.dst = srcTech.dst;
					// State
					tech.state = srcTech.state;
					// Cullmode
					tech.cullMode = srcTech.cullMode;

					// create the hardware shaders.
					// dose nothing if already loaded.
					tech.pVertexShader = XHWShader::forName(name, srcTech.vertex_func,
						source->pHlslFile->fileName.c_str(), ShaderType::Vertex, source->pHlslFile->sourceCrc32);

					tech.pPixelShader = XHWShader::forName(name, srcTech.pixel_func,
						source->pHlslFile->fileName.c_str(), ShaderType::Pixel, source->pHlslFile->sourceCrc32);
				}
			}
			else if (shader->hlslSourceCrc32 != source->hlslSourceCrc32)
			{
				X_LOG0("Shader", "reloading shader source: %s", shader->pHlslFile->name.c_str());
				if (source)
				{
					numTecs = shader->numTechs();

					// update crc
					shader->hlslSourceCrc32 = source->hlslSourceCrc32;

					for (i = 0; i < numTecs; i++)
					{
						XShaderTechnique& tech = shader->techs[i];

						// these have not changed.
						// it's safe to pass the pointers.
						const char* vertEntry = tech.pVertexShader->getEntryPoint();
						const char* pixelEntry = tech.pPixelShader->getEntryPoint();

						tech.pVertexShader = XHWShader::forName(name, vertEntry,
							source->pHlslFile->fileName.c_str(), ShaderType::Vertex,
							source->pHlslFile->sourceCrc32);

						tech.pPixelShader = XHWShader::forName(name, pixelEntry,
							source->pHlslFile->fileName.c_str(), ShaderType::Pixel,
							source->pHlslFile->sourceCrc32);

					}
				}
			}
			else
			{		
				uint32_t lastCrc32 = shader->pHlslFile->sourceCrc32;

				SourceFile* Hlslsource = loadRawSourceFile(shader->pHlslFile->fileName.c_str(), true);

				if (source)
				{
					if (lastCrc32 != Hlslsource->sourceCrc32)
					{
						X_LOG0("Shader", "reloading shader source: %s", shader->pHlslFile->name.c_str());

						// the shaders source has changed.
						// we end up here typically when a source file included by 
						// this .shader main .hlsl forcing a reload of the .shader
						// so that the main source can be checked.
						// if we are in this scope we need to recompile all the hardware shaders.
						numTecs = shader->numTechs();

						for (i = 0; i < numTecs; i++)
						{
							XShaderTechnique& tech = shader->techs[i];

							// these have not changed.
							// it's safe to pass the pointers.
							const char* vertEntry = tech.pVertexShader->getEntryPoint();
							const char* pixelEntry = tech.pPixelShader->getEntryPoint();

							tech.pVertexShader = XHWShader::forName(name, vertEntry,
								source->pHlslFile->fileName.c_str(), ShaderType::Vertex,
								source->pHlslFile->sourceCrc32);

							tech.pPixelShader = XHWShader::forName(name, pixelEntry,
								source->pHlslFile->fileName.c_str(), ShaderType::Pixel,
								source->pHlslFile->sourceCrc32);

						}

					}
					else
					{
						X_LOG0("Shader", "shader source: %s has not changed, reload skipped", shader->pHlslFile->name.c_str());
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
	m_DefaultShader = createShader("default");
//	m_DebugShader = forName("gui");
	m_FixedFunction = forName("ffe");
	m_Font = forName("font");
	m_Gui = forName("gui");
	m_DefferedShader = forName("deffered");
	m_DefferedShaderVis = forName("defferedVis");

	return true;
}



bool XShaderManager::freeCoreShaders(void)
{
	if (m_DefaultShader)
		m_DefaultShader->release();
//	if (m_DebugShader)
//		m_DebugShader->release();
	if (m_FixedFunction)
		m_FixedFunction->release();
	if (m_Font)
		m_Font->release();
	if (m_Gui)
		m_Gui->release();
	if (m_DefferedShader)
		m_DefferedShader->release();
	if (m_DefferedShaderVis)
		m_DefferedShaderVis->release();

	return true;
}

bool XShaderManager::freeSourcebin(void)
{
	ShaderSourceMap::iterator it = Sourcebin.begin();;
	for (; it != Sourcebin.end(); ++it)
	{
		X_DELETE(it->second, g_rendererArena);
	}

	Sourcebin.free();
	return true;
}

void XShaderManager::listShaders(void)
{
	render::XRenderResourceContainer::ResourceConstItor it = shaders.begin();
	XShader* pShader;

	X_LOG0("Shader", "------------- ^8Shaders(%i)^7 -------------", shaders.size());
	X_LOG_BULLET;

	for (; it != shaders.end(); ++it)
	{
		pShader = (XShader*)it->second;

		X_LOG0("Shader", "Name: ^2\"%s\"^7 tecs: %i crc: ^10x%08x^7 vertexFmt: %s",
			pShader->name.c_str(),
			pShader->techs.size(),
			pShader->sourceCrc32,
			VertexFormat::toString(pShader->vertexFmt));
	}
	X_LOG0("Shader", "------------ ^8Shaders End^7 -------------");
}

void XShaderManager::listShaderSources(void)
{
	ShaderSourceMap::const_iterator it = Sourcebin.begin();
	const SourceFile* pSource;

	X_LOG0("Shader", "--------- ^8Shader Sources(%i)^7 ---------", Sourcebin.size());
	X_LOG_BULLET;

	for (; it != Sourcebin.end(); ++it)
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
	shader = (XShader*)shaders.findAsset(name);

	if (shader)
		return shader;
	
	// load the source file containing the techniques info.
	ShaderSourceFile* source = loadShaderFile(name);

	if (source)
	{
		size_t i, numTecs;

		numTecs = source->numTechs();

		shader = createShader(name);
		shader->techs.resize(source->numTechs());
		shader->sourceCrc32 = source->sourceCrc32;
		shader->hlslSourceCrc32 = source->hlslSourceCrc32;
		shader->pHlslFile = source->pHlslFile;

		// I might use only the HLSL crc, do .shader changes matter?
		// for now use hlsl + .shader
		// uint32_t crc = source->pHlslFile->sourceCrc32;

		for (i = 0; i < numTecs; i++)
		{
			XShaderTechnique& tech = shader->techs[i];
			ShaderSourceFile::Technique& srcTech = source->techniques[i];

			tech.name = srcTech.name;
			tech.nameHash = core::StrHash(srcTech.name);
			// Blend info
			tech.src = srcTech.src;
			tech.dst = srcTech.dst;
			// State
			tech.state = srcTech.state;
			// Cullmode
			tech.cullMode = srcTech.cullMode;

			// create the hardware shaders.
			tech.pVertexShader = XHWShader::forName(name, srcTech.vertex_func,
				source->pHlslFile->fileName.c_str(), ShaderType::Vertex, source->pHlslFile->sourceCrc32);

			tech.pPixelShader = XHWShader::forName(name, srcTech.pixel_func,
				source->pHlslFile->fileName.c_str(), ShaderType::Pixel, source->pHlslFile->sourceCrc32);

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
	pShader = (XShader*)shaders.findAsset(name);
	
	if (pShader)
	{
		pShader->addRef();
	}
	else
	{
		pShader = X_NEW_ALIGNED(XShader, g_rendererArena, "Shader", 16);
		pShader->name = name;
		shaders.AddAsset(name, pShader);
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

bool ShaderSourceFile::Technique::parse(core::XLexer& lex)
{
	using namespace render;

	core::XLexToken token;

	flags.Clear();

	src.color = BlendType::SRC_ALPHA;
	dst.color = BlendType::INV_SRC_ALPHA;

	// lots of pairs :D !
	while (lex.ReadToken(token))
	{
		if (token.isEqual("}"))
			break;

		core::StackString512 key, value;

		// parse a key / value pair
		key.append(token.begin(), token.end());
		if (!lex.ReadTokenOnLine(token)) {
			X_ERROR("Shader", "unexpected EOF while reading technique, Line: %i", token.line);
			return false;
		}
		value.append(token.begin(), token.end());

		// humm we only have a fixed number of valid values.
		// so lets just check them !
		if (key.isEqual("name"))
		{
			name = value.c_str();
			flags.Set(TechniquePrams::NAME);
		}
		else if (key.isEqual("vertex_shader"))
		{
			vertex_func = value.c_str();
			flags.Set(TechniquePrams::VERTEX_FNC);
		}
		else if (key.isEqual("pixel_shader"))
		{
			pixel_func = value.c_str();
			flags.Set(TechniquePrams::PIXEL_FNC);
		}
		else if (key.isEqual("cull_mode"))
		{
			// none, front, back
			if (value.isEqual("none"))
				this->cullMode = CullMode::NONE;
			else if (value.isEqual("front"))
				this->cullMode = CullMode::FRONT;
			else if (value.isEqual("back"))
				this->cullMode = CullMode::BACK;
			else {
				X_WARNING("Shader", "invalid 'cull_mode' value, possible values: none/front/back");
			}
		}
		else if (key.isEqual("depth_test"))
		{
			//   NEVER, LESS, EQUAL, LESS_EQUAL, GREATER, NOT_EQUAL, GREATER_EQUAL, ALWAYS 
			if (value.isEqual("less_equal"))
				state |= States::DEPTHFUNC_LEQUAL;
			else if (value.isEqual("equal"))
				state |= States::DEPTHFUNC_EQUAL;
			else if (value.isEqual("greater"))
				state |= States::DEPTHFUNC_GREAT;
			else if (value.isEqual("less"))
				state |= States::DEPTHFUNC_LESS;
			else if (value.isEqual("greater_equal"))
				state |= States::DEPTHFUNC_GEQUAL;
			else if (value.isEqual("not_equal"))
				state |= States::DEPTHFUNC_NOTEQUAL;
			else if (value.isEqual("always"))
			{
				state |= States::NO_DEPTH_TEST;
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
				this->depth_write = true;
			else if(value.isEqual("false"))
				this->depth_write = false;
			else {
				X_WARNING("Shader", "invalid 'depth_write' value, possible values: true/false");
			}
		}
		else if (key.isEqual("wireframe"))
		{
			if (value.isEqual("true"))
				state |= States::WIREFRAME;
		}

		// we could have blend functions.
		else if (src.ParseBlendInfo("src_blend", key, value) || dst.ParseBlendInfo("dst_blend", key, value))
		{
			// valid.
		}
		else
		{
			X_ERROR("Shader", "unkown technique param: %s", key.c_str());
		}
	}

	// check we have all the required shit.
	// they can be in any order so we check now.
	if (!flags.IsSet(TechniquePrams::NAME)) {
		X_ERROR("Shader", "technique missing required param: name");
		return false;
	}
	if (!flags.IsSet(TechniquePrams::VERTEX_FNC)) {
		X_ERROR("Shader", "technique missing required param: vertex_shader");
		return false;
	}
	if (!flags.IsSet(TechniquePrams::PIXEL_FNC)) {
		X_ERROR("Shader", "technique missing required param: pixel_shader");
		return false;
	}


	if (depth_write)
		state.Set(render::States::DEPTHWRITE);


	// defaults
	if (src.color == BlendType::INVALID)
		src.color = BlendType::SRC_ALPHA;

	if (dst.color == BlendType::INVALID)
		dst.color = BlendType::INV_SRC_ALPHA;


	// build the state.
	switch (src.color)
	{
		case BlendType::ZERO:
			state.Set(render::States::BLEND_SRC_ZERO);
			break;
		case BlendType::ONE:
			state.Set(render::States::BLEND_SRC_ONE);
			break;
		case BlendType::DEST_COLOR:
			state.Set(render::States::BLEND_SRC_DEST_COLOR);
			break;
		case BlendType::INV_DEST_COLOR:
			state.Set(render::States::BLEND_SRC_INV_DEST_COLOR);
			break;
		case BlendType::SRC_ALPHA:
			state.Set(render::States::BLEND_SRC_SRC_ALPHA);
			break;
		case BlendType::INV_SRC_ALPHA:
			state.Set(render::States::BLEND_SRC_INV_SRC_ALPHA);
			break;
		case BlendType::DEST_ALPHA:
			state.Set(render::States::BLEND_SRC_DEST_ALPHA);
			break;
		case BlendType::INV_DEST_ALPHA:
			state.Set(render::States::BLEND_SRC_INV_DEST_ALPHA);
			break;
		case BlendType::SRC_ALPHA_SAT:
			state.Set(render::States::BLEND_SRC_ALPHA_SAT);
			break;
#if X_DEBUG
		default:
			X_ASSERT_UNREACHABLE();
#else
			X_NO_SWITCH_DEFAULT;
#endif
	}

	switch (dst.color)
	{
		case BlendType::ZERO:
			state.Set(render::States::BLEND_DEST_ZERO);
			break;
		case BlendType::ONE:
			state.Set(render::States::BLEND_DEST_ONE);
			break;
		case BlendType::SRC_COLOR:
			state.Set(render::States::BLEND_DEST_SRC_COLOR);
			break;
		case BlendType::INV_SRC_COLOR:
			state.Set(render::States::BLEND_DEST_INV_SRC_COLOR);
			break;
		case BlendType::SRC_ALPHA:
			state.Set(render::States::BLEND_DEST_SRC_ALPHA);
			break;
		case BlendType::INV_SRC_ALPHA:
			state.Set(render::States::BLEND_DEST_INV_SRC_ALPHA);
			break;
		case BlendType::DEST_ALPHA:
			state.Set(render::States::BLEND_DEST_DEST_ALPHA);
			break;
		case BlendType::INV_DEST_ALPHA:
			state.Set(render::States::BLEND_DEST_INV_DEST_ALPHA);
			break;
#if X_DEBUG
		default:
			X_ASSERT_UNREACHABLE();
#else
			X_NO_SWITCH_DEFAULT;
#endif
	}


	// did we reach EOF before close brace?
	if (!token.isEqual("}")) {
		X_ERROR("Shader", "technique missing closing brace");
		return false;
	}
	return true;
}


ShaderSourceFile* XShaderManager::loadShaderFile(const char* name, bool reload)
{
	X_ASSERT_NOT_NULL(name);

	SourceFile* pfile;
	ShaderSourceFile* pShaderSource = nullptr;
	core::StackString512 sourceFileName;

	if (pfile = loadRawSourceFile(name, reload))
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
				X_ERROR("Shader", "expected { on line: %i", token.line);
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
				X_ERROR("Shader", "invalid source name Line: %i", token.line);
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
							X_ERROR("Shader", "expected { on line: %i", token.line);
							X_DELETE(pShaderSource,g_rendererArena);
							return nullptr;
						}

						// read a technique
						ShaderSourceFile::Technique tech;
						tech.parse(lex);

						if (tech.name.find("("))
						{
							// if we find a () 
							// we have diffrent compile macro's for the 
							// technique+

						}

						pShaderSource->techniques.append(tech);
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
		pShaderSource->pHlslFile = loadRawSourceFile(sourceFileName.c_str());

		if (!pShaderSource->pHlslFile) {
			X_DELETE( pShaderSource, g_rendererArena);
			return nullptr;
		}

		// add the refrences.
		for (auto f : pShaderSource->pHlslFile->includedFiles) {
			f->refrences.insert(name);
		}
		pShaderSource->pHlslFile->refrences.insert(name);


		pShaderSource->pFile = pfile;
		pShaderSource->name = name;
		// don't combine these, I want to check if just the .shader has changed.
		// seprate to the .hlsl source.
		pShaderSource->sourceCrc32 = pfile->sourceCrc32;
		pShaderSource->hlslSourceCrc32 = pCrc32->Combine(pfile->sourceCrc32,
			pShaderSource->pHlslFile->sourceCrc32, 
			safe_static_cast<uint32_t,size_t>(pShaderSource->pHlslFile->fileData.size()));

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
			str.append("\n");
		}

		str.append(file->fileData);

		return true;
	}
	return false;
}

XShader* XShaderManager::forName(const char* name)
{
	core::Path temp(name);
	temp.toLower();
	return loadShader(temp.c_str());
}

SourceFile* XShaderManager::loadRawSourceFile(const char* name, bool reload)
{
	X_ASSERT_NOT_NULL(name);

	// already loded o.o?
	
	ShaderSourceMap::iterator it = Sourcebin.find(X_CONST_STRING(name));
	SourceFile* pfile = nullptr;

	if (it != Sourcebin.end())
	{
		pfile = it->second;

		if (!reload)
			return pfile;
	}

	// fixed relative folder.
	core::Path path("shaders/");
	path.setFileName(name);
	if(path.extension() == path.begin())
		path.setExtension("shader");

	core::XFileScoped file;

	if (file.openFile(path.c_str(), fileMode::READ))
	{
		size_t size = file.remainingBytes();

		// load into a string for now!
		core::string str;
		str.resize(size);

		uint32_t str_len = safe_static_cast<uint32_t, size_t>(size);

		if (file.read((void*)str.data(), str_len) == str_len)
		{
			// tickle my pickle?
			// check the crc.
			uint32_t crc32 = pCrc32->GetCRC32(str.data());

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
				ParseIncludeFiles_r(pfile, pfile->includedFiles, reload);

				return pfile;
			}
			else
			{

				SourceFile* data = X_NEW_ALIGNED(SourceFile,g_rendererArena, "SourceFile", 8);
				data->name = name;
				data->fileName = path.fileName();
				data->fileData = str;
				data->sourceCrc32 = crc32;

				Sourcebin.insert(std::make_pair(data->fileName, data));
			
				// load any files it includes.
				ParseIncludeFiles_r(data, data->includedFiles);
				
				return data;
			}
		}
	}

	return nullptr;
}

void XShaderManager::ParseIncludeFiles_r(SourceFile* file,
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

		while (lexer.SkipUntilString("#include"))
		{
			fileName.clear();

			if (lexer.ReadTokenOnLine(token))
			{
				if (token.isEqual("include"))
				{
					const char* start = token.begin() - 1;
					
					if (lexer.ReadTokenOnLine(token))
					{
						fileName = StackString512(token.begin(), token.end());
						memset((char*)start, ' ', (token.end() - start) + 1);
					}			
				}
				else
				{
					// not a valid include
					continue;
				}
			}

			// you silly hoe!
			if (fileName.isEmpty())
			{
				X_WARNING("Shader", "invalid #include in: \"%s\" line: %i", file->name.c_str(), token.line);
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
					ParseIncludeFiles_r(childFile, includedFiles);
					
					// add the include files crc to this one.
					// only after parsing for child includes so that
					// they are included.
					file->sourceCrc32 = pCrc32->Combine(file->sourceCrc32, 
						childFile->sourceCrc32, 
						safe_static_cast<uint32_t, size_t>(childFile->fileData.length()));


					includedFiles.append(childFile);
				}
				else
				{
					X_ERROR("Shader", "Recursive file #include for: \"%s\" in shader \"%s\" line: %i", fileName.c_str(), file->name.c_str(), token.line);
				}
			}
			else
			{
				X_WARNING("Shader", "File not found: \"%s\"", fileName.c_str());
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
	if (render::gRenDev && render::gRenDev->rThread())
		render::gRenDev->rThread()->RC_ReleaseShaderResource(this);
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
