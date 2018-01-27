#include "stdafx.h"
#include "TechSetDef.h"

#include "Util\MatUtil.h"

#include <String\Lexer.h>
#include <String\XParser.h>
#include <Hashing\Fnva1Hash.h>

#include <IFileSys.h>

#include <Time\CompressedStamps.h>

X_NAMESPACE_BEGIN(engine)

using namespace core::Hash::Fnv1Literals;

namespace techset
{

	namespace
	{
		X_DECLARE_FLAGS8(BinFileFlag)(
			COMPRESSED
		);

		typedef Flags8<BinFileFlag> BinFileFlags;

		struct TechSetDefBinHeader
		{
			static const uint32_t FOURCC = X_TAG('X', 'T', 'S', 'D');
			static const uint32_t VERSION = 1; 

			X_INLINE const bool isValid(void) const {
				return forcc == FOURCC;
			}

			uint32_t forcc;
			uint8_t version;
			BinFileFlags flags;
			uint8_t _pad[2];

			uint32_t crc32;
			core::DateTimeStampSmall modifed;

			uint8_t numTechs;
			uint8_t numParams;
			uint8_t numTextures;
			uint8_t numSamplers;
			uint8_t numStaticSamplers;
			uint8_t __pad[3];
		};

		X_ENSURE_SIZE(TechSetDefBinHeader, 24);


	} // namespace 

//-------------------------------------------------

	
Shader::Shader() :
	Shader(ShaderType::UnKnown)
{

}

Shader::Shader(ShaderType::Enum type) :
	type(type),
	aliases(g_MatLibArena)
{

}


bool Shader::SSave(core::XFile* pFile) const
{
	pFile->writeObj(type);
	pFile->writeString(source);
	pFile->writeString(entry);
	pFile->writeString(defines);
	pFile->writeObj(safe_static_cast<uint8_t>(aliases.size()));
	for (const auto& a : aliases)
	{
		pFile->writeObj(a.isCode);
		pFile->writeString(a.resourceName);
		pFile->writeString(a.name);
		pFile->writeObj(a.nameHash);
	}
	return true;
}

bool Shader::SLoad(core::XFile* pFile)
{
	pFile->readObj(type);
	pFile->readString(source);
	pFile->readString(entry);
	pFile->readString(defines);
	
	uint8_t numAliases;
	pFile->readObj(numAliases);
	
	aliases.resize(numAliases);
	for (auto& a : aliases)
	{
		pFile->readObj(a.isCode);
		pFile->readString(a.resourceName);
		pFile->readString(a.name);
		pFile->readObj(a.nameHash);
	}
	return true;
}

//-------------------------------------------------

bool Technique::SSave(core::XFile* pFile) const
{
	pFile->writeObj(state);
	pFile->writeString(source);
	pFile->writeString(defines);
	pFile->writeObj(stages);

	for (const auto& s : shaders)
	{
		s.SSave(pFile);
	}

	return true;
}

bool Technique::SLoad(core::XFile* pFile)
{
	pFile->readObj(state);
	pFile->readString(source);
	pFile->readString(defines);
	pFile->readObj(stages);

	for (auto& s : shaders)
	{
		s.SLoad(pFile);
	}

	return true;
}


//-------------------------------------------------

bool AssManProps::SSave(core::XFile* pFile) const
{
	pFile->writeString(cat);
	pFile->writeString(title);
	pFile->writeString(defaultVal);

	return true;
}

bool AssManProps::SLoad(core::XFile* pFile)
{
	pFile->readString(cat);
	pFile->readString(title);
	pFile->readString(defaultVal);

	return true;
}

//-------------------------------------------------

Param::Param(ParamType::Enum type) :
	type(type)
{

}

Param::Param(const Param& oth)
{
	*this = oth;
}

Param& Param::operator=(const Param& oth)
{
	static_assert(ParamType::ENUM_COUNT == 6, "Added additional ParamType? this code needs updating.");

	if (this != &oth)
	{
		type = oth.type;
		assProps = oth.assProps;

		switch (type)
		{
			case ParamType::Float1:
			case ParamType::Float2:
			case ParamType::Float4:
			case ParamType::Int:
			case ParamType::Bool:
			case ParamType::Color:
				vec4 = oth.vec4;
				break;
		}
	}

	return *this;
}

bool Param::SSave(core::XFile* pFile) const
{
	pFile->writeObj(type);
	pFile->writeObj(vec4);

	return assProps.SSave(pFile);
}

bool Param::SLoad(core::XFile* pFile)
{
	pFile->readObj(type);
	pFile->readObj(vec4);

	return assProps.SLoad(pFile);
}

//-------------------------------------------------

bool Texture::SSave(core::XFile* pFile) const
{
	pFile->writeString(propName);
	pFile->writeString(defaultName);
	pFile->writeObj(texSlot);

	return assProps.SSave(pFile);
}

bool Texture::SLoad(core::XFile* pFile)
{
	pFile->readString(propName);
	pFile->readString(defaultName);
	pFile->readObj(texSlot);

	return assProps.SLoad(pFile);
}

//-------------------------------------------------

bool Sampler::SSave(core::XFile* pFile) const
{
	pFile->writeString(repeatStr);
	pFile->writeString(filterStr);
	pFile->writeObj(repeat);
	pFile->writeObj(filter);

	return true;
}

bool Sampler::SLoad(core::XFile* pFile)
{
	pFile->readString(repeatStr);
	pFile->readString(filterStr);
	pFile->readObj(repeat);
	pFile->readObj(filter);

	return true;
}

//-------------------------------------------------


BaseTechSetDef::BaseTechSetDef(core::string fileName, core::MemoryArenaBase* arena) :
	arena_(arena),
	fileName_(fileName),
	techs_(arena),
	params_(arena),
	textures_(arena),
	samplers_(arena),
	staticSamplers_(arena)
{

}

BaseTechSetDef::~BaseTechSetDef()
{
	techs_.free();
}


// ISerialize

bool BaseTechSetDef::SSave(core::XFile* pFile) const
{
	TechSetDefBinHeader hdr;
	core::zero_object(hdr);
	hdr.forcc = TechSetDefBinHeader::FOURCC;
	hdr.version = TechSetDefBinHeader::VERSION;

	hdr.crc32 = 0;
	hdr.modifed = core::DateTimeStampSmall::systemDateTime();

	hdr.numTechs = safe_static_cast<uint8_t>(techs_.size());
	hdr.numParams = safe_static_cast<uint8_t>(params_.size());
	hdr.numTextures = safe_static_cast<uint8_t>(textures_.size());
	hdr.numSamplers = safe_static_cast<uint8_t>(samplers_.size());
	hdr.numStaticSamplers = safe_static_cast<uint8_t>(staticSamplers_.size());

	pFile->writeObj(hdr);

	for (const auto& t : techs_) {
		pFile->writeString(t.first);
	}
	for (const auto& p : params_) {
		pFile->writeString(p.first);
	}
	for (const auto& t : textures_) {
		pFile->writeString(t.first);
	}
	for (const auto& s : samplers_) {
		pFile->writeString(s.first);
	}
	for (const auto& s : staticSamplers_) {
		pFile->writeString(s.first);
	}

	for (const auto& t : techs_) {
		t.second.SSave(pFile);
	}
	for (const auto& p : params_) {
		p.second.SSave(pFile);
	}
	for (const auto& t : textures_) {
		t.second.SSave(pFile);
	}
	for (const auto& s : samplers_) {
		s.second.SSave(pFile);
	}
	for (const auto& s : staticSamplers_) {
		s.second.SSave(pFile);
	}
	return true;
}

bool BaseTechSetDef::SLoad(core::XFile* pFile)
{
	TechSetDefBinHeader hdr;

	if (pFile->readObj(hdr) != sizeof(hdr)) {
		return false;
	}

	if (!hdr.isValid()) {
		return false;
	}

	if (hdr.version != TechSetDefBinHeader::VERSION) {
		return false;
	}

	techs_.resize(hdr.numTechs);
	params_.resize(hdr.numParams);
	textures_.resize(hdr.numTextures);
	samplers_.resize(hdr.numSamplers);
	staticSamplers_.resize(hdr.numStaticSamplers);

	for (auto& t : techs_) {
		pFile->readString(t.first);
	}
	for (auto& p : params_) {
		pFile->readString(p.first);
	}
	for (auto& t : textures_) {
		pFile->readString(t.first);
	}
	for (auto& s : samplers_) {
		pFile->readString(s.first);
	}
	for (auto& s : staticSamplers_) {
		pFile->readString(s.first);
	}

	for (auto& t : techs_) {
		t.second.SLoad(pFile);
	}
	for (auto& p : params_) {
		p.second.SLoad(pFile);
	}
	for (auto& t : textures_) {
		t.second.SLoad(pFile);
	}
	for (auto& s : samplers_) {
		s.second.SLoad(pFile);
	}
	for (auto& s : staticSamplers_) {
		s.second.SLoad(pFile);
	}

	return true;
}

// ~ISerialize

//-------------------------------------------------


TechSetDef::TechSetDef(core::string fileName, core::MemoryArenaBase* arena) :
	BaseTechSetDef(fileName, arena),
	arena_(arena),
	fileName_(fileName),
	blendStates_(arena),
	stencilStates_(arena),
	states_(arena),
	shaders_(arena),
	prims_(arena)
{

	states_.reserve(64);
}

TechSetDef::~TechSetDef()
{
	blendStates_.free();
	stencilStates_.free();
	states_.free();
	shaders_.free();
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
		
		switch (core::Hash::Fnv1aHash(token.begin(), token.length()))
		{
			case "BlendState"_fnv1a:
				if (!parseBlendState(lex)) {
					X_ERROR("TechDef", "Failed to parse BlendState");
					return false;
				}
				break;

			case "StencilState"_fnv1a:
				if (!parseStencilState(lex)) {
					X_ERROR("TechDef", "Failed to parse StencilState");
					return false;
				}
				break;
			case "State"_fnv1a:
				if (!parseState(lex)) {
					X_ERROR("TechDef", "Failed to parse State");
					return false;
				}
				break;
			case "RenderFlags"_fnv1a:
				if (!parseRenderFlags(lex)) {
					X_ERROR("TechDef", "Failed to parse RenderFlags");
					return false;
				}
				break;
			case "Technique"_fnv1a:
				if (!parseTechnique(lex)) {
					X_ERROR("TechDef", "Failed to parse Technique");
					return false;
				}
				break;
			case "VertexShader"_fnv1a:
				if (!parseVertexShader(lex)) {
					X_ERROR("TechDef", "Failed to parse VertexShader");
					return false;
				}
				break;
			case "PixelShader"_fnv1a:
				if (!parsePixelShader(lex)) {
					X_ERROR("TechDef", "Failed to parse PixelShader");
					return false;
				}
				break;
			case "HullShader"_fnv1a:
				if (!parseHullShader(lex)) {
					X_ERROR("TechDef", "Failed to parse HullShader");
					return false;
				}
				break;
			case "DomainShader"_fnv1a:
				if (!parseDomainShader(lex)) {
					X_ERROR("TechDef", "Failed to parse DomainShader");
					return false;
				}
				break;
			case "GeometryShader"_fnv1a:
				if (!parseGeoShader(lex)) {
					X_ERROR("TechDef", "Failed to parse GeometryShader");
					return false;
				}
				break;
			case "PrimitiveType"_fnv1a:
				if (!parsePrimitiveType(lex)) {
					X_ERROR("TechDef", "Failed to parse PrimitiveType");
					return false;
				}
				break;

			// params.
			case "float1"_fnv1a:
				if (!parseParamFloat1(lex)) {
					X_ERROR("TechDef", "Failed to parse float1");
					return false;
				}
				break;
			case "float2"_fnv1a:
				if (!parseParamFloat2(lex)) {
					X_ERROR("TechDef", "Failed to parse float2");
					return false;
				}
				break;
			case "float4"_fnv1a:
				if (!parseParamFloat4(lex)) {
					X_ERROR("TechDef", "Failed to parse float4");
					return false;
				}
				break;
			case "bool"_fnv1a:
				if (!parseParamBool(lex)) {
					X_ERROR("TechDef", "Failed to parse bool");
					return false;
				}
				break;
			case "int"_fnv1a:
				if (!parseParamInt(lex)) {
					X_ERROR("TechDef", "Failed to parse int");
					return false;
				}
				break;
			case "Texture"_fnv1a:
				if (!parseParamTexture(lex)) {
					X_ERROR("TechDef", "Failed to parse Texture");
					return false;
				}
				break;
			case "Sampler"_fnv1a:
				if (!parseParamSampler(lex)) {
					X_ERROR("TechDef", "Failed to parse Sampler");
					return false;
				}
				break;
			case "Color"_fnv1a:
				if (!parseParamColor(lex)) {
					X_ERROR("TechDef", "Failed to parse Color");
					return false;
				}
				break;

			default:
				X_ERROR("TechDef", "Unexpected token \"%.*s\" File: %s:%" PRId32, token.length(), token.begin(),
					lex.GetFileName(), lex.GetLineNumber());
				return false;			
		}
	}

#if 0 // now sure how i want to handle this at runtime just yet.
	// now move any samplers that are static.
	if (samplers_.isNotEmpty())
	{
		for (auto it = samplers_.begin(); it != samplers_.end();)
		{
			const auto& sampler = it->second;

			if (sampler.isFilterDefined() && sampler.isRepeateDefined())
			{
				staticSamplers_.push_back(*it);
				it = samplers_.erase(it);
			}
			else
			{
				++it;
			}
		}
	}
#endif

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
		X_ERROR("TechDef", "BlendState with name \"%s\" redefined in File: %s:%" PRIi32, name.c_str(), lex.GetFileName(), lex.GetLineNumber());
		return false;
	}

	render::BlendState& blend = addBlendState(name, parentName);

	return parseBlendStateData(lex, blend);
}

