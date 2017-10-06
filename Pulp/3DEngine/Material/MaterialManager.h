#pragma once

#ifndef X_3D_MATERIAL_MAN_H_
#define X_3D_MATERIAL_MAN_H_

#include <String\StrRef.h>

#include <Assets\AssertContainer.h>
#include <Util\UniquePointer.h>
#include <Containers\Fifo.h>
#include <Time\TimeVal.h>

#include "Vars\MaterialVars.h"

X_NAMESPACE_DECLARE(core, 
	namespace V2 {
		struct Job;
		class JobSystem;
	}

	struct XFileAsync;
	struct IoRequestBase;
	struct IConsoleCmdArgs;
)

X_NAMESPACE_BEGIN(engine)

namespace techset
{
	class TechSetDefs;

} // namespace techset

class VariableStateManager;
class TechDefStateManager;
class CBufferManager;
class Material;

struct MaterialLoadRequest
{
	MaterialLoadRequest(Material* pModel) :
		pFile(nullptr),
		pMaterial(pModel),
		dataSize(0)
	{
	}
	core::XFileAsync* pFile;
	Material* pMaterial;
	core::UniquePointer<uint8_t[]> data;
	uint32_t dataSize;
	core::TimeVal dispatchTime;
	core::TimeVal ioTime;
	core::TimeVal loadTime;
};


class XMaterialManager : 
	public IMaterialManager, 
	public ICoreEventListener, 
	public core::IXHotReload
{
	typedef core::AssetContainer<Material, MTL_MAX_LOADED, core::SingleThreadPolicy> MaterialContainer;
	typedef MaterialContainer::Resource MaterialResource;

	typedef core::Array<Material*> MaterialArr;
	typedef core::Array<MaterialLoadRequest*> MaterialLoadRequestArr;
	typedef core::Fifo<MaterialResource*> MaterialQueue;

public:
	XMaterialManager(core::MemoryArenaBase* arena, VariableStateManager& vsMan, CBufferManager& cBufMan);
	virtual ~XMaterialManager();

	void registerCmds(void);
	void registerVars(void);

	bool init(void);
	void shutDown(void);

	bool asyncInitFinalize(void);
	void dispatchPendingLoads(void);

	// IMaterialManager
	Material* findMaterial(const char* pMtlName) const X_FINAL;
	Material* loadMaterial(const char* pMtlName) X_FINAL;
	Material* getDefaultMaterial(void) const X_FINAL;
	
	// returns true if load succeed.
	bool waitForLoad(core::AssetBase* pMaterial) X_FINAL; 
	bool waitForLoad(Material* pMaterial) X_FINAL; 
	void releaseMaterial(Material* pMat);

	Material::Tech* getTechForMaterial(Material* pMat, core::StrHash hash, render::shader::VertexFormat::Enum vrtFmt, 
		PermatationFlags permFlags = PermatationFlags()) X_FINAL;
	bool setTextureID(Material* pMat, Material::Tech* pTech, core::StrHash texNameHash, texture::TexID id) X_FINAL;


	// ~IMaterialManager
	void listMaterials(const char* pSearchPatten = nullptr) const;

private:
	Material::Tech* getTechForMaterial_int(Material* pMat, core::StrHash hash, render::shader::VertexFormat::Enum vrtFmt,
		PermatationFlags permFlags);

private:
	bool initDefaults(void);
	void freeDanglingMaterials(void);
	void releaseResources(Material* pMat);

	void addLoadRequest(MaterialResource* pMaterial);
	void queueLoadRequest(MaterialResource* pMaterial, core::CriticalSection::ScopedLock&);
	void dispatchLoad(Material* pMaterial, core::CriticalSection::ScopedLock&);
	bool dispatchPendingLoad(core::CriticalSection::ScopedLock&);
	void dispatchLoadRequest(MaterialLoadRequest* pLoadReq);


	// load / processing
	void onLoadRequestFail(MaterialLoadRequest* pLoadReq);
	void loadRequestCleanup(MaterialLoadRequest* pLoadReq);

	void IoRequestCallback(core::IFileSys& fileSys, const core::IoRequestBase* pRequest,
		core::XFileAsync* pFile, uint32_t bytesTransferred);

	void ProcessData_job(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);

	bool processData(Material* pMaterial, core::XFile* pFile);


private:
	// ICoreEventListener
	virtual void OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam) X_FINAL;
	// ~ICoreEventListener

	// IXHotReload
	virtual void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_FINAL;
	// ~IXHotReload

private:
	void Cmd_ListMaterials(core::IConsoleCmdArgs* pCmd);


private:
	core::MemoryArenaBase* arena_;
	core::MemoryArenaBase* blockArena_;
	CBufferManager& cBufMan_;
	VariableStateManager& vsMan_;
	TechDefStateManager* pTechDefMan_;

	MaterialVars vars_;
	MaterialContainer materials_;

	Material* pDefaultMtl_;

	// loading
	core::CriticalSection loadReqLock_;
	core::ConditionVariable loadCond_;

	MaterialQueue requestQueue_;
	MaterialLoadRequestArr pendingRequests_;
	MaterialArr failedLoads_;
};


X_NAMESPACE_END

#include "MaterialManager.inl"

#endif // X_3D_MATERIAL_MAN_H_