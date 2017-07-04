#pragma once

#include <Util\UniquePointer.h>
#include <String\GrowingStringTable.h>

#include <Ilevel.h>

#include "Enity\EnitiySystem.h"


X_NAMESPACE_DECLARE(core,
	namespace V2 {
		struct Job;
		class JobSystem;
	}
	
	struct XFileAsync;

	struct FrameData;
)



X_NAMESPACE_BEGIN(game)

namespace entity
{
	class EnititySystem;

} // namespace entity


class Level
{
public:
	Level(core::MemoryArenaBase* arena, entity::EnititySystem& entSys);
	~Level();

	void load(const char* pMapName);
	void free(void);

private:
	void IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
		core::XFileAsync* pFile, uint32_t bytesTransferred);

	void processHeader_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);
	void processData_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);
	bool processHeader(void);
	bool processData(void);


private:
	core::MemoryArenaBase* arena_;
	core::V2::JobSystem* pJobSys_;
	core::IFileSys* pFileSys_;
	entity::EnititySystem& entSys_;

	core::Path<char> path_;
	level::FileHeader fileHdr_;
	core::UniquePointer<uint8_t[]> levelData_;

	core::GrowingStringTable<256, 16, 4, uint32_t> stringTable_;


};


class World
{
public:
	World(core::MemoryArenaBase* arena);
	~World();

	bool init(physics::IPhysics* pPhys);
	void update(core::FrameData& frame);


private:
	bool createPhysicsScene(physics::IPhysics* pPhys);

private:
	physics::IScene* pScene_;

	
	entity::EnititySystem ents_;
	Level level_;
};


X_NAMESPACE_END