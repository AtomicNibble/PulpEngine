#pragma once


#ifndef X_SHADER_H_
#define X_SHADER_H_

#include <IShader.h>
#include <String\StrRef.h>
#include <String\Lexer.h>
#include <String\StringHash.h>

#include <Containers\HashMap.h>
#include <Containers\Array.h>
#include <Util\Flags.h>

#include <../Common/Resources/BaseRenderAsset.h>
#include "XShaderBin.h"

#include <unordered_set>

X_NAMESPACE_DECLARE(core,
	struct IConsoleCmdArgs;
)

X_NAMESPACE_BEGIN(shader)

class XShaderManager;
struct ShaderSourceFile;
struct SourceFile;

#define SHADER_BIND_SAMPLER 0x4000


struct ConstbufType
{
	enum Enum
	{
		PER_FRAME,
		PER_BATCH,
		PER_INSTANCE,
		Num
	};
};


struct BlendInfo
{
	BlendType::Enum color;
	BlendType::Enum alpha;

	bool ParseBlendInfo(const char* name,
		const core::StackString512& key, const core::StackString512& value);
};


X_DECLARE_FLAGS(TechniquePrams) (NAME, VERTEX_FNC, PIXEL_FNC);

// in the shader name space so it's ok to call just: PrePro
X_DECLARE_ENUM(PreProType)(Include, Define, Undef, If, IfDef, IfNDef, Else, EndIF);
X_DECLARE_FLAGS(ILFlag)(Normal, BiNormal, Color);


class XShaderResources : public IRenderShaderResources
{
public:
	XShaderResources();

	// IRenderShaderResources
	virtual ~XShaderResources() X_OVERRIDE;

	virtual void release(void) X_OVERRIDE;

	virtual Color& getDiffuseColor() X_OVERRIDE{ return diffuse; }
	virtual Color& getSpecularColor() X_OVERRIDE{ return spec; }
	virtual Color& getEmissiveColor() X_OVERRIDE{ return emissive; }

	virtual float& getSpecularShininess() X_OVERRIDE{ return specShine; }
	virtual float& getGlow() X_OVERRIDE{ return glow; }
	virtual float& getOpacity() X_OVERRIDE{ return opacity; }

	virtual XTextureResource* getTexture(ShaderTextureIdx::Enum idx) const X_OVERRIDE{
		return pTextures[idx];
	}
	// ~IRenderShaderResources

	X_INLINE bool hasTexture(ShaderTextureIdx::Enum idx) const {
		return pTextures[idx] != nullptr;
	}

	// called from render thread.
	void RT_Release(void);
private:

	void freeAssets(void);

protected:
	X_NO_COPY(XShaderResources);
	X_NO_ASSIGN(XShaderResources);

	friend class XShaderManager;

	// the textures
	shader::XTextureResource* pTextures[ShaderTextureIdx::ENUM_COUNT]; // 8 x 3 = 24

	// 12 * 3 = 36
	Color diffuse;
	Color spec;
	Color emissive;

	// 12
	float specShine;
	float glow;
	float opacity;
};


class XShader;

class XHWShader : public core::XBaseAsset
{
public:
	typedef core::Array<core::string> MacroList;

	XHWShader();

	static XHWShader* forName(const char* shader_name, const char* entry,
		const char* sourceFile, const MacroList& macros,
		ShaderType::Enum type, Flags<ILFlag> ILFlag, uint32_t sourceCrc);

	static const char* getProfileFromType(ShaderType::Enum type);

	X_INLINE const char* getName(void) const {
		return name.c_str();
	}
	X_INLINE const char* getSourceFileName(void) const {
		return sourceFileName.c_str();
	}
	X_INLINE const char* getEntryPoint(void) const {
		return entryPoint.c_str();
	}
	X_INLINE const VertexFormat::Enum getVertexFmt(void) const {
		return vertexFmt;
	}
protected:
	static render::XRenderResourceContainer* pHWshaders;


	core::string name;
	core::string sourceFileName;
	core::string entryPoint;
	uint32_t sourceCrc32; // the crc of the source this was compiled from.

	// set for all shader types.
	VertexFormat::Enum vertexFmt; 
	ShaderType::Enum type; // V / P / G
	XShader* pShader;
};


struct InputLayoutEntry
{
	const char* name;
	ILFlag::Enum flag;
};

struct PreProEntry
{
	const char* name;
	PreProType::Enum type;
};

struct PrePro
{
	PreProType::Enum type;
	core::string expression;
};


// a hlsl
struct SourceFile
{
	friend class XShaderManager;

	SourceFile() :
		sourceCrc32(0),
		includedFiles(g_rendererArena),
		prePros(g_rendererArena)
	{}

protected:
	core::string name;
	core::string fileName;
	core::string fileData;
	core::Array<SourceFile*> includedFiles;
	core::Array<PrePro> prePros;
	std::unordered_set<core::string, core::hash<core::string>> refrences;
	Flags<ILFlag> ILFlags;
	uint32_t sourceCrc32;
};


struct ShaderSourceFile
{
	friend class XShaderManager;

	ShaderSourceFile() :
		sourceCrc32(0),
		techniques(g_rendererArena)
	{}

	struct Technique
	{
		Technique();

		typedef core::Array<core::string> CompileFlagList;

