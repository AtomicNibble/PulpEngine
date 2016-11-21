#pragma once

#ifndef X_3D_MATERIAL_MAN_H_
#define X_3D_MATERIAL_MAN_H_

#include <String\StrRef.h>
#include <IDirectoryWatcher.h>

#include "EngineBase.h"

#include <../../tools/MaterialLib/MatLib.h>

#include <Assets\AssertContainer.h>

X_NAMESPACE_BEGIN(engine)

class VariableStateManager;

class XMaterial;

class XMaterialManager : 
	public IMaterialManager, 
	public ICoreEventListener, 
	public XEngineBase,
	public core::IXHotReload
{
	typedef core::AssetContainer<XMaterial, MTL_MAX_LOADED, core::SingleThreadPolicy> MaterialContainer;
	typedef MaterialContainer::Resource MaterialResource;

public:
	XMaterialManager(VariableStateManager& vsMan);
	virtual ~XMaterialManager();

	bool Init(void);
	void ShutDown(void);

	// IMaterialManager
	virtual XMaterial* createMaterial(const char* pMtlName) X_OVERRIDE;
	virtual XMaterial* findMaterial(const char* pMtlName) const X_OVERRIDE;
	virtual XMaterial* loadMaterial(const char* pMtlName) X_OVERRIDE;

	void releaseMaterial(XMaterial* pMat);

	virtual XMaterial* getDefaultMaterial(void) X_OVERRIDE;

	virtual void setListener(IMaterialManagerListener* pListner) X_OVERRIDE;
	// ~IMaterialManager

	// ICoreEventListener
	virtual void OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam) X_OVERRIDE;
	// ~ICoreEventListener

	// IXHotReload
	virtual void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_OVERRIDE;
	// ~IXHotReload

	void ListMaterials(const char* pSearchPatten = nullptr) const;

private:
	MaterialResource* createMaterial_Internal(const char* pModelName);
	MaterialResource* findMaterial_Internal(const core::string& name) const;

	void InitDefaults(void);

private:
	// typedef core::ReferenceCountedInstance<engine::Material, core::AtomicInt> MatResource;
	// typedef core::HashMap<core::string, MatResource*> MaterialMap;
	VariableStateManager& vsMan_;

	MaterialContainer materials_;

	IMaterialManagerListener* pListner_;
	XMaterial* pDefaultMtl_;
};


X_NAMESPACE_END


#endif // X_3D_MATERIAL_MAN_H_