bool TechSetDef::parseBlendStateData(core::XParser& lex, render::BlendState& blend)
{
	// enagble all by default.
	blend.writeMask = render::WriteMaskFlags(render::WriteMask::RED | render::WriteMask::GREEN | render::WriteMask::BLUE | render::WriteMask::ALPHA);

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
		X_ERROR("TechDef", "Expected name token for BlendType. Line: %" PRIi32, lex.GetLineNumber());
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
		X_ERROR("TechDef", "Expected name token for blendOp. Line: %" PRIi32, lex.GetLineNumber());
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
		X_ERROR("TechDef", "Expected string token for writeChannel mask. Line: %" PRIi32, lex.GetLineNumber());
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
		X_ERROR("TechDef", "StencilState with name \"%s\" redefined in File: %s:%" PRIi32, name.c_str(), lex.GetFileName(), lex.GetLineNumber());
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
		X_ERROR("TechDef", "Expected name token for stencilFunc. Line: %" PRIi32, lex.GetLineNumber());
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
		X_ERROR("TechDef", "Expected name token for stencilOp. Line: %" PRIi32, lex.GetLineNumber());
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
		X_ERROR("TechDef", "State with name \"%s\" redefined in File: %s:%" PRIi32, name.c_str(), lex.GetFileName(), lex.GetLineNumber());
		return false;
	}

	auto& state = addState(name, parentName);

	return parseStateData(lex, state);
}

