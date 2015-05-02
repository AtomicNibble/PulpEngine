#pragma once

#ifndef X_BSP_DATA_H_
#define X_BSP_DATA_H_

#include <IBsp.h>


struct BSPData
{
	BSPData(core::MemoryArenaBase* arena) :
	areas(arena),
	surfaces(arena),
	verts(arena),
	indexes(arena)
	{}

	core::Array<level::Area> areas;
	core::Array<level::Surface> surfaces;
	core::Array<level::Vertex> verts;
	core::Array<level::Index> indexes;
};

#endif // !X_BSP_DATA_H_