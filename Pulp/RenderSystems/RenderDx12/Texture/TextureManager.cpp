#include "stdafx.h"
#include "TextureManager.h"


X_NAMESPACE_BEGIN(render)

namespace texture
{

	TextureManager::TextureManager(core::MemoryArenaBase* arena) :
		arena_(arena)
	{

	}

	TextureManager::~TextureManager()
	{

	}

	bool TextureManager::init(void)
	{



		return true;
	}

	bool TextureManager::shutDown(void)
	{



		return true;
	}


} // namespace


X_NAMESPACE_END