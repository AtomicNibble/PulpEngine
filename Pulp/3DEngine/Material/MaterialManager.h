#pragma once

#ifndef X_3D_MATERIAL_MAN_H_
#define X_3D_MATERIAL_MAN_H_

#include <String\StrRef.h>
#include <IDirectoryWatcher.h>

#include "EngineBase.h"

#include <../../tools/MaterialLib/MatLib.h>

#include <Assets\AssertContainer.h>

X_NAMESPACE_BEGIN(engine)

class TechSetDefs;
class VariableStateManager;
class TechDefStateManager;
class CBufferManager;
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
	XMaterialManager(core::MemoryArenaBase* arena, VariableStateManager& vsMan);
	virtual ~XMaterialManager();

	bool Init(void);
	void ShutDown(void);

	// IMaterialManager
	virtual Material* createMaterial(const char* pMtlName) X_FINAL;
	virtual Material* findMaterial(const char* pMtlName) const X_FINAL;
	virtual Material* loadMaterial(const char* pMtlName) X_FINAL;

	void releaseMaterial(Material* pMat);

	Material::Tech* getTechForMaterial(Material* pMat, core::StrHash hash, render::shader::VertexFormat::Enum vrtFmt) X_FINAL;
	bool setTextureID(Material* pMat, Material::Tech* pTech, core::StrHash texNameHash, texture::TexID id) X_FINAL;

	X_INLINE virtual Material* getDefaultMaterial(void) const X_FINAL;

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
	void freeDanglingMaterials(void);

private:
	core::MemoryArenaBase* arena_;
	VariableStateManager& vsMan_;
	TechDefStateManager* pTechDefMan_;

	MaterialContainer materials_;

	Material* pDefaultMtl_;
};


X_NAMESPACE_END

#include "MaterialManager.inl"

#endif // X_3D_MATERIAL_MAN_H_