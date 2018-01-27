#pragma once


#include <IRender.h>
#include <IMaterial.h>


#include <Containers\Array.h>

#include <Traits\MemberFunctionTraits.h>
#include <Util\Delegate.h>

X_NAMESPACE_DECLARE(core,
	class XLexer;
	class XLexToken;
	class XParser;
)

X_NAMESPACE_BEGIN(engine)

namespace techset
{


struct StencilState
{
	render::StencilDesc state;
	bool enabled;
};

struct Alias
{
	bool isCode;
	core::string resourceName;
	core::string name;
	core::StrHash nameHash;
};

struct Shader
{
	typedef core::Array<Alias> AliaseArr;

	Shader();

	bool SSave(core::XFile* pFile) const;
	bool SLoad(core::XFile* pFile);

	render::shader::ShaderType::Enum type;
	core::string source;
	core::string entry;
	core::string defines;

	AliaseArr aliases;
};

struct Technique
{
	bool SSave(core::XFile* pFile) const;
	bool SLoad(core::XFile* pFile);

	render::StateDesc state;

	core::string source;
	core::string defines;

	render::shader::ShaderStageFlags stages;
	std::array<Shader, render::shader::ShaderType::ENUM_COUNT - 1> shaders;
};

struct AssManProps
{
	bool SSave(core::XFile* pFile) const;
	bool SLoad(core::XFile* pFile);

	core::string cat; // the assetScript cat.
	core::string title;
	core::string defaultVal;
};


struct Param
{
	Param() = default;
	Param(const Param& oth);

	Param& operator=(const Param& oth);

	bool SSave(core::XFile* pFile) const;
	bool SLoad(core::XFile* pFile);


	ParamType::Enum type;

	// we can store most params in this.
	// bool, int, float1, float2, float4, color
	Vec4f vec4;
	core::string vec4Props[4];

	// for assetManager
	AssManProps assProps;
};

struct Texture
{
	Texture() = default;

	bool SSave(core::XFile* pFile) const;
	bool SLoad(core::XFile* pFile);

	core::string propName;
	core::string defaultName;

	render::TextureSlot::Enum texSlot;

	AssManProps assProps;
};


struct Sampler
{
	Sampler() :
		repeat(static_cast<render::TexRepeat::Enum>(0xff)),
		filter(static_cast<render::FilterType::Enum>(0xff))
	{}

	bool isRepeateDefined(void) const {
		return repeat != static_cast<render::TexRepeat::Enum>(0xff);
	}
	bool isFilterDefined(void) const {
		return filter != static_cast<render::FilterType::Enum>(0xff);
	}

	bool SSave(core::XFile* pFile) const;
	bool SLoad(core::XFile* pFile);


	core::string repeatStr;
	core::string filterStr;

	render::TexRepeat::Enum repeat;
	render::FilterType::Enum filter;

	AssManProps assProps;
};

class BaseTechSetDef : core::ISerialize
{
	template<typename T>
	using NameArr = core::Array<std::pair<core::string, T>>;

	typedef NameArr<Technique> TechniqueArr;
	typedef NameArr<Param> ParamArr;
	typedef NameArr<Texture> TextureArr;
	typedef NameArr<Sampler> SamplerArr;

public:
	BaseTechSetDef(core::string fileName, core::MemoryArenaBase* arena);
	virtual ~BaseTechSetDef();

	MATLIB_EXPORT bool SSave(core::XFile* pFile) const X_FINAL;
	MATLIB_EXPORT bool SLoad(core::XFile* pFile) X_FINAL;

	// we need a api for getting the techs.
	X_INLINE TechniqueArr::size_type numTechs(void) const;
	X_INLINE TechniqueArr::const_iterator techBegin(void) const;
	X_INLINE TechniqueArr::const_iterator techEnd(void) const;

	X_INLINE ParamArr::size_type numParams(void) const;
	X_INLINE ParamArr::const_iterator paramBegin(void) const;
	X_INLINE ParamArr::const_iterator paramEnd(void) const;

	X_INLINE TextureArr::size_type numTexture(void) const;
	X_INLINE TextureArr::const_iterator textureBegin(void) const;
	X_INLINE TextureArr::const_iterator textureEnd(void) const;

