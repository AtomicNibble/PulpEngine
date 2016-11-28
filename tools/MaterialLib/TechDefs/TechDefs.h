#pragma once

#include <IRender.h>
#include <IMaterial.h>

#include <Containers\Array.h>
#include <Containers\HashMap.h>

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

	struct Technique
	{
		render::StateDesc state;

		core::string source;
		core::string vs;
		core::string ps;
		core::string defines;
	};

	class TechSetDefs
	{
		typedef core::HashMap<core::string, render::BlendState> BlendStatesMap;
		typedef core::HashMap<core::string, StencilState> StencilStatesMap;
		typedef core::HashMap<core::string, render::StateDesc> StatesMap;
		typedef core::HashMap<core::string, Technique> TechniqueMap;

		typedef core::Array<char> FileBuf;

	public:
		MATLIB_EXPORT TechSetDefs(core::MemoryArenaBase* arena);


		MATLIB_EXPORT bool parseFile(core::Path<char>& path);

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

		// Technique
		bool parseTechnique(core::XParser& lex);
		bool parseState(core::XParser& lex, render::StateDesc& state);


		 
		bool parseBool(core::XParser& lex, bool& out);
		bool parseString(core::XParser& lex, core::string& out);
		bool parseName(core::XParser& lex, core::string& name, core::string& parentName);

		bool blendStateExsists(const core::string& name, render::BlendState* pBlendOut = nullptr);
		bool stencilStateExsists(const core::string& name, StencilState* pStencilOut = nullptr);
		bool stateExsists(const core::string& name, render::StateDesc* pStateOut = nullptr);
		bool techniqueExsists(const core::string& name);

		render::BlendState& addBlendState(const core::string& name, const core::string& parentName);
		StencilState& addStencilState(const core::string& name, const core::string& parentName);
		render::StateDesc& addState(const core::string& name, const core::string& parentName);
		Technique& addTechnique(const core::string& name, const core::string& parentName);

		template<typename T>
		static T& addHelper(core::HashMap<core::string, T>& map, 
			const core::string& name, const core::string& parentName, const char* pNick);

	private:
		core::MemoryArenaBase* arena_;

		BlendStatesMap blendStates_;
		StencilStatesMap stencilStates_;
		StatesMap states_;
		TechniqueMap techs_;
	};


X_NAMESPACE_END
