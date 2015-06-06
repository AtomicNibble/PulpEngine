#pragma once

#ifndef X_BSP_LOADER_H_
#define X_BSP_LOADER_H_

#include "EngineBase.h"

#include <Ilevel.h>
#include <Time\TimeVal.h>

#include "String\GrowingStringTable.h"

X_NAMESPACE_DECLARE(model,
struct MeshHeader;
struct IRenderMesh;
)


X_NAMESPACE_BEGIN(level)


struct LoadStats
{
	core::TimeVal startTime;
	core::TimeVal elapse;
};

struct AsyncLoadData
{
	AsyncLoadData(core::XFileAsync* pFile, core::XFileAsyncOperation AsyncOp) :
	headerLoaded_(false),
	waitingForIo_(false),
	pFile_(pFile), 
	AsyncOp_(AsyncOp)
	{}

	~AsyncLoadData();

	bool waitingForIo_;
	bool headerLoaded_;
	core::XFileAsync* pFile_;
	core::XFileAsyncOperation AsyncOp_;
};

struct AreaModel
{
	AreaModel() {
		pMesh = nullptr;
		pRenderMesh = nullptr;
	}

	model::MeshHeader* pMesh;
	model::IRenderMesh* pRenderMesh;
};

class Level : public engine::XEngineBase
{
public:
	Level();
	~Level();	

	static bool Init(void);
	static void ShutDown(void);

	void update(void);

	void free(void);
	bool canRender(void);
	bool render(void);

	bool Load(const char* mapName);

private:
	bool ProcessHeader(uint32_t bytesRead);
	bool ProcessData(uint32_t bytesRead);

private:
	core::GrowingStringTable<256, 16, 4, uint32_t> stringTable_;

	core::Array<AreaModel> areaModels_;
	uint8_t* pFileData_;

	bool canRender_;
	bool _pad[3];

private:
	core::Path path_;
	FileHeader fileHdr_;
	LoadStats loadStats_;
	AsyncLoadData* pAsyncLoadData_;

private:
	core::ITimer* pTimer_;
	core::IFileSys* pFileSys_;

private:
	// vars
	static int s_var_drawAreaBounds_;
	static int s_var_drawArea_;
};


X_NAMESPACE_END

#endif // !X_BSP_LOADER_H_