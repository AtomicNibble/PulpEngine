#include "stdafx.h"
#include "TechSetDef.h"

#include "Util\MatUtil.h"

#include <String\Lexer.h>
#include <String\XParser.h>
#include <Hashing\Fnva1Hash.h>

#include <IFileSys.h>

X_NAMESPACE_BEGIN(engine)

Shader::Shader() :
	type(render::shader::ShaderType::UnKnown)
{

}


//-------------------------------------------------


TechSetDef::TechSetDef(core::string fileName, core::MemoryArenaBase* arena) :
	arena_(arena),
	fileName_(fileName),
	blendStates_(arena, 32),
	stencilStates_(arena, 16),
	states_(arena, 128),
	shaders_(arena, 16),
	techs_(arena, 8),
	prims_(arena, 6)
{

}

TechSetDef::~TechSetDef()
{
	blendStates_.free();
	stencilStates_.free();
	states_.free();
	shaders_.free();
	techs_.free();
	prims_.free();
}


bool TechSetDef::parseFile(FileBuf& buf)
{
	core::XParser lex(buf.begin(), buf.end(), fileName_, core::LexFlag::ALLOWPATHNAMES, arena_);

	return parseFile(lex);
}

bool TechSetDef::parseFile(FileBuf& buf, TechSetDef::OpenIncludeDel incDel)
{
	core::XParser lex(buf.begin(), buf.end(), fileName_, core::LexFlag::ALLOWPATHNAMES, arena_);
	lex.setIncludeCallback(incDel);

	return parseFile(lex);
}

bool TechSetDef::parseFile(core::XParser& lex)
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
		using namespace core::Hash::Fnva1Literals;

		switch (core::Hash::Fnv1aHash(token.begin(), token.length()))
		{
			case "BlendState"_fnv1a:
				if (!parseBlendState(lex)) {
					X_ERROR("TestDefs", "Failed to parse BlendState");
					return false;
				}
				break;

			case "StencilState"_fnv1a:
				if (!parseStencilState(lex)) {
					X_ERROR("TestDefs", "Failed to parse StencilState");
					return false;
				}
				break;
			case "State"_fnv1a:
				if (!parseState(lex)) {
					X_ERROR("TestDefs", "Failed to parse State");
					return false;
				}
				break;
			case "RenderFlags"_fnv1a:
				if (!parseRenderFlags(lex)) {
					X_ERROR("TestDefs", "Failed to parse RenderFlags");
					return false;
				}
				break;
			case "Technique"_fnv1a:
				if (!parseTechnique(lex)) {
					X_ERROR("TestDefs", "Failed to parse Technique");
					return false;
				}
				break;
			case "VertexShader"_fnv1a:
				if (!parseVertexShader(lex)) {
					X_ERROR("TestDefs", "Failed to parse VertexShader");
					return false;
				}
				break;
			case "PixelShader"_fnv1a:
				if (!parsePixelShader(lex)) {
					X_ERROR("TestDefs", "Failed to parse PixelShader");
					return false;
				}
				break;
			case "HullShader"_fnv1a:
				if (!parseHullShader(lex)) {
					X_ERROR("TestDefs", "Failed to parse HullShader");
					return false;
				}
				break;
			case "DomainShader"_fnv1a:
				if (!parseDomainShader(lex)) {
					X_ERROR("TestDefs", "Failed to parse DomainShader");
					return false;
				}
				break;
			case "GeometryShader"_fnv1a:
				if (!parseGeoShader(lex)) {
					X_ERROR("TestDefs", "Failed to parse GeometryShader");
					return false;
				}
				break;
			case "PrimitiveType"_fnv1a:
				if (!parsePrimitiveType(lex)) {
					X_ERROR("TestDefs", "Failed to parse PrimitiveType");
					return false;
				}
				break;

			default:
				X_ERROR("TestDefs", "Unexpected token \"%.*s\" File: %s:%" PRId32, token.length(), token.begin(),
					lex.GetFileName(), lex.GetLineNumber());
				return false;			
		}
	}

	if (!lex.isEOF()) {
		X_ERROR("TechDefs", "Failed to fully parse file");
		return false;
	}

	return true;
}

bool TechSetDef::parseBlendState(core::XParser& lex)
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

bool TechSetDef::parseBlendStateData(core::XParser& lex, render::BlendState& blend)
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

bool TechSetDef::parseBlendType(core::XParser& lex, render::BlendType::Enum& blendTypeOut)
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
	if (blendTypeOut == render::BlendType::INVALID) {
		return false;
	}

	return true;
}