		core::string name;
		core::string vertex_func;
		core::string pixel_func;

		BlendInfo src;
		BlendInfo dst;

		render::CullMode::Enum cullMode;
		bool depth_write;

		render::StateFlag state;
		Flags<TechniquePrams> flags;

		CompileFlagList compileFlags;

		bool parse(core::XLexer& lex);
		bool processName(void);
	};


	X_INLINE size_t numTechs(void) const { return techniques.size(); }

protected:

	core::string name;
	SourceFile* pFile;
	SourceFile* pHlslFile;
	uint32_t sourceCrc32;
	uint32_t hlslSourceCrc32;
	core::Array<Technique> techniques;
};


struct XShaderTechnique
{
	typedef core::Array<core::string> CompileFlagList;

	XShaderTechnique() :
		pVertexShader(nullptr),
		pPixelShader(nullptr),
		pGeoShader(nullptr),
		compileFlags(g_rendererArena)
	{}

	XShaderTechnique& operator=(const ShaderSourceFile::Technique& srcTech);

	core::string name;
	core::StrHash nameHash;
	render::StateFlag state;
	render::CullMode::Enum cullMode;

	BlendInfo src;
	BlendInfo dst;

	// what's the use case for the diffrent vertext formats.
	// a HW shader can only work out it's final vertex format post compile.
	// before that i can only know if it takes normals / tangents / color etc.
	// i set vertex format for rendering.
	// so i need some way to quickly pick the correct hardware shader.
	// based on the current input layout.
	// i think this should be part of a a technique.
	// since it's the same thing, just diffrent input layout.
	// i don't really want to make a array the size of num vertex formats.
	// since i don't know a hardware shaders vertex format till post compile.
	// unless i had some sort of callback.
	XHWShader* pVertexShader;
	XHWShader* pPixelShader;
	XHWShader* pGeoShader;

	// leave at end, not accesed much.
	CompileFlagList compileFlags;
};

class XShader : public IShader, public core::XBaseAsset
{
	friend class XShaderManager;

public:
	XShader();
	~XShader();

	virtual ShaderID getID() X_OVERRIDE{ return XBaseAsset::getID(); }
	virtual const int addRef() X_OVERRIDE{ return XBaseAsset::addRef(); }
	virtual const int release() X_OVERRIDE;

	virtual const char* getName() const X_OVERRIDE{ return name.c_str(); }
	virtual VertexFormat::Enum getVertexFmt() X_OVERRIDE{ return vertexFmt; }

	// D3D Effects interface
	bool FXSetTechnique(const char* name);
	bool FXSetTechnique(const core::StrHash& name);
	bool FXBegin(uint32 *uiPassCount, uint32 nFlags);
	bool FXBeginPass(uint32 uiPass);
	bool FXCommit(const uint32 nFlags);
	bool FXEndPass();
	bool FXEnd();

	bool FXSetVSFloat(const core::StrHash& NameParam, const Vec4f* pVecs, uint32_t numVecs);

private:

	X_INLINE size_t numTechs(void) const { return techs.size(); }

private:

	core::string name;
	uint32_t sourceCrc32;
	uint32_t hlslSourceCrc32;

	SourceFile* pHlslFile;

	VertexFormat::Enum vertexFmt;

	core::Array<XShaderTechnique> techs;
};

class XShaderManager : public core::IXHotReload
{
public:
	XShaderManager();
	~XShaderManager();

	bool Init(void);
	bool Shutdown(void);

	XShader* forName(const char* name);
	SourceFile* loadRawSourceFile(const char* name, bool reload = false);

	bool sourceToString(core::string& str, const char* name);

	// IXHotReload
	virtual bool OnFileChange(const char* name) X_OVERRIDE;
	// ~IXHotReload

	XShaderResources* createShaderResources(const XInputShaderResources& input);

private:
	bool loadCoreShaders(void);
	bool freeCoreShaders(void);
	bool freeSourcebin(void);
	void listShaders(void);
	void listShaderSources(void);


//	bool loadShaderFile(const char* name);
	ShaderSourceFile* loadShaderFile(const char* name, bool reload = false);


	XShader* createShader(const char* name);
	XShader* loadShader(const char* name);
	XShader* reloadShader(const char* name);
	// ParseIncludesAndPrePro_r
	void ParseIncludesAndPrePro_r(SourceFile* file, core::Array<SourceFile*>& includedFiles,
		bool reload = false);


private:
	static void writeSourceToFile(core::XFile* f, const SourceFile* source);

	friend void Cmd_ListShaders(core::IConsoleCmdArgs* pArgs);
	friend void Cmd_ListShaderSources(core::IConsoleCmdArgs* pArgs);

private:
	typedef core::HashMap<core::string, SourceFile*> ShaderSourceMap;

	ShaderSourceMap Sourcebin;
	render::XRenderResourceContainer shaders;

public:
	core::Crc32* pCrc32;

	static XShader* m_DefaultShader;
	static XShader* m_DebugShader;
	static XShader* m_FixedFunction;
	static XShader* m_Font;
	static XShader* m_Gui;
	static XShader* m_DefferedShader;
	static XShader* m_DefferedShaderVis;
	
};

X_NAMESPACE_END


#endif // !X_SHADER_H_