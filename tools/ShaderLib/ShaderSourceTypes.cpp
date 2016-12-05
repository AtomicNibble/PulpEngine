#include "stdafx.h"
#include "ShaderSourceTypes.h"

#include <String\Lexer.h>
#include <String\StringTokenizer.h>

#include <IFileSys.h>
#include <IShader.h>

X_NAMESPACE_BEGIN(render)

namespace shader
{
	namespace
	{

		static bool TechFlagFromStr(const char* pStr, TechFlags& flagOut)
		{
			using namespace core::Hash::Fnva1Literals;

			core::StackString<128, char> strUpper(pStr);
			strUpper.toUpper();

			static_assert(TechFlags::FLAGS_COUNT == 4, "TechFlag count changed? this code needs updating.");
			switch (core::Hash::Fnv1aHash(strUpper.c_str(), strUpper.length()))
			{
				case "COLOR"_fnv1a:
					flagOut = TechFlags::Color;
				case "TEXTURED"_fnv1a:
					flagOut = TechFlags::Textured;
				case "SKINNED"_fnv1a:
					flagOut = TechFlags::Skinned;
				case "INSTANCED"_fnv1a:
					flagOut = TechFlags::Instanced;
				default:
					return false;
			}

			return true;
		}

	} // namespace



	  // -------------------------------------------------------------

	SourceFile::SourceFile(core::MemoryArenaBase* arena) :
		includedFiles_(arena),
		sourceCrc32_(0)
	{

	}

	void SourceFile::writeSourceToFile(core::XFile* pFile) const
	{
		pFile->printf("\n// ======== %s ========\n\n", getFileName().c_str());
		pFile->writeStringNNT(getFileData());
	}


# if 0

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


#endif


} // namespace shader

X_NAMESPACE_END