bool TechSetDef::parseBlendOp(core::XParser& lex, render::BlendOp::Enum& blendOpOut)
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

bool TechSetDef::parseWriteChannels(core::XParser& lex, render::WriteMaskFlags& channels)
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

bool TechSetDef::parseStencilState(core::XParser& lex)
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

bool TechSetDef::parseStencilStateData(core::XParser& lex, StencilState& stencil)
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

bool TechSetDef::parseStencilFunc(core::XParser& lex, render::StencilFunc::Enum& funcOut)
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

bool TechSetDef::parseStencilOp(core::XParser& lex, render::StencilOperation::Enum& opOut)
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

bool TechSetDef::parseState(core::XParser& lex)
{
	core::string name, parentName;

	// we have the name.
	if (!parseName(lex, name, parentName)) {
		return false;
	}

	if (stateExsists(name)) {
		return false;
	}

	auto& state = addState(name, parentName);

	return parseStateData(lex, state);
}

bool TechSetDef::parseStateData(core::XParser& lex, render::StateDesc& state)
{
	MaterialPolygonOffset::Enum polyOffset;

	state.topo = render::TopoType::TRIANGLELIST;
	state.vertexFmt = render::shader::VertexFormat::P3F_T2S_C4B;

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

bool TechSetDef::parseBlendState(core::XParser& lex, render::BlendState& blendState)
{
	return parseHelper<render::BlendState>(lex, blendState, &TechSetDef::parseBlendStateData,
		&TechSetDef::blendStateExsists, "State", "BlendState");
}

bool TechSetDef::parseStencilState(core::XParser& lex, StencilState& stencilstate)
{
	return parseHelper<StencilState>(lex, stencilstate, &TechSetDef::parseStencilStateData,
		&TechSetDef::stencilStateExsists, "State", "StencilState");
}

bool TechSetDef::parseStencilRef(core::XParser& lex, uint32_t& stencilRef)
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

bool TechSetDef::parseCullMode(core::XParser& lex, render::CullType::Enum& cullOut)
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

bool TechSetDef::parseDepthTest(core::XParser& lex, render::DepthFunc::Enum& depthFuncOut)
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

bool TechSetDef::parsePolyOffset(core::XParser& lex, MaterialPolygonOffset::Enum& polyOffset)
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


bool TechSetDef::parseRenderFlags(core::XParser& lex)
{
	return false;
}

// ----------------------------------------------------


bool TechSetDef::parsePrimitiveType(core::XParser& lex)
{
	core::string name, parentName;

	// we have the name.
	if (!parseName(lex, name, parentName)) {
		return false;
	}

	if (!lex.ExpectTokenString("{")) {
		return false;
	}

	if (primTypeExsists(name)) {
		return false;
	}

	auto& prim = addPrimType(name, parentName);

	return parsePrimitiveTypeData(lex, prim);
}

bool TechSetDef::parsePrimitiveTypeData(core::XParser& lex, render::TopoType::Enum& topo)
{
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
			case "topo"_fnv1a:
			{
				if (!lex.ExpectTokenString("=")) {
					return false;
				}
				
				if (!lex.ReadToken(token)) {
					return false;
				}

				topo = Util::TopoFromStr(token.begin(), token.end());
				break;
			}

			default:
				X_ERROR("TechDef", "Unknown PrimitiveType prop: \"%.*s\"", token.length(), token.begin());
				return false;
		}
	}

	return false;
}


// ----------------------------------------------------

bool TechSetDef::parseVertexShader(core::XParser& lex)
{
	return parseShader(lex, render::shader::ShaderType::Vertex);
}

bool TechSetDef::parsePixelShader(core::XParser& lex)
{
	return parseShader(lex, render::shader::ShaderType::Pixel);
}

bool TechSetDef::parseHullShader(core::XParser& lex)
{
	return parseShader(lex, render::shader::ShaderType::Hull);
}

bool TechSetDef::parseDomainShader(core::XParser& lex)
{
	return parseShader(lex, render::shader::ShaderType::Domain);
}

bool TechSetDef::parseGeoShader(core::XParser& lex)
{
	return parseShader(lex, render::shader::ShaderType::Geometry);
}

// ----------------------------------------------------

bool TechSetDef::parseShader(core::XParser& lex, render::shader::ShaderType::Enum stage)
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

	auto& shader = addShader(name, parentName, stage);


	return parseShaderData(lex, shader);
}

