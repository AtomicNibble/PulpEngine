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

	SourceFile::SourceFile(const core::string& name, core::MemoryArenaBase* arena) :
		name_(name),
		fileData_(arena),
		includedFiles_(arena),
		sourceCrc32_(0)
	{

	}

	void SourceFile::writeSourceToFile(core::XFile* pFile) const
	{
		LockType::ScopedLockShared readLock(lock);

		pFile->printf("\n// ======== %s ========\n\n", getName().c_str());
		pFile->write(fileData_.begin(), fileData_.size());
	}


} // namespace shader

X_NAMESPACE_END