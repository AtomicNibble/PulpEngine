#pragma once


#include <IRender.h>
#include <IMaterial.h>


#include <Containers\Array.h>
#include <Containers\HashMap.h>

#include <Traits\MemberFunctionTraits.h>
#include <Util\Delegate.h>

X_NAMESPACE_DECLARE(core,
	class XLexer;
	class XLexToken;
	class XParser;
)

X_NAMESPACE_BEGIN(engine)


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

	render::shader::ShaderType::Enum type;
	core::string source;
	core::string entry;
	core::string defines;

	AliaseArr aliases;
};

struct Technique
{
	render::StateDesc state;

	core::string source;
	core::string defines;

	render::shader::ShaderStageFlags stages;
	Shader shaders[render::shader::ShaderType::ENUM_COUNT - 1];
};

struct AssManProps
{
	core::string cat; // the assetScript cat.
	core::string title;
	core::string default;
};

struct Image
{
	core::string propName;
	core::string default;
};

// the names of these are used directly as the define names (case sensitive)
X_DECLARE_ENUM(ParamType)(
	Float1,
	Float2,
	Float4,
	Int,
	Bool,
	Color,
	Texture
);

struct Param
{
	Param() = default;
	Param(const Param& oth);

	Param& operator=(const Param& oth);


	ParamType::Enum type;

	// we can store most params in this.
	// bool, int, float1, float2, float4, color
	Vec4f vec4;
	core::string vec4Props[4];

	// texture stuff
	render::TextureSlot::Enum texSlot;
	Image img;

	// for assetManager
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


	core::string repeatStr;
	core::string filterStr;

	render::TexRepeat::Enum repeat;
	render::FilterType::Enum filter;

	AssManProps assProps;
};

class TechSetDef
{
	typedef core::HashMap<core::string, render::BlendState> BlendStatesMap;
	typedef core::HashMap<core::string, StencilState> StencilStatesMap;
	typedef core::HashMap<core::string, render::StateDesc> StatesMap;
	typedef core::HashMap<core::string, Technique> TechniqueMap;
	typedef core::HashMap<core::string, Shader> ShaderMap;
	typedef core::HashMap<core::string, render::TopoType::Enum> PrimMap;
	typedef core::HashMap<core::string, Param> ParamMap;
	typedef core::HashMap<core::string, Sampler> SamplerMap;

	typedef core::Array<char> FileBuf;

public:
	typedef core::traits::Function<bool(core::XParser& lex, Param& param, 
		const core::XLexToken& token, core::Hash::Fnv1aVal hash)>::Pointer ParamParseFunction;

	typedef core::Delegate<bool(core::XLexer& lex, core::string&, bool)> OpenIncludeDel;

public:
	TechSetDef(core::string fileName, core::MemoryArenaBase* arena);
	~TechSetDef();

	// we need a api for getting the techs.
	X_INLINE TechniqueMap::size_type numTechs(void) const;
	X_INLINE TechniqueMap::const_iterator techBegin(void) const;
	X_INLINE TechniqueMap::const_iterator techEnd(void) const;

	X_INLINE ParamMap::size_type numParams(void) const;
	X_INLINE ParamMap::const_iterator paramBegin(void) const;
	X_INLINE ParamMap::const_iterator paramEnd(void) const;

	X_INLINE SamplerMap::size_type numSampler(void) const;
	X_INLINE SamplerMap::const_iterator samplerBegin(void) const;
	X_INLINE SamplerMap::const_iterator samplerEnd(void) const;

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
	static bool parseParamImageData(core::XParser& lex, Image& props);
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
	bool shaderExsists(const core::string& name, Shader* pShaderOut = nullptr);
	bool techniqueExsists(const core::string& name);
	bool primTypeExsists(const core::string& name, render::TopoType::Enum* pTopo = nullptr);
	bool paramExsists(const core::string& name, Param* pParam = nullptr);
	bool samplerExsists(const core::string& name, Sampler* pSampler= nullptr);

	render::BlendState& addBlendState(const core::string& name, const core::string& parentName);
	StencilState& addStencilState(const core::string& name, const core::string& parentName);
	render::StateDesc& addState(const core::string& name, const core::string& parentName);
	Shader& addShader(const core::string& name, const core::string& parentName, render::shader::ShaderType::Enum type);
	Technique& addTechnique(const core::string& name, const core::string& parentName);
	render::TopoType::Enum& addPrimType(const core::string& name, const core::string& parentName);
	Param& addParam(const core::string& name, const core::string& parentName, ParamType::Enum type);
	Sampler& addSampler(const core::string& name, const core::string& parentName);

	template<typename T>
	static bool findHelper(core::HashMap<core::string, T>& map,
		const core::string& name, T* pOut);

	template<typename T>
	static T& addHelper(core::HashMap<core::string, T>& map,
		const core::string& name, const core::string& parentName, const char* pNick);

private:
	core::MemoryArenaBase* arena_;
	core::string fileName_;

	BlendStatesMap blendStates_;
	StencilStatesMap stencilStates_;
	StatesMap states_;
	ShaderMap shaders_;
	TechniqueMap techs_; // leaving this as map, to make supporting parents simple. otherwise id probs make this a array.
	PrimMap prims_;
	ParamMap params_;
	SamplerMap samplers_;
};


X_NAMESPACE_END

#include "TechSetDef.inl"