bool TechSetDef::parseStateData(core::XParser& lex, render::StateDesc& state)
{
	MaterialPolygonOffset::Enum polyOffset;

	if (!lex.ExpectTokenString("{")) {
		return false;
	}

	core::XLexToken token;
	while (lex.ReadToken(token))
	{
		if (token.isEqual("}")) {
			return true;
		}

		
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

				// work out if blend state should be anbled.
				if (
					state.blend.colorOp != render::BlendOp::OP_ADD ||
					state.blend.dstBlendColor != render::BlendType::ONE ||
					state.blend.srcBlendColor != render::BlendType::ZERO ||
					state.blend.alphaOp != render::BlendOp::OP_ADD ||
					state.blend.dstBlendAlpha != render::BlendType::ONE ||
					state.blend.srcBlendAlpha != render::BlendType::ZERO ||
					// enable if wrtie mask diffrent?
					!state.blend.writeMask.AreAllSet()
					)
				{
					// check for disabled op's also.
					if (state.blend.colorOp != render::BlendOp::DISABLE && state.blend.alphaOp != render::BlendOp::DISABLE)
					{
						state.stateFlags.Set(render::StateFlag::BLEND);
					}
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
				else {
					state.stateFlags.Remove(render::StateFlag::DEPTHWRITE);
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

				state.stateFlags.Remove(render::StateFlag::NO_DEPTH_TEST);
				break;
			case "polygonOffset"_fnv1a:
				if (!parsePolyOffset(lex, polyOffset)) {
					return false;
				}
				break;
			case "primitiveType"_fnv1a:
				if (!parsePrimitiveType(lex, state.topo)) {
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
		X_ERROR("TechDef", "Expected numeric token for stencilRef. Line: %" PRIi32, lex.GetLineNumber());
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
		X_ERROR("TechDef", "Expected name token for cullMode. Line: %" PRIi32, lex.GetLineNumber());
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
		X_ERROR("TechDef", "Expected name token for depthFunc. Line: %" PRIi32, lex.GetLineNumber());
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
		X_ERROR("TechDef", "Expected string token for polyOffset. Line: %" PRIi32, lex.GetLineNumber());
		return false;
	}

	polyOffset = Util::MatPolyOffsetFromStr(token.begin(), token.end());
	return true;
}


bool TechSetDef::parsePrimitiveType(core::XParser& lex, render::TopoType::Enum& topo)
{
	return parseHelper<render::TopoType::Enum>(lex, topo, &TechSetDef::parsePrimitiveTypeData,
		&TechSetDef::primTypeExsists, "State", "PrimitiveType");
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
		X_ERROR("TechDef", "PrimType with name \"%s\" redefined in File: %s:%" PRIi32, name.c_str(), lex.GetFileName(), lex.GetLineNumber());
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

	if (shaderExsists(name, stage)) {
		X_ERROR("TechDef", "Shader with name \"%s\" redefined in File: %s:%" PRIi32, name.c_str(), lex.GetFileName(), lex.GetLineNumber());
		return false;
	}

	auto& shader = addShader(name, parentName, stage);


	return parseShaderData(lex, shader);
}

bool TechSetDef::parseShaderData(core::XParser& lex, Shader& shader)
{
	core::XLexToken token;
	while (lex.ReadToken(token))
	{
		if (token.isEqual("}")) {
			return true;
		}

		
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
			{
				// we support 'aliasing' which basically allows mapping values to resource names in shader.
				// so for example if a shader has 3 samplers but you only want to expose 1 sampler to assMan
				// but you want the value of that sampler to be used in all 3, you can just alias the other two
				Alias al;
				al.resourceName = core::string(token.begin(), token.end());

				if (!lex.ExpectTokenString("=")) {
					return false;
				}

				// this can just be a simple name alias or currently a 'CodeTexture'
				// which just means the resource if aliased from a texture defined at runtime.
				if (!lex.ReadToken(token)) {
					return false;
				}

				if (token.GetType() == core::TokenType::NAME)
				{
					// only codeTexture allowed currently for named shit.
					if (!token.isEqual("CodeTexture")) {
						return false;
					}

					if (!lex.ExpectTokenString("(")) {
						return false;
					}

					if (!lex.ReadToken(token)) {
						return false;
					}

					if (!lex.ExpectTokenString(")")) {
						return false;
					}

					al.isCode = true;
				}
				else
				{
					// do nothing the token contains the name.
					al.isCode = false;
				}

				X_ASSERT(token.length() > 0, "alias name is empty")(token.begin(), token.end(), token.length());

				al.name = core::string(token.begin(), token.end());
				al.nameHash = core::StrHash(token.begin(), token.length());

				shader.aliases.append(al);
			}
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
		X_ERROR("TechDef", "Technique with name \"%s\" redefined in File: %s:%" PRIi32, name.c_str(), lex.GetFileName(), lex.GetLineNumber());
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
		if (shaderExsists(name, type, &shader)) {
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
			// inline define can have a parent.
			// but it must exist if defined.
			if (!shaderExsists(parentName, type, &shader)) {
				X_ERROR("TechDef", "Tech has a inline %s define with a undefined parent of: %s \"%s\" File: %s:%" PRIi32,
					defName.c_str(), parentName.c_str(), lex.GetFileName(), lex.GetLineNumber());
				return false;
			}

			// if we selected a parent it should be impossible for it to have a diffrent stage.
			// even if the user wanted to. this is source code logic fail.
			X_ASSERT(shader.type == type, "Parent not from same stage")(shader.type, type);
		}

		if (!lex.ExpectTokenString("{")) {
			return false;
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

bool TechSetDef::parseParamFloat1(core::XParser& lex)
{
	return parseParamHelper(lex, ParamType::Float1, [](core::XParser& lex, Param& param, const core::XLexToken& token, core::Hash::Fnv1aVal hash) -> bool {
			switch (hash)
			{
				case "x"_fnv1a:
					if (!parseParamFloat(lex, param.vec4Props[0], param.vec4.x)) {
						return false;
					}
					break;
				default:
					X_ERROR("TechDef", "Unknown Float1 prop: \"%.*s\"", token.length(), token.begin());
					return false;
			}
			return true;
		}
	);
}

bool TechSetDef::parseParamFloat2(core::XParser& lex)
{
	return parseParamHelper(lex, ParamType::Float2, [](core::XParser& lex, Param& param, const core::XLexToken& token, core::Hash::Fnv1aVal hash) -> bool {
			switch (hash)
			{
				case "x"_fnv1a:
					if (!parseParamFloat(lex, param.vec4Props[0], param.vec4.x)) {
						return false;
					}
					break;
				case "y"_fnv1a:
					if (!parseParamFloat(lex, param.vec4Props[1], param.vec4.y)) {
						return false;
					}
					break;
				default:
					X_ERROR("TechDef", "Unknown Float2 prop: \"%.*s\"", token.length(), token.begin());
					return false;
			}
			return true;
		}
	);
}

bool TechSetDef::parseParamFloat4(core::XParser& lex)
{
	return parseParamHelper(lex, ParamType::Float4, [](core::XParser& lex, Param& param, const core::XLexToken& token, core::Hash::Fnv1aVal hash) -> bool {
			switch (hash)
			{
				case "x"_fnv1a:
					if (!parseParamFloat(lex, param.vec4Props[0], param.vec4.x)) {
						return false;
					}
					break;
				case "y"_fnv1a:
					if (!parseParamFloat(lex, param.vec4Props[1], param.vec4.y)) {
						return false;
					}
					break;
				case "z"_fnv1a:
					if (!parseParamFloat(lex, param.vec4Props[2], param.vec4.z)) {
						return false;
					}
					break;
				case "w"_fnv1a:
					if (!parseParamFloat(lex, param.vec4Props[3], param.vec4.w)) {
						return false;
					}
					break;
				default:
					X_ERROR("TechDef", "Unknown Float4 prop: \"%.*s\"", token.length(), token.begin());
					return false;
			}
			return true;
		}
	);
}

bool TechSetDef::parseParamColor(core::XParser& lex)
{
	// for color it's just single prop.
	return parseParamHelper(lex, ParamType::Color, [](core::XParser& lex, Param& param, const core::XLexToken& token, core::Hash::Fnv1aVal hash) -> bool {
		
		bool isExplicit = false;
		switch (hash)
		{
			case "value"_fnv1a:
				if (!parsePropName(lex, param.vec4Props[0], isExplicit)) {
					X_ERROR("TechDef", "Failed to parse repeat prop");
					return false;
				}
				break;
			default:
				X_ERROR("TechDef", "Unknown Color prop: \"%.*s\"", token.length(), token.begin());
				return false;

		}
		return true;
	}
	);
}

bool TechSetDef::parseParamInt(core::XParser& lex)
{
	return parseParamHelper(lex, ParamType::Int, [](core::XParser& lex, Param& param, const core::XLexToken& token, core::Hash::Fnv1aVal hash) -> bool {
			switch (hash)
			{
				case "value"_fnv1a:
					if (!parseParamFloat(lex, param.vec4Props[0], param.vec4.x)) {
						return false;
					}
					break;
				default:
					X_ERROR("TechDef", "Unknown Int prop: \"%.*s\"", token.length(), token.begin());
					return false;
			}
			return true;
		}
	);
}

bool TechSetDef::parseParamBool(core::XParser& lex)
{
	return parseParamHelper(lex, ParamType::Bool, [](core::XParser& lex, Param& param, const core::XLexToken& token, core::Hash::Fnv1aVal hash) -> bool {
			switch (hash)
			{
				case "value"_fnv1a:
					if (!parseParamFloat(lex, param.vec4Props[0], param.vec4.x)) {
						return false;
					}
					break;
				default:
					X_ERROR("TechDef", "Unknown Bool prop: \"%.*s\"", token.length(), token.begin());
					return false;
			}
			return true;
		}
	);
}

bool TechSetDef::parseParamFloat(core::XParser& lex, core::string& propsName, float& val)
{
	// this is either a constant or prop name :D !
	if (!lex.ExpectTokenString("=")) {
		return false;
	}

	core::XLexToken token;
	if (!lex.ReadToken(token)) {
		return false;
	}

	if(token.GetType() == core::TokenType::PUNCTUATION && token.GetPuncId() == core::PunctuationId::LOGIC_LESS)
	{
		// it's a prop.
		if (!lex.ReadToken(token)) {
			return false;
		}

		if (token.GetType() != core::TokenType::NAME) {
			return false;
		}

		propsName = core::string(token.begin(), token.end());
		val = 0;

		if (!lex.ExpectTokenString(">")) {
			return false;
		}
	}
	else
	{
		// we want a float.
		if (token.GetType() != core::TokenType::NUMBER) {
			return false;
		}

		val = token.GetFloatValue();
	}

	return true;
}

bool TechSetDef::parseParamTextureSlot(core::XParser& lex, render::TextureSlot::Enum& texSlot)
{
	if (!lex.ExpectTokenString("=")) {
		return false;
	}

	core::XLexToken token;
	if (!lex.ReadToken(token)) {
		return false;
	}

	if (token.GetType() != core::TokenType::STRING) {
		X_ERROR("TechDef", "Expected string token for TextureSlot. Line: %" PRIi32, lex.GetLineNumber());
		return false;
	}

	texSlot = Util::TextureSlotFromStr(token.begin(), token.end());
	return true;
}

bool TechSetDef::parseParamTexture(core::XParser& lex)
{
	core::string name, parentName;

	// we have the name.
	if (!parseName(lex, name, parentName)) {
		return false;
	}

	if (!lex.ExpectTokenString("{")) {
		return false;
	}

	if (samplerExsists(name)) {
		X_ERROR("TechDef", "Texture with name \"%s\" redefined in File: %s:%" PRIi32, name.c_str(), lex.GetFileName(), lex.GetLineNumber());
		return false;
	}

	auto& texture = addTexture(name, parentName);

	// parse it.
	// this is texture specific fields.
	core::XLexToken token;
	while (lex.ReadToken(token))
	{
		if (token.isEqual("}")) {
			return true;
		}

		
		switch (core::Hash::Fnv1aHash(token.begin(), token.length()))
		{
			case "image"_fnv1a:
				if (!parseParamTextureData(lex, texture)) {
					return false;
				}
				break;
			case "semantic"_fnv1a:
				if (!parseParamTextureSlot(lex, texture.texSlot)) {
					return false;
				}
				break;

			case "ass"_fnv1a:
				if (!parseAssPropsData(lex, texture.assProps)) {
					return false;
				}
				break;

			default:
				X_ERROR("TechDef", "Unknown Texture prop: \"%.*s\"", token.length(), token.begin());
				return false;
		}
	}

	// failed to read token
	return false;
}


bool TechSetDef::parseParamTextureData(core::XParser& lex, Texture& texture)
{
	if (!lex.ExpectTokenString("=")) {
		return false;
	}

	if (!lex.ExpectTokenString("Image")) {
		return false;
	}

	if (!lex.ExpectTokenString("(")) {
		return false;
	}

	{
		// i want to support like defaults
		// so for normal maps etc I can specify a default texture name.
		// but I also want to be able to specify textures that only exsist at runtime.
		// i guess i don't really need any special logic for that just need to give the runtime textures names.

		// "iamgeName" : hardcoded image name
		// <propName> : the property name
		// <proName, imageName> : property name followed by hardcoded default.

		core::XLexToken token;
		if (!lex.ReadToken(token)) {
			return false;
		}

		if (token.GetType() == core::TokenType::NAME || token.GetType() == core::TokenType::STRING)
		{
			// this is just hard coded image name.
			texture.defaultName = core::string(token.begin(), token.end());
		}
		else if(token.GetType() == core::TokenType::PUNCTUATION && token.GetPuncId() == core::PunctuationId::LOGIC_LESS)
		{
			if (!lex.ReadToken(token)) {
				return false;
			}

			if (token.GetType() != core::TokenType::NAME) {
				return false;
			}

			texture.propName = core::string(token.begin(), token.end());

			// now we have optional default value.
			{
				if (!lex.ReadToken(token)) {
					return false;
				}

				if (token.GetType() == core::TokenType::PUNCTUATION && token.GetPuncId() == core::PunctuationId::COMMA)
				{
					// default value.
					if (!lex.ReadToken(token)) {
						return false;
					}

					if (token.GetType() != core::TokenType::NAME && token.GetType() != core::TokenType::STRING) {
						return false;
					}

					texture.defaultName = core::string(token.begin(), token.end());
				}
				else
				{
					lex.UnreadToken(token);
				}
			}

			if (!lex.ExpectTokenString(">")) {
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	if (!lex.ExpectTokenString(")")) {
		return false;
	}

	return true;
}

bool TechSetDef::parseParamSampler(core::XParser& lex)
{
	core::string name, parentName;

	// we have the name.
	if (!parseName(lex, name, parentName)) {
		return false;
	}

	if (!lex.ExpectTokenString("{")) {
		return false;
	}

	if (samplerExsists(name)) {
		X_ERROR("TechDef", "Sampler with name \"%s\" redefined in File: %s:%" PRIi32, name.c_str(), lex.GetFileName(), lex.GetLineNumber());
		return false;
	}

	auto& sampler = addSampler(name, parentName);

	// parse it.
	// this is texture specific fields.
	core::XLexToken token;
	while (lex.ReadToken(token))
	{
		if (token.isEqual("}")) {
			return true;
		}

		
		bool isExplicit;

		switch (core::Hash::Fnv1aHash(token.begin(), token.length()))
		{
			case "repeat"_fnv1a:
				// we need to parse it can be a string or punctiation.
				if (!parsePropName(lex, sampler.repeatStr, isExplicit)) {
					X_ERROR("TechDef", "Failed to parse repeat prop");
					return false;
				}

				if (isExplicit) {
					sampler.repeat = Util::TexRepeatFromStr(sampler.repeatStr.begin(), sampler.repeatStr.end());
				}
				break;
			case "filter"_fnv1a:
				if (!parsePropName(lex, sampler.filterStr, isExplicit)) {
					X_ERROR("TechDef", "Failed to parse filter prop");
					return false;
				}

				if (isExplicit) {
					sampler.filter = Util::FilterTypeFromStr(sampler.filterStr.begin(), sampler.filterStr.end());
				}
				break;

			case "ass"_fnv1a:
				if (!parseAssPropsData(lex, sampler.assProps)) {
					return false;
				}
				break;

			default:
				X_ERROR("TechDef", "Unknown Sampler state prop: \"%.*s\"", token.length(), token.begin());
				return false;
		}
	}

	// failed to read token
	return false;
}

bool TechSetDef::parsePropName(core::XParser& lex, core::string& str, bool& isExplicit)
{
	if (!lex.ExpectTokenString("=")) {
		return false;
	}

	core::XLexToken token;
	if (!lex.ReadToken(token)) {

		return false;
	}

	if (token.GetType() == core::TokenType::PUNCTUATION)
	{
		if (token.GetPuncId() != core::PunctuationId::LOGIC_LESS) {
			return false;
		}

		if (!lex.ReadToken(token)) {

			return false;
		}

		str = core::string(token.begin(), token.end());
		isExplicit = false;

		if (!lex.ExpectTokenString(">")) {
			return false;
		}
	}
	else if(token.GetType() == core::TokenType::STRING)
	{
		str = core::string(token.begin(), token.end());
		isExplicit = true;
	}
	else
	{
		return false;
	}

	return true;
}

bool TechSetDef::parseParamHelper(core::XParser& lex, ParamType::Enum type, ParamParseFunction parseFieldsFunc)
{
	core::string name, parentName;

	// we have the name.
	if (!parseName(lex, name, parentName)) {
		return false;
	}

	if (!lex.ExpectTokenString("{")) {
		return false;
	}

	if (paramExsists(name)) {
		X_ERROR("TechDef", "Param with name \"%s\" redefined in File: %s:%" PRIi32, name.c_str(), lex.GetFileName(), lex.GetLineNumber());
		return false;
	}

	auto& param = addParam(name, parentName, type);

	core::XLexToken token;
	while (lex.ReadToken(token))
	{
		if (token.isEqual("}")) {
			return true;
		}

		
		const auto hash = core::Hash::Fnv1aHash(token.begin(), token.length());
		switch (hash)
		{
			case "ass"_fnv1a:
				if (!parseAssPropsData(lex, param.assProps)) {
					return false;
				}
				break;

			default:
				if (!parseFieldsFunc(lex, param, token, hash)) {
					return false;
				}
				break;
		}
	}

	// failed to read token
	return false;
}



bool TechSetDef::parseAssPropsData(core::XParser& lex, AssManProps& props)
{
	core::string name, parentName;

	if (!parseInlineDefine(lex, name, parentName, "AssProps")) {
		return false;
	}

	if (parentName.isNotEmpty()) {
		X_ERROR("TechDef", "Parent name not supported for 'AssProps' File: %s:%" PRId32, lex.GetFileName(), lex.GetLineNumber());
		return false;
	}

	if (!lex.ExpectTokenString("{")) {
		return false;
	}

	core::XLexToken token;
	while (lex.ReadToken(token))
	{
		if (token.isEqual("}")) {
			return true;
		}

		
		switch (core::Hash::Fnv1aHash(token.begin(), token.length()))
		{
			case "cat"_fnv1a:
				if (!parseString(lex, props.cat)) {
					return false;
				}
				break;
			case "title"_fnv1a:
				if (!parseString(lex, props.title)) {
					return false;
				}
				break;

			default:
				X_ERROR("TechDef", "Unknown AssProp prop: \"%.*s\" File: %s:%" PRId32, token.length(), token.begin(),
					lex.GetFileName(), lex.GetLineNumber());
				return false;
		}
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
		X_ERROR("TechDef", "Expected string token for name. Line: %" PRIi32, lex.GetLineNumber());
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
				X_ERROR("TechDef", "Failed to read parent name. Line: %" PRIi32, lex.GetLineNumber());
				return false;
			}

			if (token.GetType() != core::TokenType::STRING && token.GetType() != core::TokenType::NAME) {
				X_ERROR("TechDef", "Expected string/name token for parent name. Line: %" PRIi32, lex.GetLineNumber());
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
				X_ERROR("TechDef", "Failed to read parent name. File: %s:%" PRIi32, lex.GetFileName(), lex.GetLineNumber());
				return false;
			}

			if (token.GetType() != core::TokenType::STRING && token.GetType() != core::TokenType::NAME) {
				X_ERROR("TechDef", "Expected string/name token for parent name. File: %s:%" PRIi32, lex.GetFileName(), lex.GetLineNumber());
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

bool TechSetDef::shaderExsists(const core::string& name, render::shader::ShaderType::Enum type, Shader* pShaderOut)
{
	core::string mergedName(name);
	mergedName += render::shader::ShaderType::ToString(type);

	return findHelper(shaders_, mergedName, pShaderOut);
}

bool TechSetDef::techniqueExsists(const core::string& name)
{
	return findHelper(techs_, name) != techs_.end();
}

bool TechSetDef::primTypeExsists(const core::string& name, render::TopoType::Enum* pTopo)
{
	return findHelper(prims_, name, pTopo);
}

bool TechSetDef::paramExsists(const core::string& name, Param* pParam)
{
	return findHelper(params_, name, pParam);
}

bool TechSetDef::textureExsists(const core::string& name, Texture* pTexture)
{
	return findHelper(textures_, name, pTexture);
}

bool TechSetDef::samplerExsists(const core::string& name, Sampler* pSampler)
{
	return findHelper(samplers_, name, pSampler);
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
	auto& state =  addHelper(states_, name, parentName, "state");

	if (parentName.isEmpty())
	{
		// defaults.
		state.topo = render::TopoType::TRIANGLELIST;
		state.vertexFmt = render::shader::VertexFormat::P3F_T2S_C4B;
		state.stateFlags.Set(render::StateFlag::NO_DEPTH_TEST);
		state.depthFunc = render::DepthFunc::ALWAYS;
	}

	return state;
}

Shader& TechSetDef::addShader(const core::string& name, const core::string& parentName, render::shader::ShaderType::Enum type)
{
	core::string mergedName(name);
	mergedName += render::shader::ShaderType::ToString(type);
			
	Shader& shader = addHelper(shaders_, mergedName, parentName, "state", type);
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

Param& TechSetDef::addParam(const core::string& name, const core::string& parentName, ParamType::Enum type)
{
	auto& p = addHelper(params_, name, parentName, ParamType::ToString(type), type);
	return p;
}

Texture& TechSetDef::addTexture(const core::string& name, const core::string& parentName)
{
	auto& t = addHelper(textures_, name, parentName, "Texture");
	return t;
}

Sampler& TechSetDef::addSampler(const core::string& name, const core::string& parentName)
{
	auto& p = addHelper(samplers_, name, parentName, "Sampler");
	return p;
}


template<typename T>
X_INLINE bool TechSetDef::findHelper(NameArr<T>& arr, const core::string& name, T* pOut)
{
	auto it = findHelper(arr, name);

	if (it != arr.end()) {
		if (pOut) {
			*pOut = it->second;
		}
		return true;
	}

	return false;
}


template<typename T>
X_INLINE typename T::const_iterator TechSetDef::findHelper(T& arr, const core::string& name)
{
	return std::find_if(arr.begin(), arr.end(), [&name](const typename T::Type& inst) {
		return inst.first == name;
	});
}

template<typename T>
X_INLINE T& TechSetDef::addHelper(NameArr<T>& arr,
	const core::string& name, const core::string& parentName, const char* pNick)
{
	return addHelper(arr, name, parentName, pNick, T());
}

template<typename T, class... Args>
X_INLINE T& TechSetDef::addHelper(NameArr<T>& arr,
	const core::string& name, const core::string& parentName, const char* pNick, Args&&... args)
{
	if (!parentName.isEmpty())
	{
		auto it = findHelper(arr, parentName);
		if (it != arr.end())
		{
			arr.push_back(std::make_pair(name, it->second));
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
		arr.emplace_back(name, std::forward<Args>(args)...);
	}

	return arr.back().second;
}

} // namespace techset

X_NAMESPACE_END
