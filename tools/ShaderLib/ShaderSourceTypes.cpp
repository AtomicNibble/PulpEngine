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


	void SourceFile::applyRefrences(void)
	{
		for (const auto& pIncSource : includedFiles_) {
			pIncSource->addRefrence(name_);
		}
	}


} // namespace shader

X_NAMESPACE_END