bool TechSetDef::parseShaderData(core::XParser& lex, Shader& shader)
{
	// start of define.
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
			case "source"_fnv1a:
				if (!parseString(lex, shader.source)) {
					return false;
				}
				break;
			case "entry"_fnv1a:
				if (!parseString(lex, shader.entry)) {
					return false;
				}
				break;
			case "defines"_fnv1a:
				if (!parseDefines(lex, shader.defines)) {
					return false;
				}
				break;

			default:
				X_ERROR("TechDef", "Unknown Shader prop: \"%.*s\" File: %s:%" PRId32, token.length(), token.begin(),
					lex.GetFileName(), lex.GetLineNumber());
				return false;
		}
	}


	return false;
}

// ----------------------------------------------------



bool TechSetDef::parseTechnique(core::XParser& lex)
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

			// we have a global source than is passed down to shaders if they don't override.
			if (tech.source.isNotEmpty())
			{
				// only set source for defined stages.
				for(uint32_t i=0; i<render::shader::ShaderStage::FLAGS_COUNT; i++)
				{
					const auto type = static_cast<render::shader::ShaderType::Enum>(1);
					const auto stage = staderTypeToStageFlag(type);

					if (tech.stages.IsSet(stage))
					{
						if (tech.shaders[i].source.isEmpty()) {
							tech.shaders[i].source = tech.source;
						}
					}
				}

			}
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
				if (!parseShaderStage(lex, tech, render::shader::ShaderType::Vertex)) {
					return false;
				}
				break;
			case "ps"_fnv1a:
				if (!parseShaderStage(lex, tech, render::shader::ShaderType::Pixel)) {
					return false;
				}
				break;
			case "ds"_fnv1a:
				if (!parseShaderStage(lex, tech, render::shader::ShaderType::Domain)) {
					return false;
				}
				break;
			case "gs"_fnv1a:
				if (!parseShaderStage(lex, tech, render::shader::ShaderType::Geometry)) {
					return false;
				}
				break;
			case "hs"_fnv1a:
				if (!parseShaderStage(lex, tech, render::shader::ShaderType::Hull)) {
					return false;
				}
				break;
			case "defines"_fnv1a:
				if (!parseDefines(lex, tech.defines)) {
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

bool TechSetDef::parseState(core::XParser& lex, render::StateDesc& state)
{
	return parseHelper<render::StateDesc>(lex, state, &TechSetDef::parseStateData,
		&TechSetDef::stateExsists, "Tech", "State");
}



bool TechSetDef::parseShaderStage(core::XParser& lex, Technique& tech, render::shader::ShaderType::Enum type)
{
	Shader& shader = tech.shaders[type];
	shader.type = render::shader::ShaderType::UnKnown;

	if (!parseShaderStageHelper(lex, shader, type)) {
		return false;
	}

	// check it's from correct stage.
	if (shader.type != type)
	{
		// if the state is unknow it was inline and it must be correct otherwise parsing would fail.
		if (shader.type == render::shader::ShaderType::UnKnown)
		{
			shader.type = type;
		}
		else
		{

			return false;
		}
	}

	tech.stages.Set(staderTypeToStageFlag(type));
	return true;
}

bool TechSetDef::parseShaderStageHelper(core::XParser& lex, Shader& shader, render::shader::ShaderType::Enum type)
{
	core::StackString<128, char> defName;
	defName.appendFmt("%sShader", render::shader::ShaderType::ToString(type));


	core::string name, parentName;

	if (!parseInlineDefine(lex, name, parentName, defName.c_str())) {
		return false;
	}

	if (name.isNotEmpty())
	{
		name += render::shader::ShaderType::ToString(type);

		if (shaderExsists(name, &shader)) {
			return true;
		}

		X_ERROR("TechDef", "Tech uses undefined %s: \"%s\" File: %s:%" PRIi32, defName.c_str(), name.c_str(),
			lex.GetFileName(), lex.GetLineNumber());
	}
	else
	{
		X_ASSERT(name.isEmpty(), "Inline define can't have a name")(name.c_str());

		if (parentName.isNotEmpty())
		{
			// create temp so we don't log diffrent parent name, then in file.
			const core::string mergedParentName = parentName + render::shader::ShaderType::ToString(type);
			// inline define can have a parent.
			// but it must exist if defined.
			if (!shaderExsists(mergedParentName, &shader)) {
				X_ERROR("TechDef", "Tech has a inline %s define with a undefined parent of: %s \"%s\" File: %s:%" PRIi32,
					defName.c_str(), parentName.c_str(), lex.GetFileName(), lex.GetLineNumber());
				return false;
			}

			// if we selected a parent it should be impossible for it to have a diffrent stage.
			// even if the user wanted to. this is source code logic fail.
			X_ASSERT(shader.type == type, "Parent not from same stage")(shader.type, type);
		}

		// parse the inline state.
		if (parseShaderData(lex, shader)) {
			shader.type = type;
			return true;
		}

		X_ERROR("TechDef", "Failed to parse inline %s. File: %s:%" PRIi32, defName.c_str(), lex.GetFileName(), lex.GetLineNumber());
	}

	return false;
}

// ----------------------------------------------------

bool TechSetDef::parseBool(core::XParser& lex, bool& out)
{
	if (!lex.ExpectTokenString("=")) {
		return false;
	}

	out = lex.ParseBool();
	return true;
}

bool TechSetDef::parseString(core::XParser& lex, core::string& out)
{
	core::XLexToken token;
	if (!lex.ExpectTokenType(core::TokenType::PUNCTUATION,
		core::TokenSubType::UNUSET, core::PunctuationId::UNUSET, token)) {
		return false;
	}

	const auto punc = token.GetPuncId();
	if (punc != core::PunctuationId::ASSIGN && token.GetPuncId() != core::PunctuationId::ADD_ASSIGN) {
		X_ERROR("TechDef", "Expected assign or add assign. Line: %" PRIi32, lex.GetLineNumber());
		return false;
	}

	if (!lex.ExpectTokenType(core::TokenType::STRING,
		core::TokenSubType::UNUSET, core::PunctuationId::UNUSET, token)) {
		return false;
	}

	if (punc == core::PunctuationId::ASSIGN)
	{
		out = core::string(token.begin(), token.end());
	}
	else if (punc == core::PunctuationId::ADD_ASSIGN)
	{
		out += core::string(token.begin(), token.end());
	}
	else
	{
		X_ASSERT_UNREACHABLE();
	}

	return true;
}


bool TechSetDef::parseDefines(core::XParser& lex, core::string& out)
{
	core::XLexToken token;
	if (!lex.ExpectTokenType(core::TokenType::PUNCTUATION,
		core::TokenSubType::UNUSET, core::PunctuationId::UNUSET, token)) {
		return false;
	}

	const auto punc = token.GetPuncId();
	if (punc != core::PunctuationId::ASSIGN && token.GetPuncId() != core::PunctuationId::ADD_ASSIGN) {
		X_ERROR("TechDef", "Expected assign or add assign. Line: %" PRIi32, lex.GetLineNumber());
		return false;
	}

	if (!lex.ExpectTokenType(core::TokenType::STRING,
		core::TokenSubType::UNUSET, core::PunctuationId::UNUSET, token)) {
		return false;
	}

	// support multiple common delmie.
	core::string value(token.begin(), token.end());

	while (1)
	{
		if (!lex.ReadToken(token)) {
			break; // don't assume always more to read.
		}

		if (token.GetType() == core::TokenType::PUNCTUATION && token.GetPuncId() == core::PunctuationId::COMMA)
		{
			// another string.
			if (!lex.ReadToken(token)) {
				return false;
			}

			value += ",";
			value += core::string(token.begin(), token.end());
		}
		else
		{
			lex.UnreadToken(token);
			break;
		}
	}


	if (punc == core::PunctuationId::ASSIGN)
	{
		out = value;
	}
	else if (punc == core::PunctuationId::ADD_ASSIGN)
	{
		if (out.isNotEmpty()) {
			out += ",";
		}
		out += value;
	}
	else
	{
		X_ASSERT_UNREACHABLE();
	}

	return true;
}


bool TechSetDef::parseName(core::XParser& lex, core::string& name, core::string& parentName)
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


template <typename T>
bool TechSetDef::parseHelper(core::XParser& lex, T& state,
	typename core::traits::MemberFunction<TechSetDef, bool(core::XParser& lex, T& state)>::Pointer parseStateFunc,
	typename core::traits::MemberFunction<TechSetDef, bool(const core::string& name, T* pState)>::Pointer stateExsistsFunc,
	const char* pObjName, const char* pStateName)
{
	// this supports inline defines we parents or name of exsisting state.
	core::string name, parentName;

	if (!parseInlineDefine(lex, name, parentName, pStateName)) {
		return false;
	}

	if (name.isNotEmpty())
	{
		if ((*this.*stateExsistsFunc)(name, &state)) {
			return true;
		}

		X_ERROR("TechDef", "%s uses undefined %s: \"%s\" File: %s:%" PRIi32, pObjName, pStateName, name.c_str(),
			lex.GetFileName(), lex.GetLineNumber());
	}
	else
	{
		X_ASSERT(name.isEmpty(), "Inline define can't have a name")(name.c_str());

		if (parentName.isNotEmpty())
		{
			// inline define can have a parent.
			// but it must exist if defined.
			if (!(*this.*stateExsistsFunc)(parentName, &state)) {
				X_ERROR("TechDef", "%s has a inline %s define with a undefined parent of: %s \"%s\" File: %s:%" PRIi32,
					pObjName, pStateName, parentName.c_str(), lex.GetFileName(), lex.GetLineNumber());
				return false;
			}
		}

		// parse the inline state.
		if ((*this.*parseStateFunc)(lex, state)) {
			return true;
		}

		X_ERROR("TechDef", "Failed to parse inline %s. File: %s:%" PRIi32, pStateName, lex.GetFileName(), lex.GetLineNumber());
	}

	return false;
}

bool TechSetDef::parseInlineDefine(core::XParser& lex, core::string& name, core::string& parentName, const char* pStateName)
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
		name.clear();

		if (!token.isEqual(pStateName)) {
			X_ERROR("TechDef", "Expected '%s entry' or inline '%s' define. File: %s:%" PRIi32, pStateName, pStateName, 
				lex.GetFileName(), lex.GetLineNumber());
			return false;
		}

		if (parseNameInline(lex, parentName)) {
			return true;
		}
	}
	else if (token.GetType() == core::TokenType::STRING)
	{
		// a predefined one
		name = core::string(token.begin(), token.end());
		return true;
	}

	return false;
}


