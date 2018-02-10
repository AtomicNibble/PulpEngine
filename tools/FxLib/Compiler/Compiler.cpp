#include "stdafx.h"
#include "Compiler.h"

#include <IFileSys.h>


X_NAMESPACE_BEGIN(engine)

namespace fx
{

	EffectCompiler::EffectCompiler(core::MemoryArenaBase* arena) :
		arena_(arena),
		stages_(arena)
	{

	}

	EffectCompiler::~EffectCompiler()
	{

	}

	bool EffectCompiler::writeToFile(core::XFile* pFile) const
	{
		X_UNUSED(pFile);

		EffectHdr hdr;
		hdr.fourCC = EFFECT_FOURCC;
		hdr.version = EFFECT_VERSION;
		hdr.numStages = 0;

		if (pFile->writeObj(hdr) != sizeof(hdr)) {
			X_ERROR("Effect", "Failed to write header");
			return false;
		}

		// write the stages.


		return true;
	}



} // namespace fx


X_NAMESPACE_END