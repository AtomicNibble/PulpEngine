#pragma once

#ifndef X_BSP_LOADER_H_
#define X_BSP_LOADER_H_

#include <IBsp.h>

X_NAMESPACE_BEGIN(bsp)


struct BSPData
{
	BSPData(core::MemoryArenaBase* arena) :
	areas(arena),
	surfaces(arena),
	verts(arena),
	indexes(arena)
	{}

	core::Array<bsp::Area> areas;
	core::Array<bsp::Surface> surfaces;
	core::Array<bsp::Vertex> verts;
	core::Array<bsp::Index> indexes;
};

class Bsp
{
public:
	Bsp();
	~Bsp();

	bool LoadFromFile(const char* filename);


private:

	BSPData data_;
};


X_NAMESPACE_END

#endif // !X_BSP_LOADER_H_