bool TechSetDef::parseNameInline(core::XParser& lex, core::string& parentName)
{
	if (!lex.ExpectTokenString("(")) {
		return false;
	}
	if (!lex.ExpectTokenString(")")) {
		return false;
	}

	// look for parent name.
	parentName.clear();

	core::XLexToken token;
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
				X_ERROR("TestDefs", "Failed to read parent name. File: %s:%" PRIi32, lex.GetFileName(), lex.GetLineNumber());
				return false;
			}

			if (token.GetType() != core::TokenType::STRING && token.GetType() != core::TokenType::NAME) {
				X_ERROR("TestDefs", "Expected string/name token for parent name. File: %s:%" PRIi32, lex.GetFileName(), lex.GetLineNumber());
				return false;
			}

			parentName = core::string(token.begin(), token.end());
		}
	}

	return true;
}


bool TechSetDef::blendStateExsists(const core::string& name, render::BlendState* pBlendOut)
{
	return findHelper(blendStates_, name, pBlendOut);
}

bool TechSetDef::stencilStateExsists(const core::string& name, StencilState* pStencilOut)
{
	return findHelper(stencilStates_, name, pStencilOut);
}

bool TechSetDef::stateExsists(const core::string& name, render::StateDesc* pStateOut)
{
	return findHelper(states_, name, pStateOut);
}

