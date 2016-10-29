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
		
	}

	bool ShaderSourceFileTechnique::parse(core::XLexer& lex)
	{
		using namespace render;

		core::XLexToken token;

		flags_.Clear();

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
				X_WARNING("Shader", "cull_mode is deprecated");
			}
			else if (key.isEqual("depth_test"))
			{
				X_WARNING("Shader", "depth_test is deprecated");
			}
			else if (key.isEqual("depth_write"))
			{
				X_WARNING("Shader", "depth_write is deprecated");
			}
			else if (key.isEqual("wireframe"))
			{
				X_WARNING("Shader", "wireframe is deprecated");
			}
			else if (key.isEqual("src_blend"))
			{
				X_WARNING("Shader", "src_blend is deprecated");
			}
			else if (key.isEqual("dst_blend"))
			{
				X_WARNING("Shader", "dst_blend is deprecated");
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

		// did we reach EOF before close brace?
		if (!token.isEqual("}")) {
			X_ERROR("Shader", "technique missing closing brace");
			return false;
		}

		return processName();
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