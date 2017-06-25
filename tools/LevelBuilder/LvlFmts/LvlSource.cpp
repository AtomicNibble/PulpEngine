#include "stdafx.h"
#include "LvlSource.h"

X_NAMESPACE_BEGIN(lvl)



LvlSource::LvlSource(core::MemoryArenaBase* arena, ModelCache& modelCache, MatManager& matMan, XPlaneSet& planes) :
	arena_(arena),
	modelCache_(modelCache),
	matMan_(matMan),
	entities_(arena),
	planes_(planes)
{

}

LvlSource::~LvlSource()
{

}


void LvlSource::calculateLvlBounds(void)
{
	mapBounds_.clear();

	// bound me baby
	for (const auto& ent : entities_)
	{
		for (const auto& brush : ent.brushes)
		{
			mapBounds_.add(brush.bounds);
		}
	}
}


X_NAMESPACE_END