bool TechSetDef::shaderExsists(const core::string& name, Shader* pShaderOut)
{
	return findHelper(shaders_, name, pShaderOut);
}

bool TechSetDef::techniqueExsists(const core::string& name)
{
	return techs_.find(name) != techs_.end();
}

bool TechSetDef::primTypeExsists(const core::string& name)
{
	return prims_.find(name) != prims_.end();
}


render::BlendState& TechSetDef::addBlendState(const core::string& name, const core::string& parentName)
{
	return addHelper(blendStates_, name, parentName, "BlendState");
}

StencilState& TechSetDef::addStencilState(const core::string& name, const core::string& parentName)
{
	return addHelper(stencilStates_, name, parentName, "StencilState");
}

render::StateDesc& TechSetDef::addState(const core::string& name, const core::string& parentName)
{
	return addHelper(states_, name, parentName, "state");
}

Shader& TechSetDef::addShader(const core::string& name, const core::string& parentName, render::shader::ShaderType::Enum type)
{
	X_UNUSED(type);
	Shader& shader = addHelper(shaders_, name, parentName, "state");
	shader.type = type;
	return shader;
}

Technique& TechSetDef::addTechnique(const core::string& name, const core::string& parentName)
{
	return addHelper(techs_, name, parentName, "Tech");
}

render::TopoType::Enum& TechSetDef::addPrimType(const core::string& name, const core::string& parentName)
{
	return addHelper(prims_, name, parentName, "PrimitiveType");
}

template<typename T>
X_INLINE bool TechSetDef::findHelper(core::HashMap<core::string, T>& map,
	const core::string& name, T* pOut)
{
	auto it = map.find(name);
	if (it != map.end()) {
		if (pOut) {
			*pOut = it->second;
		}
		return true;
	}

	return false;
}

template<typename T>
X_INLINE  T& TechSetDef::addHelper(core::HashMap<core::string, T>& map,
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
