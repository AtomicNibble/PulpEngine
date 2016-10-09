#include "stdafx.h"
#include "ShaderSourceTypes.h"

#include <String\Lexer.h>
#include <String\StringTokenizer.h>

X_NAMESPACE_BEGIN(render)

namespace shader
{
	namespace
	{
		struct TechFlagEntry
		{
			const char* pName;
			TechFlag::Enum flag;
		};

		TechFlagEntry g_TechFlags[] = {
			{ "Color", TechFlag::Color },
			{ "Textured", TechFlag::Textured },
			{ "Skinned", TechFlag::Skinned },
			{ "Instanced", TechFlag::Instanced },
		};


		static bool TechFlagFromStr(const char* pStr, TechFlags& flagOut)
		{
			const size_t num = sizeof(g_TechFlags) / sizeof(const char*);
			size_t i;
			for (i = 0; i < num; i++)
			{
				if (core::strUtil::IsEqualCaseInsen(pStr, g_TechFlags[i].pName))
				{
					flagOut.Set(g_TechFlags[i].flag);
					return true;
				}
			}
			return false;
		}

	} // namespace



	// -------------------------------------------------------------

	SourceFile::SourceFile(core::MemoryArenaBase* arena) :
		includedFiles_(arena),
		prePros_(arena),
		sourceCrc32_(0)
	{

	}

	ShaderSourceFile::ShaderSourceFile(core::MemoryArenaBase* arena) :
		pFile_(nullptr),
		pHlslFile_(nullptr),
		sourceCrc32_(0),
		hlslSourceCrc32_(0),
		techniques_(arena)
	{

	}

	// -------------------------------------------------------------


	ShaderSourceFileTechnique::ShaderSourceFileTechnique()
	{
		depth_write_ = true;
	}

	bool ShaderSourceFileTechnique::parse(core::XLexer& lex)
	{
		using namespace render;

		core::XLexToken token;

		flags_.Clear();

		src_.color = BlendType::SRC_ALPHA;
		dst_.color = BlendType::INV_SRC_ALPHA;

		// lots of pairs :D !
		while (lex.ReadToken(token))
		{
			if (token.isEqual("}"))
				break;

			core::StackString512 key, value;

			// parse a key / value pair
			key.append(token.begin(), token.end());
			if (!lex.ReadTokenOnLine(token)) {
				X_ERROR("Shader", "unexpected EOF while reading technique, Line: %i", token.GetLine());
				return false;
			}
			value.append(token.begin(), token.end());

			// humm we only have a fixed number of valid values.
			// so lets just check them !
			if (key.isEqual("name"))
			{
				name_ = value.c_str();
				flags_.Set(TechniquePrams::NAME);
			}
			else if (key.isEqual("vertex_shader"))
			{
				vertex_func_ = value.c_str();
				flags_.Set(TechniquePrams::VERTEX_FNC);
			}
			else if (key.isEqual("pixel_shader"))
			{
				pixel_func_ = value.c_str();
				flags_.Set(TechniquePrams::PIXEL_FNC);
			}
			else if (key.isEqual("cull_mode"))
			{
				// none, front, back
				if (value.isEqual("none"))
					this->cullMode_ = CullMode::NONE;
				else if (value.isEqual("front"))
					this->cullMode_ = CullMode::FRONT;
				else if (value.isEqual("back"))
					this->cullMode_ = CullMode::BACK;
				else {
					X_WARNING("Shader", "invalid 'cull_mode' value, possible values: none/front/back");
				}
			}
			else if (key.isEqual("depth_test"))
			{
				//   NEVER, LESS, EQUAL, LESS_EQUAL, GREATER, NOT_EQUAL, GREATER_EQUAL, ALWAYS 
				if (value.isEqual("less_equal"))
					state_ |= States::DEPTHFUNC_LEQUAL;
				else if (value.isEqual("equal"))
					state_ |= States::DEPTHFUNC_EQUAL;
				else if (value.isEqual("greater"))
					state_ |= States::DEPTHFUNC_GREAT;
				else if (value.isEqual("less"))
					state_ |= States::DEPTHFUNC_LESS;
				else if (value.isEqual("greater_equal"))
					state_ |= States::DEPTHFUNC_GEQUAL;
				else if (value.isEqual("not_equal"))
					state_ |= States::DEPTHFUNC_NOTEQUAL;
				else if (value.isEqual("always"))
				{
					state_ |= States::NO_DEPTH_TEST;
				}
				else
				{
					X_WARNING("Shader", "invalid 'depth_test' value, possible values: "
						"less_equal/equal/greater/less/greater_equal/not_equal");
				}
			}
			else if (key.isEqual("depth_write"))
			{
				// true false.
				if (value.isEqual("true"))
					this->depth_write_ = true;
				else if (value.isEqual("false"))
					this->depth_write_ = false;
				else {
					X_WARNING("Shader", "invalid 'depth_write' value, possible values: true/false");
				}
			}
			else if (key.isEqual("wireframe"))
			{
				if (value.isEqual("true")) {
					state_ |= States::WIREFRAME;
				}
			}

			// we could have blend functions.
			else if (parseBlend(src_, "src_blend", key, value) ||
				parseBlend(dst_, "dst_blend", key, value))
			{
				// valid.
			}
			else
			{
				X_ERROR("Shader", "unknown technique param: %s", key.c_str());
			}
		}

		// check we have all the required shit.
		// they can be in any order so we check now.
		if (!flags_.IsSet(TechniquePrams::NAME)) {
			X_ERROR("Shader", "technique missing required param: name");
			return false;
		}
		if (!flags_.IsSet(TechniquePrams::VERTEX_FNC)) {
			X_ERROR("Shader", "technique missing required param: vertex_shader");
			return false;
		}
		if (!flags_.IsSet(TechniquePrams::PIXEL_FNC)) {
			X_ERROR("Shader", "technique missing required param: pixel_shader");
			return false;
		}


		if (depth_write_) {
			state_.Set(render::States::DEPTHWRITE);
		}

		// defaults
		if (src_.color == BlendType::INVALID) {
			src_.color = BlendType::SRC_ALPHA;
		}

		if (dst_.color == BlendType::INVALID) {
			dst_.color = BlendType::INV_SRC_ALPHA;
		}

		// build the state.
		switch (src_.color)
		{
		case BlendType::ZERO:
			state_.Set(render::States::BLEND_SRC_ZERO);
			break;
		case BlendType::ONE:
			state_.Set(render::States::BLEND_SRC_ONE);
			break;
		case BlendType::DEST_COLOR:
			state_.Set(render::States::BLEND_SRC_DEST_COLOR);
			break;
		case BlendType::INV_DEST_COLOR:
			state_.Set(render::States::BLEND_SRC_INV_DEST_COLOR);
			break;
		case BlendType::SRC_ALPHA:
			state_.Set(render::States::BLEND_SRC_SRC_ALPHA);
			break;
		case BlendType::INV_SRC_ALPHA:
			state_.Set(render::States::BLEND_SRC_INV_SRC_ALPHA);
			break;
		case BlendType::DEST_ALPHA:
			state_.Set(render::States::BLEND_SRC_DEST_ALPHA);
			break;
		case BlendType::INV_DEST_ALPHA:
			state_.Set(render::States::BLEND_SRC_INV_DEST_ALPHA);
			break;
		case BlendType::SRC_ALPHA_SAT:
			state_.Set(render::States::BLEND_SRC_ALPHA_SAT);
			break;
#if X_DEBUG
		default:
			X_ASSERT_UNREACHABLE();
#else
			X_NO_SWITCH_DEFAULT;
#endif
		}

		switch (dst_.color)
		{
		case BlendType::ZERO:
			state_.Set(render::States::BLEND_DEST_ZERO);
			break;
		case BlendType::ONE:
			state_.Set(render::States::BLEND_DEST_ONE);
			break;
		case BlendType::SRC_COLOR:
			state_.Set(render::States::BLEND_DEST_SRC_COLOR);
			break;
		case BlendType::INV_SRC_COLOR:
			state_.Set(render::States::BLEND_DEST_INV_SRC_COLOR);
			break;
		case BlendType::SRC_ALPHA:
			state_.Set(render::States::BLEND_DEST_SRC_ALPHA);
			break;
		case BlendType::INV_SRC_ALPHA:
			state_.Set(render::States::BLEND_DEST_INV_SRC_ALPHA);
			break;
		case BlendType::DEST_ALPHA:
			state_.Set(render::States::BLEND_DEST_DEST_ALPHA);
			break;
		case BlendType::INV_DEST_ALPHA:
			state_.Set(render::States::BLEND_DEST_INV_DEST_ALPHA);
			break;
#if X_DEBUG
		default:
			X_ASSERT_UNREACHABLE();
#else
			X_NO_SWITCH_DEFAULT;
#endif
		}

		// did we reach EOF before close brace?
		if (!token.isEqual("}")) {
			X_ERROR("Shader", "technique missing closing brace");
			return false;
		}

		return processName();
	}



