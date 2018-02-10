#include "stdafx.h"
#include "Effect.h"


X_NAMESPACE_BEGIN(engine)


Effect::Effect(core::string& name, core::MemoryArenaBase* arena) :
	core::AssetBase(name, assetDb::AssetType::FX)
{
	X_UNUSED(arena);
}

Effect::~Effect()
{

}



X_NAMESPACE_END