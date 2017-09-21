#pragma once

#include <IWeapon.h>
#include <IDirectoryWatcher.h>

#include <Assets\AssertContainer.h>

#include "WeaponDef.h"

X_NAMESPACE_DECLARE(core,
namespace V2 {
	struct Job;
	class JobSystem;
}

struct XFileAsync;
struct IoRequestBase;
)

X_NAMESPACE_BEGIN(game)

namespace weapon
{
	class WeaponDef;

	struct WeaponDefLoadRequest
	{
		WeaponDefLoadRequest(WeaponDef* pWeaponDef) :
			pFile(nullptr),
			pWeaponDef(pWeaponDef),
			dataSize(0)
		{
		}

		core::XFileAsync* pFile;
		WeaponDef* pWeaponDef;
		core::UniquePointer<uint8_t[]> data;
		uint32_t dataSize;
	};


	class WeaponDefManager :
		public core::IXHotReload
	{
		typedef core::AssetContainer<WeaponDef, WEAPON_MAX_LOADED, core::SingleThreadPolicy> WeaponDefContainer;
		typedef WeaponDefContainer::Resource WeaponDefResource;

		typedef core::Array<WeaponDefLoadRequest*> WeaponDefLoadRequestArr;
		typedef core::Fifo<WeaponDefResource*> WeaponDefQueue;

	public:
		WeaponDefManager(core::MemoryArenaBase* arena);

		void registerCmds(void);
		void registerVars(void);

		bool init(void);
		void shutDown(void);

		bool asyncInitFinalize(void);


		WeaponDef* findWeaponDef(const char* pName) const;
		WeaponDef* loadWeaponDef(const char* pName);
		WeaponDef* getDefaultDef(void) const;

		bool waitForLoad(WeaponDef* pWeaponDef); // returns true if load succeed.
		void releaseWeaponDef(WeaponDef* pMat);

	private:
		bool initDefaults(void);
		void freeDangling(void);

		void addLoadRequest(WeaponDefResource* pWeaponDef);
		void dispatchLoad(WeaponDef* pWeaponDef, core::CriticalSection::ScopedLock&);
		void dispatchLoadRequest(WeaponDefLoadRequest* pLoadReq);


		// load / processing
		void onLoadRequestFail(WeaponDefLoadRequest* pLoadReq);
		void loadRequestCleanup(WeaponDefLoadRequest* pLoadReq);

		void IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
			core::XFileAsync* pFile, uint32_t bytesTransferred);

		void ProcessData_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);

		bool processData(WeaponDef* pWeaponDef, core::XFile* pFile);

	private:
		// IXHotReload
		virtual void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_FINAL;
		// ~IXHotReload

	private:
		core::MemoryArenaBase* arena_;
		core::MemoryArenaBase* blockArena_;

		WeaponDef* pDefaultWeaponDef_;

		// loading
		core::CriticalSection loadReqLock_;
		core::ConditionVariable loadCond_;

		WeaponDefLoadRequestArr pendingRequests_;

		WeaponDefContainer weaponDefs_;
	};


} // namespace weapon

X_NAMESPACE_END