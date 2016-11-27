#pragma once

#include <IRender.h>

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

	struct BlendState
	{
		render::BlendState state;

	};

	struct StencilState
	{
		render::StencilDesc state;
		bool enabled;
	};



	class TechSetDefs
	{
		typedef core::HashMap<core::string, BlendState> BlendStatesArr;
		typedef core::HashMap<core::string, StencilState> StencilStatesArr;

		typedef core::Array<char> FileBuf;

	public:
		MATLIB_EXPORT TechSetDefs(core::MemoryArenaBase* arena);


		MATLIB_EXPORT bool parseFile(core::Path<char>& path);

	private:
		bool parseFile(core::XParser& lex);
		
		// blend
		bool parseBlendState(core::XParser& lex);
		bool parseBlendType(core::XParser& lex, render::BlendType::Enum& blendTypeOut);
		bool parseBlendOp(core::XParser& lex, render::BlendOp::Enum& blendOpOut);
		bool parseWriteChannels(core::XParser& lex, render::WriteMaskFlags& channels);

		// stencil
		bool parseStencilState(core::XParser& lex);
		bool parseStencilFunc(core::XParser& lex, render::StencilFunc::Enum& funcOut);
		bool parseStencilOp(core::XParser& lex, render::StencilOperation::Enum& opOut);

		bool parseBool(core::XParser& lex, bool& out);
		bool parseName(core::XParser& lex, core::string& name);

		bool blendStateExsists(const core::string& name);
		bool stencilStateExsists(const core::string& name);

		BlendState& addBlendState(const core::string& name);
		StencilState& addStencilState(const core::string& name);

	private:
		core::MemoryArenaBase* arena_;

		BlendStatesArr blendStates_;
		StencilStatesArr stencilStates_;

	};


X_NAMESPACE_END
