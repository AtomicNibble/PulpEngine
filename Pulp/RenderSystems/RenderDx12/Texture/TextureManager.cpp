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
		X_LOG1("TextureManager", "Starting");



		return true;
	}

	bool TextureManager::shutDown(void)
	{
		X_LOG1("TextureManager", "Shutting Down");



		return true;
	}


} // namespace


X_NAMESPACE_END