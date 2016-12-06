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

class Material;

class XMaterialManager : 
	public IMaterialManager, 
	public ICoreEventListener, 
	public XEngineBase,
	public core::IXHotReload
{
	typedef core::AssetContainer<Material, MTL_MAX_LOADED, core::SingleThreadPolicy> MaterialContainer;
	typedef MaterialContainer::Resource MaterialResource;

public:
	XMaterialManager(VariableStateManager& vsMan);
	virtual ~XMaterialManager();

	bool Init(void);
	void ShutDown(void);

	// IMaterialManager
	virtual Material* createMaterial(const char* pMtlName) X_OVERRIDE;
	virtual Material* findMaterial(const char* pMtlName) const X_OVERRIDE;
	virtual Material* loadMaterial(const char* pMtlName) X_OVERRIDE;

	void releaseMaterial(Material* pMat);

	virtual Material* getDefaultMaterial(void) X_OVERRIDE;

	// ~IMaterialManager

	// ICoreEventListener
	virtual void OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam) X_OVERRIDE;
	// ~ICoreEventListener

	// IXHotReload
	virtual void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_OVERRIDE;
	// ~IXHotReload

	void ListMaterials(const char* pSearchPatten = nullptr) const;

private:
	MaterialResource* loadMaterialCompiled(const core::string& name);

	MaterialResource* createMaterial_Internal(const core::string& name);
	MaterialResource* findMaterial_Internal(const core::string& name) const;

	void InitDefaults(void);

private:
	VariableStateManager& vsMan_;

	MaterialContainer materials_;

	Material* pDefaultMtl_;
};


X_NAMESPACE_END


#endif // X_3D_MATERIAL_MAN_H_