#pragma once

#ifndef X_BSP_DATA_H_
#define X_BSP_DATA_H_

#include <IBsp.h>


struct BSPData
{
	BSPData(core::MemoryArenaBase* arena) :
	surfaces(arena),
	verts(arena),
	indexes(arena)
	{}

	core::Array<bsp::Surface> surfaces;

	core::Array<bsp::Vertex> verts;
	core::Array<bsp::Index> indexes;
};

#endif // !X_BSP_DATA_H_