#pragma once

#ifndef X_BSP_LOADER_H_
#define X_BSP_LOADER_H_

#include <IBsp.h>
// #include <IModel.h>

X_NAMESPACE_DECLARE(model,
struct MeshHeader;
struct IRenderMesh;
)


X_NAMESPACE_BEGIN(level)


class Level
{
public:
	Level();
	~Level();

	void free(void);
	bool render(void);

	bool LoadFromFile(const char* filename);


private:

	uint32_t numAreas_;


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