	bool ShaderSourceFileTechnique::parseBlend(BlendInfo& blend, const char* pName,
		const core::StackString512& key, const core::StackString512& value)
	{
		X_ASSERT_NOT_NULL(pName);

		core::StackString<64> color_str(pName);
		core::StackString<64> alpha_str(pName);

		color_str.append("_color");
		alpha_str.append("_alpha");

		if (color_str.isEqual(key.c_str()))
		{
			blend.color = BlendType::typeFromStr(value.c_str());
			if (blend.color == BlendType::INVALID) {
				X_ERROR("Shader", "invalid %s type: %s", color_str.c_str(), value.c_str());
				return false; // invalid
			}
		}
		else if (alpha_str.isEqual(key.c_str()))
		{
			blend.alpha = BlendType::typeFromStr(value.c_str());
			if (blend.alpha == BlendType::INVALID) {
				X_ERROR("Shader", "invalid %s type: %s", alpha_str.c_str(), value.c_str());
				return false; // invalid
			}
		}
		else {
			return false;
		}

		return true;
	}

	bool ShaderSourceFileTechnique::processName(void)
	{
		const char* pBrace, *pCloseBrace;
		if ((pBrace = name_.find('(')) != nullptr)
		{
			// if we find a () 
			// we have diffrent compile macro's for the 
			// technique
			pCloseBrace = name_.find(')');
			if (pCloseBrace < pBrace)
			{
				X_ERROR("Shader", "invalid name for shader: %s", name_.c_str());
				return false;
			}

			core::StackString512 flags(pBrace + 1, pCloseBrace);
			core::StackString512 temp;

			if (flags.isNotEmpty())
			{
				core::StringTokenizer<char> tokens(flags.begin(), flags.end(), ',');
				core::StringRange<char> flagName(nullptr, nullptr);

				while (tokens.ExtractToken(flagName))
				{
					// valid tech flag?
					temp.set(flagName.GetStart(), flagName.GetEnd());
					if (!TechFlagFromStr(temp.c_str(), techFlags_))
					{
						X_WARNING("Shader", "not a valid tech flag: %s", temp.c_str());
					}
				}
			}

			// fix name.
			name_ = name_.substr(nullptr, pBrace);

		}
		return true;
	}


} // namespace shader

X_NAMESPACE_END