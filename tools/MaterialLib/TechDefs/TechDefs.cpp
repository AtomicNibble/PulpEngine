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
		stencilStates_(arena, 16)
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
				parseBlendState(lex);
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
		core::string name;

		// we have the name.
		if (!parseName(lex, name)) {
			return false;
		}

		if (!lex.ExpectTokenString("{")) {
			return false;
		}

		if (blendStateExsists(name)) {
			return false;
		}

		BlendState& blend = addBlendState(name);
		auto& state = blend.state;

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
					if (!parseBlendOp(lex, state.colorOp)) {
						return false;
					}
					break;
				case "colorBlendSrc"_fnv1a:
					if (!parseBlendType(lex, state.srcBlendColor)) {
						return false;
					}
					break;
				case "colorBlendDst"_fnv1a:
					if (!parseBlendType(lex, state.dstBlendColor)) {
						return false;
					}
					break;
				case "alphaBlendFunc"_fnv1a:
					if (!parseBlendOp(lex, state.alphaOp)) {
						return false;
					}
					break;
				case "alphaBlendSrc"_fnv1a:
					if (!parseBlendType(lex, state.srcBlendAlpha)) {
						return false;
					}
					break;
				case "alphaBlendDst"_fnv1a:
					if (!parseBlendType(lex, state.dstBlendAlpha)) {
						return false;
					}
					break;
				case "writeChannels"_fnv1a:
					if (!parseWriteChannels(lex, state.writeMask)) {
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
		core::string name;

		// we have the name.
		if (!parseName(lex, name)) {
			return false;
		}

		if (!lex.ExpectTokenString("{")) {
			return false;
		}

		if (stencilStateExsists(name)) {
			return false;
		}

		StencilState& stencil = addStencilState(name);
		auto& state = stencil.state;

		core::XLexToken token;
		while (lex.ReadToken(token))
		{
			if (token.isEqual("}")) {

				state.back = state.front;
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
					if (!parseStencilFunc(lex, state.front.stencilFunc)) {
						return false;
					}
					break;
				case "opFail"_fnv1a:
					if (!parseStencilOp(lex, state.front.failOp)) {
						return false;
					}
					break;
				case "opPass"_fnv1a:
					if (!parseStencilOp(lex, state.front.passOp)) {
						return false;
					}
					break;
				case "opZFail"_fnv1a:
					if (!parseStencilOp(lex, state.front.zFailOp)) {
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

	bool TechSetDefs::parseBool(core::XParser& lex, bool& out)
	{
		if (!lex.ExpectTokenString("=")) {
			return false;
		}

		out = lex.ParseBool();
		return true;
	}

	bool TechSetDefs::parseName(core::XParser& lex, core::string& name)
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

		return true;
	}

	bool TechSetDefs::blendStateExsists(const core::string& name)
	{
		return blendStates_.find(name) != blendStates_.end();
	}

	bool TechSetDefs::stencilStateExsists(const core::string& name)
	{
		return stencilStates_.find(name) != stencilStates_.end();
	}


	BlendState& TechSetDefs::addBlendState(const core::string& name)
	{
		blendStates_.insert(std::make_pair(name, BlendState()));

		return blendStates_[name];
	}

	StencilState& TechSetDefs::addStencilState(const core::string& name)
	{
		stencilStates_.insert(std::make_pair(name, StencilState()));

		return stencilStates_[name];
	}

X_NAMESPACE_END
