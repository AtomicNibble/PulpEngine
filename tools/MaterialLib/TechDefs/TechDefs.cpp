#include "stdafx.h"
#include "TechDefs.h"

#include "Util\MatUtil.h"

#include <String\Lexer.h>
#include <String\XParser.h>
#include <Hashing\Fnva1Hash.h>

#include <IFileSys.h>

X_NAMESPACE_BEGIN(engine)


	TechSetDefs::TechSetDefs(core::MemoryArenaBase* arena) :
		arena_(arena),
		blendStates_(arena, 32),
		stencilStates_(arena, 16),
		states_(arena, 128),
		techs_(arena, 128)
	{

	}



	bool TechSetDefs::parseFile(core::Path<char>& path)
	{
		core::XFileScoped file;
		core::fileModeFlags mode = core::fileMode::READ | core::fileMode::SHARE;

		if (!file.openFile(path.c_str(), mode)) {
			X_ERROR("TestDefs", "Failed to open file");
			return false;
		}

		const size_t fileSize = safe_static_cast<size_t>(file.remainingBytes());

		FileBuf fileData(arena_, fileSize);

		if (file.read(fileData.data(), fileSize) != fileSize) {
			X_ERROR("TestDefs", "Failed to read file data");
			return false;
		}

		core::XParser lex(fileData.begin(), fileData.end(), path.fileName(), core::LexFlag::ALLOWPATHNAMES, arena_);

		return parseFile(lex);
	}

	bool TechSetDefs::parseFile(core::XParser& lex)
	{
		core::XLexToken token;

		// we want to parse the format :|
		// how best to support includes without doing include pass.
		while (lex.ReadToken(token))
		{
			// each line can be either:
			// #include (or other prepro)
			// BlendState, State, StencilState

			// how best to find each type since the token can contain more than just the name i think.

			if (token.isEqual("BlendState"))
			{
				if (!parseBlendState(lex)) {
					X_ERROR("TestDefs", "Failed to parse BlendState");
					return false;
				}
			}
			else if (token.isEqual("StencilState"))
			{
				if (!parseStencilState(lex)) {
					X_ERROR("TestDefs", "Failed to parse StencilState");
					return false;
				}
			}
			else if (token.isEqual("State"))
			{
				if (!parseState(lex)) {
					X_ERROR("TestDefs", "Failed to parse State");
					return false;
				}
			}
			else if (token.isEqual("RenderFlags"))
			{
				if (!parseRenderFlags(lex)) {
					X_ERROR("TestDefs", "Failed to parse RenderFlags");
					return false;
				}
			}
			else if (token.isEqual("Technique"))
			{
				if (!parseTechnique(lex)) {
					X_ERROR("TestDefs", "Failed to parse Technique");
					return false;
				}
			}
			else
			{
				X_ERROR("TestDefs", "Unexpected token \"%.*s\" Line: %" PRId32, token.length(), token.begin(), lex.GetLineNumber());
				return false;
			}
		}

		if (!lex.isEOF()) {
			X_WARNING("TechDefs", "Failed to fully parse file");
		}

		return true;
	}


	bool TechSetDefs::parseBlendState(core::XParser& lex)
	{
		core::string name, parentName;

		// we have the name.
		if (!parseName(lex, name, parentName)) {
			return false;
		}

		if (blendStateExsists(name)) {
			return false;
		}

		render::BlendState& blend = addBlendState(name, parentName);

		return parseBlendStateData(lex, blend);
	}

	bool TechSetDefs::parseBlendStateData(core::XParser& lex, render::BlendState& blend)
	{
		if (!lex.ExpectTokenString("{")) {
			return false;
		}

		// parse all the values.
		core::XLexToken token;
		while (lex.ReadToken(token))
		{
			if (token.isEqual("}")) {
				return true;
			}

			using namespace core::Hash::Fnva1Literals;

			switch (core::Hash::Fnv1aHash(token.begin(), token.length()))
			{
				case "colorBlendFunc"_fnv1a:
					if (!parseBlendOp(lex, blend.colorOp)) {
						return false;
					}
					break;
				case "colorBlendSrc"_fnv1a:
					if (!parseBlendType(lex, blend.srcBlendColor)) {
						return false;
					}
					break;
				case "colorBlendDst"_fnv1a:
					if (!parseBlendType(lex, blend.dstBlendColor)) {
						return false;
					}
					break;
				case "alphaBlendFunc"_fnv1a:
					if (!parseBlendOp(lex, blend.alphaOp)) {
						return false;
					}
					break;
				case "alphaBlendSrc"_fnv1a:
					if (!parseBlendType(lex, blend.srcBlendAlpha)) {
						return false;
					}
					break;
				case "alphaBlendDst"_fnv1a:
					if (!parseBlendType(lex, blend.dstBlendAlpha)) {
						return false;
					}
					break;
				case "writeChannels"_fnv1a:
					if (!parseWriteChannels(lex, blend.writeMask)) {
						return false;
					}
					break;

				default:
					X_ERROR("TechDef", "Unknown blend state prop: \"%.*s\"", token.length(), token.begin());
					return false;
			}
		}

		// failed to read token
		return false;
	}

	bool TechSetDefs::parseBlendType(core::XParser& lex, render::BlendType::Enum& blendTypeOut)
	{
		if (!lex.ExpectTokenString("=")) {
			return false;
		}

		core::XLexToken token;
		if (!lex.ReadToken(token)) {
			return false;
		}

		if (token.GetType() != core::TokenType::NAME) {
			X_ERROR("TestDefs", "Expected name token for BlendType. Line: %" PRIi32, lex.GetLineNumber());
			return false;
		}

		blendTypeOut = engine::Util::BlendTypeFromStr(token.begin(), token.end());
		if (blendTypeOut  == render::BlendType::INVALID) {
			return false;
		}

		return true;
	}

	bool TechSetDefs::parseBlendOp(core::XParser& lex, render::BlendOp::Enum& blendOpOut)
	{
		if (!lex.ExpectTokenString("=")) {
			return false;
		}

		core::XLexToken token;
		if (!lex.ReadToken(token)) {
			return false;
		}

		if (token.GetType() != core::TokenType::NAME) {
			X_ERROR("TestDefs", "Expected name token for blendOp. Line: %" PRIi32, lex.GetLineNumber());
			return false;
		}

		blendOpOut = engine::Util::BlendOpFromStr(token.begin(), token.end());
		return true;
	}

	bool TechSetDefs::parseWriteChannels(core::XParser& lex, render::WriteMaskFlags& channels)
	{
		if (!lex.ExpectTokenString("=")) {
			return false;
		}

		core::XLexToken token;
		if (!lex.ReadToken(token)) {
			return false;
		}

		if (token.GetType() != core::TokenType::STRING) {
			X_ERROR("TestDefs", "Expected string token for writeChannel mask. Line: %" PRIi32, lex.GetLineNumber());
			return false;
		}

		channels = engine::Util::WriteMaskFromStr(token.begin(), token.end());
		return true;
	}

	// ----------------------------------------------------

	bool TechSetDefs::parseStencilState(core::XParser& lex)
	{
		core::string name, parentName;

		// we have the name.
		if (!parseName(lex, name, parentName)) {
			return false;
		}

		if (stencilStateExsists(name)) {
			return false;
		}

		StencilState& stencil = addStencilState(name, parentName);

		return parseStencilStateData(lex, stencil);
	}

	bool TechSetDefs::parseStencilStateData(core::XParser& lex, StencilState& stencil)
	{
		auto& state = stencil.state;

		if (!lex.ExpectTokenString("{")) {
			return false;
		}

		core::XLexToken token;
		while (lex.ReadToken(token))
		{
			if (token.isEqual("}")) {
				return true;
			}

			using namespace core::Hash::Fnva1Literals;

			switch (core::Hash::Fnv1aHash(token.begin(), token.length()))
			{
				case "enable"_fnv1a:
					if (!parseBool(lex, stencil.enabled)) {
						return false;
					}
					break;
				case "func"_fnv1a:
					if (!parseStencilFunc(lex, state.stencilFunc)) {
						return false;
					}
					break;
				case "opFail"_fnv1a:
					if (!parseStencilOp(lex, state.failOp)) {
						return false;
					}
					break;
				case "opPass"_fnv1a:
					if (!parseStencilOp(lex, state.passOp)) {
						return false;
					}
					break;
				case "opZFail"_fnv1a:
					if (!parseStencilOp(lex, state.zFailOp)) {
						return false;
					}
					break;

				default:
					X_ERROR("TechDef", "Unknown stencil state prop: \"%.*s\"", token.length(), token.begin());
					return false;
			}
		}

		// failed to read token
		return false;
	}

	bool TechSetDefs::parseStencilFunc(core::XParser& lex, render::StencilFunc::Enum& funcOut)
	{
		if (!lex.ExpectTokenString("=")) {
			return false;
		}

		core::XLexToken token;
		if (!lex.ReadToken(token)) {
			return false;
		}

		if (token.GetType() != core::TokenType::NAME) {
			X_ERROR("TestDefs", "Expected name token for stencilFunc. Line: %" PRIi32, lex.GetLineNumber());
			return false;
		}

		funcOut = engine::Util::StencilFuncFromStr(token.begin(), token.end());
		return true;
	}

	bool TechSetDefs::parseStencilOp(core::XParser& lex, render::StencilOperation::Enum& opOut)
	{
		if (!lex.ExpectTokenString("=")) {
			return false;
		}

		core::XLexToken token;
		if (!lex.ReadToken(token)) {
			return false;
		}

		if (token.GetType() != core::TokenType::NAME) {
			X_ERROR("TestDefs", "Expected name token for stencilOp. Line: %" PRIi32, lex.GetLineNumber());
			return false;
		}

		opOut = engine::Util::StencilOpFromStr(token.begin(), token.end());
		return true;
	}

	// ----------------------------------------------------

	bool TechSetDefs::parseState(core::XParser& lex)
	{
		core::string name, parentName;

		// we have the name.
		if (!parseName(lex, name, parentName)) {
			return false;
		}

		if (!lex.ExpectTokenString("{")) {
			return false;
		}

		if (stateExsists(name)) {
			return false;
		}

		auto& state = addState(name, parentName);

		return parseStateData(lex, state);
	}

	bool TechSetDefs::parseStateData(core::XParser& lex, render::StateDesc& state)
	{
		MaterialPolygonOffset::Enum polyOffset;

		core::XLexToken token;
		while (lex.ReadToken(token))
		{
			if (token.isEqual("}")) {
				return true;
			}

			using namespace core::Hash::Fnva1Literals;

			switch (core::Hash::Fnv1aHash(token.begin(), token.length()))
			{
				// anything that is not Stencil or blend.
				case "stencilFront"_fnv1a:
				{
					StencilState stencil;
					if (!parseStencilState(lex, stencil)) {
						return false;
					}

					state.stencil.front = stencil.state;
					if (stencil.enabled) {
						state.stateFlags.Set(render::StateFlag::STENCIL);
					}
					break;
				}
				case "stencilBack"_fnv1a:
				{
					StencilState stencil;
					if (!parseStencilState(lex, stencil)) {
						return false;
					}

					state.stencil.back = stencil.state;
					if (stencil.enabled) {
						state.stateFlags.Set(render::StateFlag::STENCIL);
					}
					break;
				}
				case "stencilRef"_fnv1a:
				{
					uint32_t ref = 0;
					if (!parseStencilRef(lex, ref)) {
						return false;
					}
					break;
				}
				case "blendState"_fnv1a:
				case "blendState0"_fnv1a:
				// we should probs support blend states for the diffrent rt's
				case "blendState1"_fnv1a:
				case "blendState2"_fnv1a:
					if (!parseBlendState(lex, state.blend)) {
						return false;
					}
					break;
				case "cull"_fnv1a:
					if (!parseCullMode(lex, state.cullType)) {
						return false;
					}
					break;
				case "depthWrite"_fnv1a:
				{
					bool depthWrite = false;
					if (!parseBool(lex, depthWrite)) {
						return false;
					}

					if (depthWrite) {
						state.stateFlags.Set(render::StateFlag::DEPTHWRITE);
					}
					break;
				}
				case "wireframe"_fnv1a:
				{
					bool wireframe = false;
					if (!parseBool(lex, wireframe)) {
						return false;
					}

					if (wireframe) {
						state.stateFlags.Set(render::StateFlag::WIREFRAME);
					}
					break;
				}
				case "depthTest"_fnv1a:
					if (!parseDepthTest(lex, state.depthFunc)) {
						return false;
					}
					break;
				case "polygonOffset"_fnv1a:
					if (!parsePolyOffset(lex, polyOffset)) {
						return false;
					}
					break;

				default:
					X_ERROR("TechDef", "Unknown state prop: \"%.*s\"", token.length(), token.begin());
					return false;
			}
		}

		// failed to read token
		return false;
	}

	bool TechSetDefs::parseBlendState(core::XParser& lex, render::BlendState& blendState)
	{
		// this can eitehr be a string or inline define of new state.
		if (!lex.ExpectTokenString("=")) {
			return false;
		}

		core::XLexToken token;
		if (!lex.ReadToken(token)) {
			return false;
		}

		if (token.GetType() == core::TokenType::NAME) 
		{
			// inline declare.
			if (!token.isEqual("BlendState")) {
				X_ERROR("TechDef", "Expected either blend state name or inline define. Line: %" PRIi32, lex.GetLineNumber());
				return false;
			}

			// got ()
			if (!lex.ExpectTokenString("(")) {
				return false;
			}
			if (!lex.ExpectTokenString(")")) {
				return false;
			}

			// parse the inline state.
			if (parseBlendStateData(lex, blendState)) {
				return true;
			}

			X_ERROR("TechDef", "Failed to parse inline BlendState. Line: %" PRIi32, lex.GetLineNumber());
		}
		else if (token.GetType() == core::TokenType::STRING)
		{
			// a predefined one
			core::string name(token.begin(), token.end());

			if (blendStateExsists(name, &blendState)) {
				return true;
			}

			X_ERROR("TechDef", "State uses undefined blendstate: \"%s\" Line: %" PRIi32, name.c_str(), lex.GetLineNumber());
		}

		return false;
	}

	bool TechSetDefs::parseStencilState(core::XParser& lex, StencilState& stencilstate)
	{
		if (!lex.ExpectTokenString("=")) {
			return false;
		}

		core::XLexToken token;
		if (!lex.ReadToken(token)) {
			return false;
		}

		if (token.GetType() == core::TokenType::NAME)
		{
			// inline declare.
			if (!token.isEqual("StencilState")) {
				X_ERROR("TechDef", "Expected either stencil state name or inline define. Line: %" PRIi32, lex.GetLineNumber());
				return false;
			}

			// got ()
			if (!lex.ExpectTokenString("(")) {
				return false;
			}
			if (!lex.ExpectTokenString(")")) {
				return false;
			}

			// parse the inline state.
			if (parseStencilStateData(lex, stencilstate)) {
				return true;
			}

			X_ERROR("TechDef", "Failed to parse inline StencilState. Line: %" PRIi32, lex.GetLineNumber());
		}
		else if (token.GetType() == core::TokenType::STRING)
		{
			// a predefined one
			core::string name(token.begin(), token.end());

			if (stencilStateExsists(name, &stencilstate)) {
				return true;
			}

			X_ERROR("TechDef", "State uses undefined StencilState: \"%s\" Line: %" PRIi32, name.c_str(), lex.GetLineNumber());
		}

		return false;
	}

	bool TechSetDefs::parseStencilRef(core::XParser& lex, uint32_t& stencilRef)
	{
		if (!lex.ExpectTokenString("=")) {
			return false;
		}

		core::XLexToken token;
		if (!lex.ReadToken(token)) {
			return false;
		}


		if (token.GetType() != core::TokenType::NUMBER) {
			X_ERROR("TestDefs", "Expected numeric token for stencilRef. Line: %" PRIi32, lex.GetLineNumber());
			return false;
		}

		stencilRef = static_cast<uint32_t>(token.GetIntValue());
		return true;
	}

	bool TechSetDefs::parseCullMode(core::XParser& lex, render::CullType::Enum& cullOut)
	{
		if (!lex.ExpectTokenString("=")) {
			return false;
		}

		core::XLexToken token;
		if (!lex.ReadToken(token)) {
			return false;
		}

		if (token.GetType() != core::TokenType::NAME) {
			X_ERROR("TestDefs", "Expected name token for cullMode. Line: %" PRIi32, lex.GetLineNumber());
			return false;
		}

		cullOut = engine::Util::CullTypeFromStr(token.begin(), token.end());
		return true;
	}

	bool TechSetDefs::parseDepthTest(core::XParser& lex, render::DepthFunc::Enum& depthFuncOut)
	{
		if (!lex.ExpectTokenString("=")) {
			return false;
		}

		core::XLexToken token;
		if (!lex.ReadToken(token)) {
			return false;
		}

		if (token.GetType() != core::TokenType::NAME) {
			X_ERROR("TestDefs", "Expected name token for depthFunc. Line: %" PRIi32, lex.GetLineNumber());
			return false;
		}

		depthFuncOut = engine::Util::DepthFuncFromStr(token.begin(), token.end());
		return true;
	}

	bool TechSetDefs::parsePolyOffset(core::XParser& lex, MaterialPolygonOffset::Enum& polyOffset)
	{
		if (!lex.ExpectTokenString("=")) {
			return false;
		}

		core::XLexToken token;
		if (!lex.ReadToken(token)) {
			return false;
		}

		if (token.GetType() != core::TokenType::STRING) {
			X_ERROR("TestDefs", "Expected string token for polyOffset. Line: %" PRIi32, lex.GetLineNumber());
			return false;
		}

		polyOffset = Util::MatPolyOffsetFromStr(token.begin(), token.end());
		return true;
	}

	// ----------------------------------------------------


	bool TechSetDefs::parseRenderFlags(core::XParser& lex)
	{


		return false;
	}


	// ----------------------------------------------------


	bool TechSetDefs::parseTechnique(core::XParser& lex)
	{
		core::string name, parentName;

		// we have the name.
		if (!parseName(lex, name, parentName)) {
			return false;
		}

		if (!lex.ExpectTokenString("{")) {
			return false;
		}

		if (techniqueExsists(name)) {
			return false;
		}

		auto& tech = addTechnique(name, parentName);

		core::XLexToken token;
		while (lex.ReadToken(token))
		{
			if (token.isEqual("}")) {
				return true;
			}

			using namespace core::Hash::Fnva1Literals;

			switch (core::Hash::Fnv1aHash(token.begin(), token.length()))
			{
				case "state"_fnv1a:
					if (!parseState(lex, tech.state)) {
						return false;
					}
					break;
				case "source"_fnv1a:
					if (!parseString(lex, tech.source)) {
						return false;
					}
					break;
				case "vs"_fnv1a:
					if (!parseString(lex, tech.vs)) {
						return false;
					}
					break;
				case "ps"_fnv1a:
					if (!parseString(lex, tech.ps)) {
						return false;
					}
					break;
				case "defines"_fnv1a:
					if (!parseString(lex, tech.defines)) {
						return false;
					}
					break;
				default:
					X_ERROR("TechDef", "Unknown Technique prop: \"%.*s\"", token.length(), token.begin());
					return false;
			}
		}

		// failed to read token
		return false;
	}

	bool TechSetDefs::parseState(core::XParser& lex, render::StateDesc& state)
	{
		if (!lex.ExpectTokenString("=")) {
			return false;
		}

		core::XLexToken token;
		if (!lex.ReadToken(token)) {
			return false;
		}

		if (token.GetType() == core::TokenType::NAME)
		{
			// inline declare.
			if (!token.isEqual("State")) {
				X_ERROR("TechDef", "Expected either State name or inline define. Line: %" PRIi32, lex.GetLineNumber());
				return false;
			}

			// got ()
			if (!lex.ExpectTokenString("(")) {
				return false;
			}
			if (!lex.ExpectTokenString(")")) {
				return false;
			}

			// parse the inline state.
			if (parseStateData(lex, state)) {
				return true;
			}

			X_ERROR("TechDef", "Failed to parse inline State. Line: %" PRIi32, lex.GetLineNumber());
		}
		else if (token.GetType() == core::TokenType::STRING)
		{
			// a predefined one
			core::string name(token.begin(), token.end());

			if (stateExsists(name, &state)) {
				return true;
			}

			X_ERROR("TechDef", "Tech uses undefined State: \"%s\" Line: %" PRIi32, name.c_str(), lex.GetLineNumber());
		}

		return false;
	}

	// ----------------------------------------------------

	bool TechSetDefs::parseBool(core::XParser& lex, bool& out)
	{
		if (!lex.ExpectTokenString("=")) {
			return false;
		}

		out = lex.ParseBool();
		return true;
	}

	bool TechSetDefs::parseString(core::XParser& lex, core::string& out)
	{
		if (!lex.ExpectTokenString("=")) {
			return false;
		}

		core::XLexToken token;
		if (lex.ExpectTokenType(core::TokenType::STRING, 
			core::TokenSubType::UNUSET, core::PunctuationId::UNUSET, token)) {
			return false;
		}

		out = core::string(token.begin(), token.end());
		return true;
	}

	bool TechSetDefs::parseName(core::XParser& lex, core::string& name, core::string& parentName)
	{
		if (!lex.ExpectTokenString("(")) {
			return false;
		}

		core::XLexToken token;
		if (!lex.ReadToken(token)) {
			return false;
		}

		if (token.GetType() != core::TokenType::STRING) {
			X_ERROR("TestDefs", "Expected string token for name. Line: %" PRIi32, lex.GetLineNumber());
			return false;
		}

		name = core::string(token.begin(), token.end());

		if (!lex.ExpectTokenString(")")) {
			return false;
		}

		// look for parent name.
		parentName.clear();

		if (lex.ReadToken(token))
		{
			if (!token.isEqual(":"))
			{
				lex.UnreadToken(token);
			}
			else
			{
				// got a parent name.
				if (!lex.ReadToken(token)) {
					X_ERROR("TestDefs", "Failed to read parent name. Line: %" PRIi32, lex.GetLineNumber());
					return false;
				}

				if (token.GetType() != core::TokenType::STRING && token.GetType() != core::TokenType::NAME) {
					X_ERROR("TestDefs", "Expected string/name token for parent name. Line: %" PRIi32, lex.GetLineNumber());
					return false;
				}

				parentName = core::string(token.begin(), token.end());
			}
		}

		return true;
	}

	bool TechSetDefs::blendStateExsists(const core::string& name, render::BlendState* pBlendOut)
	{
		auto it = blendStates_.find(name);
		if (it != blendStates_.end()) {
			if (pBlendOut) {
				*pBlendOut = it->second;
			}
			return true;
		}

		return false;
	}

	bool TechSetDefs::stencilStateExsists(const core::string& name, StencilState* pStencilOut)
	{
		auto it = stencilStates_.find(name);
		if (it != stencilStates_.end()) {
			if (pStencilOut) {
				*pStencilOut = it->second;
			}
			return true;
		}

		return false;
	}

	bool TechSetDefs::stateExsists(const core::string& name, render::StateDesc* pStateOut)
	{
		auto it = states_.find(name);
		if (it != states_.end()) {
			if (pStateOut) {
				*pStateOut = it->second;
			}
			return true;
		}

		return false;
	}

	bool TechSetDefs::techniqueExsists(const core::string& name)
	{
		return techs_.find(name) != techs_.end();
	}

	render::BlendState& TechSetDefs::addBlendState(const core::string& name, const core::string& parentName)
	{
		return addHelper(blendStates_, name, parentName, "BlendState");
	}

	StencilState& TechSetDefs::addStencilState(const core::string& name, const core::string& parentName)
	{
		return addHelper(stencilStates_, name, parentName, "StencilState");
	}

	render::StateDesc& TechSetDefs::addState(const core::string& name, const core::string& parentName)
	{
		return addHelper(states_, name, parentName, "state");
	}

	Technique& TechSetDefs::addTechnique(const core::string& name, const core::string& parentName)
	{
		return addHelper(techs_, name, parentName, "Tech");
	}

	template<typename T>
	T& TechSetDefs::addHelper(core::HashMap<core::string, T>& map, 
		const core::string& name, const core::string& parentName, const char* pNick)
	{
		if (!parentName.isEmpty())
		{
			auto it = map.find(parentName);
			if (it != map.end())
			{
				map.insert(std::make_pair(name, it->second));
			}
			else
			{
				X_ERROR("TestDef", "Unknown parent %s: \"%s\"", pNick, parentName.c_str());
				goto insert_default;
			}
		}
		else
		{
		insert_default:
			map.insert(std::make_pair(name, T()));
		}

		return map[name];
	}

X_NAMESPACE_END