	X_INLINE SamplerArr::size_type numSampler(void) const;
	X_INLINE SamplerArr::const_iterator samplerBegin(void) const;
	X_INLINE SamplerArr::const_iterator samplerEnd(void) const;


protected:
	core::MemoryArenaBase* arena_;
	core::string fileName_;

	TechniqueArr techs_; 
	ParamArr params_;
	TextureArr textures_;
	SamplerArr samplers_;
};

class TechSetDef : public BaseTechSetDef
{
	template<typename T>
	using NameArr = BaseTechSetDef::NameArr<T>;

	typedef NameArr<render::BlendState> BlendStatesArr;
	typedef NameArr<StencilState> StencilStatesArr;
	typedef NameArr<render::StateDesc> StatesArr;
	typedef NameArr<Shader> ShaderArr;
	typedef NameArr<render::TopoType::Enum> PrimArr;

	typedef core::Array<char> FileBuf;

public:
	typedef core::traits::Function<bool(core::XParser& lex, Param& param, 
		const core::XLexToken& token, core::Hash::Fnv1aVal hash)>::Pointer ParamParseFunction;

	typedef core::Delegate<bool(core::XLexer& lex, core::string&, bool)> OpenIncludeDel;

public:
	TechSetDef(core::string fileName, core::MemoryArenaBase* arena);
	~TechSetDef();


	bool parseFile(FileBuf& buf);
	bool parseFile(FileBuf& buf, OpenIncludeDel incDel);

private:
	bool parseFile(core::XParser& lex);

	// blend
	bool parseBlendState(core::XParser& lex);
	bool parseBlendStateData(core::XParser& lex, render::BlendState& blend);
	bool parseBlendType(core::XParser& lex, render::BlendType::Enum& blendTypeOut);
	bool parseBlendOp(core::XParser& lex, render::BlendOp::Enum& blendOpOut);
	bool parseWriteChannels(core::XParser& lex, render::WriteMaskFlags& channels);

	// stencil
	bool parseStencilState(core::XParser& lex);
	bool parseStencilStateData(core::XParser& lex, StencilState& stencil);
	bool parseStencilFunc(core::XParser& lex, render::StencilFunc::Enum& funcOut);
	bool parseStencilOp(core::XParser& lex, render::StencilOperation::Enum& opOut);

	// State
	bool parseState(core::XParser& lex);
	bool parseStateData(core::XParser& lex, render::StateDesc& state);
	bool parseBlendState(core::XParser& lex, render::BlendState& blendState);
	bool parseStencilState(core::XParser& lex, StencilState& stencilstate);
	bool parseStencilRef(core::XParser& lex, uint32_t& stencilRef);
	bool parseCullMode(core::XParser& lex, render::CullType::Enum& cullOut);
	bool parseDepthTest(core::XParser& lex, render::DepthFunc::Enum& depthFuncOut);
	bool parsePolyOffset(core::XParser& lex, MaterialPolygonOffset::Enum& polyOffset);
	bool parsePrimitiveType(core::XParser& lex, render::TopoType::Enum& topo);

	// RenderFlags
	bool parseRenderFlags(core::XParser& lex);

	// Primt
	bool parsePrimitiveType(core::XParser& lex);
	bool parsePrimitiveTypeData(core::XParser& lex, render::TopoType::Enum& topo);

	// VertexShader
	bool parseVertexShader(core::XParser& lex);
	bool parsePixelShader(core::XParser& lex);
	bool parseHullShader(core::XParser& lex);
	bool parseDomainShader(core::XParser& lex);
	bool parseGeoShader(core::XParser& lex);


	// Shaders
	bool parseShader(core::XParser& lex, render::shader::ShaderType::Enum stage);
	bool parseShaderData(core::XParser& lex, Shader& shader);


	// Technique
	bool parseTechnique(core::XParser& lex);
	bool parseState(core::XParser& lex, render::StateDesc& state);
	bool parseShaderStage(core::XParser& lex, Technique& tech, render::shader::ShaderType::Enum stage);
	bool parseShaderStageHelper(core::XParser& lex, Shader& shader, render::shader::ShaderType::Enum stage);

