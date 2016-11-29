#pragma once

#include <IRender.h>
#include <IMaterial.h>

#include <Containers\Array.h>
#include <Containers\HashMap.h>

#include <Traits\MemberFunctionTraits.h>
#include <Util\Delegate.h>

X_NAMESPACE_DECLARE(core,
	class XLexer;
	class XParser;
);

X_NAMESPACE_BEGIN(engine)


	// This is like a file format for defining states and flags that the materials can select
	// Just makes it easy to define what state a material will have.
	//
	//	this format supports includes
	//	defining states: blend, stencil etc.
	//
	//


	struct StencilState
	{
		render::StencilDesc state;
		bool enabled;
	};

	struct Shader
	{

		core::string source;
		core::string entry;
		core::string defines;

	};

	struct Technique
	{
		render::StateDesc state;

		core::string source;
		Shader vs;
		Shader ps;
		core::string defines;
	};

	class TechSetDef
	{
		typedef core::HashMap<core::string, render::BlendState> BlendStatesMap;
		typedef core::HashMap<core::string, StencilState> StencilStatesMap;
		typedef core::HashMap<core::string, render::StateDesc> StatesMap;
		typedef core::HashMap<core::string, Technique> TechniqueMap;
		typedef core::HashMap<core::string, Shader> ShaderMap;

		typedef core::Array<char> FileBuf;

	public:
		typedef core::Delegate<bool(core::XLexer& lex, core::string&)> OpenIncludeDel;

	public:
		TechSetDef(core::string fileName, core::MemoryArenaBase* arena);


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

		// RenderFlags
		bool parseRenderFlags(core::XParser& lex);


		// VertexShader
		bool parseVertexShader(core::XParser& lex);
		bool parseVertexShaderData(core::XParser& lex, Shader& shader);

		// PixelShader
		bool parsePixelShader(core::XParser& lex);
		bool parsePixelShaderData(core::XParser& lex, Shader& shader);


		// Technique
		bool parseTechnique(core::XParser& lex);
		bool parseState(core::XParser& lex, render::StateDesc& state);
		bool parseVertexShader(core::XParser& lex, Shader& shader);
		bool parsePixelShader(core::XParser& lex, Shader& shader);


		 
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

		render::BlendState& addBlendState(const core::string& name, const core::string& parentName);
		StencilState& addStencilState(const core::string& name, const core::string& parentName);
		render::StateDesc& addState(const core::string& name, const core::string& parentName);
		Shader& addShader(const core::string& name, const core::string& parentName, render::shader::ShaderType::Enum type);
		Technique& addTechnique(const core::string& name, const core::string& parentName);

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
		TechniqueMap techs_;
	};


	class TechSetDefs
	{
		typedef core::Array<char> FileBuf;
		typedef core::HashMap<core::string, FileBuf> SourceMap;
		typedef core::HashMap<core::string, TechSetDef> TechSetDefMap;

		static const char* INCLUDE_DIR;
		static const wchar_t* INCLUDE_DIR_W;

	public:
		MATLIB_EXPORT TechSetDefs(core::MemoryArenaBase* arena);

		MATLIB_EXPORT void setBaseDir(core::Path<char>& path);
		MATLIB_EXPORT bool parseTechDef(const core::string& name);

		MATLIB_EXPORT void clearIncSrcCache(void);

	private:
		bool loadFile(const core::string& name, FileBuf& bufOut);

		bool includeCallback(core::XLexer& lex, core::string& name);

	private:
		core::MemoryArenaBase* arena_;
		TechSetDef::OpenIncludeDel incDel_;
		core::Path<char> baseDir_;

		TechSetDefMap techDefs_;
		SourceMap incSourceMap_;
	};


X_NAMESPACE_END
