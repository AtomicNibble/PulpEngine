#include "stdafx.h"
#include "ShaderSourceTypes.h"

#include <String\Lexer.h>
#include <String\StringTokenizer.h>

#include <IFileSys.h>
#include <IShader.h>

X_NAMESPACE_BEGIN(render)

namespace shader
{
	// -------------------------------------------------------------

	SourceFile::SourceFile(const core::string& name, core::MemoryArenaBase* arena) :
		name_(name),
		fileData_(arena),
		includedFiles_(arena),
		refrences_(arena),
		sourceCrc32_(0)
	{

	}

	void SourceFile::writeSourceToFile(core::XFile* pFile) const
	{
		LockType::ScopedLockShared readLock(lock);

		pFile->printf("\n// ======== %s ========\n\n", getName().c_str());
		pFile->write(fileData_.begin(), fileData_.size());
	}


	void SourceFile::applyRefrences(void) const
	{
#if X_ENABLE_RENDER_SHADER_RELOAD
		for (const auto& pIncSource : includedFiles_) {
			pIncSource->addRefrence(name_);
		}
#endif // !X_ENABLE_RENDER_SHADER_RELOAD
	}

	void SourceFile::removeRefrences(void) const
	{
#if X_ENABLE_RENDER_SHADER_RELOAD
		for (const auto& pIncSource : includedFiles_) {
			pIncSource->removeRefrence(name_);
		}
#endif // !X_ENABLE_RENDER_SHADER_RELOAD
	}

} // namespace shader

X_NAMESPACE_END