	// params
	bool parseParamFloat1(core::XParser& lex);
	bool parseParamFloat2(core::XParser& lex);
	bool parseParamFloat4(core::XParser& lex);
	bool parseParamColor(core::XParser& lex);
	bool parseParamInt(core::XParser& lex);
	bool parseParamBool(core::XParser& lex);
	bool parseParamTexture(core::XParser& lex);
	bool parseParamSampler(core::XParser& lex);

	bool parseAssPropsData(core::XParser& lex, AssManProps& props);
	static bool parseParamFloat(core::XParser& lex, core::string& propsName, float& val);
	static bool parseParamTextureSlot(core::XParser& lex, render::TextureSlot::Enum& texSlot);
	static bool parseParamTextureData(core::XParser& lex, Texture& texture);
	static bool parsePropName(core::XParser& lex, core::string& str, bool& isExplicit);

	bool parseParamHelper(core::XParser& lex, ParamType::Enum type, ParamParseFunction parseFieldsFunc);

	// Helpers.
	bool parseBool(core::XParser& lex, bool& out);
	bool parseString(core::XParser& lex, core::string& out);
	bool parseDefines(core::XParser& lex, core::string& out);
	bool parseName(core::XParser& lex, core::string& name, core::string& parentName);

	template <typename T>
	bool parseHelper(core::XParser& lex, T& state,
		typename core::traits::MemberFunction<TechSetDef, bool(core::XParser& lex, T& state)>::Pointer parseStateFunc,
		typename core::traits::MemberFunction<TechSetDef, bool(const core::string& name, T* pState)>::Pointer stateExsistsFunc,
		const char* pObjName, const char* pStateName);
	bool parseInlineDefine(core::XParser& lex, core::string& name, core::string& parentName, const char* pStateName);
	bool parseNameInline(core::XParser& lex, core::string& parentName);

	bool blendStateExsists(const core::string& name, render::BlendState* pBlendOut = nullptr);
	bool stencilStateExsists(const core::string& name, StencilState* pStencilOut = nullptr);
	bool stateExsists(const core::string& name, render::StateDesc* pStateOut = nullptr);
	bool shaderExsists(const core::string& name, render::shader::ShaderType::Enum type, Shader* pShaderOut = nullptr);
	bool techniqueExsists(const core::string& name);
	bool primTypeExsists(const core::string& name, render::TopoType::Enum* pTopo = nullptr);
	bool paramExsists(const core::string& name, Param* pParam = nullptr);
	bool textureExsists(const core::string& name, Texture* pTexture = nullptr);
	bool samplerExsists(const core::string& name, Sampler* pSampler= nullptr);

	render::BlendState& addBlendState(const core::string& name, const core::string& parentName);
	StencilState& addStencilState(const core::string& name, const core::string& parentName);
	render::StateDesc& addState(const core::string& name, const core::string& parentName);
	Shader& addShader(const core::string& name, const core::string& parentName, render::shader::ShaderType::Enum type);
	Technique& addTechnique(const core::string& name, const core::string& parentName);
	render::TopoType::Enum& addPrimType(const core::string& name, const core::string& parentName);
	Param& addParam(const core::string& name, const core::string& parentName, ParamType::Enum type);
	Texture& addTexture(const core::string& name, const core::string& parentName);
	Sampler& addSampler(const core::string& name, const core::string& parentName);

	template<typename T>
	static bool findHelper(NameArr<T>& map, const core::string& name, T* pOut);

	template<typename T>
	static typename T::const_iterator findHelper(T& map, const core::string& name);

	template<typename T>
	static T& addHelper(NameArr<T>& map, const core::string& name,
		const core::string& parentName, const char* pNick);

	template<typename T, class... Args>
	static T& addHelper(NameArr<T>& map, const core::string& name, 
		const core::string& parentName, const char* pNick, Args&&... args);

private:
	core::MemoryArenaBase* arena_;
	core::string fileName_;

	BlendStatesArr blendStates_;
	StencilStatesArr stencilStates_;
	StatesArr states_;
	ShaderArr shaders_;
	PrimArr prims_;
};

} // namespace techset

X_NAMESPACE_END

#include "TechSetDef.inl"