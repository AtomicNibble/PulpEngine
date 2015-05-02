#pragma once

#ifndef X_BSP_LOADER_H_
#define X_BSP_LOADER_H_

#include <Ilevel.h>
#include <Time\TimeVal.h>


X_NAMESPACE_DECLARE(model,
struct MeshHeader;
struct IRenderMesh;
)


X_NAMESPACE_BEGIN(level)


struct LoadStats
{
	core::TimeVal startTime;
	core::TimeVal endTime;
};

class Level
{
public:
	Level();
	~Level();

	void free(void);
	bool render(void);

	bool Load(const char* mapName);


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

private:
	FileHeader fileHdr_;

	LoadStats loadStats_;
};


X_NAMESPACE_END

#endif // !X_BSP_LOADER_H_