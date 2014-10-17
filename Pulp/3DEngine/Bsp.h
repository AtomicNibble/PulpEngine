#pragma once

#ifndef X_BSP_LOADER_H_
#define X_BSP_LOADER_H_

#include <IBsp.h>
// #include <IModel.h>

X_NAMESPACE_DECLARE(model,
struct MeshHeader;
struct IRenderMesh;
)


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

	void free(void);
	bool render(void);

	bool LoadFromFile(const char* filename);


private:
	BSPData data_;


	struct AreaModel
	{
		AreaModel() {
			pMesh = nullptr;
			pRenderMesh = nullptr;
		}

		model::MeshHeader* pMesh;
		model::IRenderMesh* pRenderMesh;
	};

	core::Array<AreaModel> areaModels_;

	uint8_t* pFileData_;
};


X_NAMESPACE_END

#endif // !X_BSP_LOADER_H_