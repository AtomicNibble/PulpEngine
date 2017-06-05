#pragma once

#ifndef X_3D_MATERIAL_MAN_H_
#define X_3D_MATERIAL_MAN_H_

#include <String\StrRef.h>

#include <Assets\AssertContainer.h>

X_NAMESPACE_DECLARE(core, 
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

class XMaterialManager : 
	public IMaterialManager, 
	public ICoreEventListener, 
	public core::IXHotReload
{
	typedef core::AssetContainer<Material, MTL_MAX_LOADED, core::SingleThreadPolicy> MaterialContainer;
	typedef MaterialContainer::Resource MaterialResource;

public:
	XMaterialManager(core::MemoryArenaBase* arena, VariableStateManager& vsMan, CBufferManager& cBufMan);
	virtual ~XMaterialManager();

	void registerCmds(void);
	void registerVars(void);

	bool init(void);
	void shutDown(void);

	bool asyncInitFinalize(void);

	// IMaterialManager
	virtual Material* findMaterial(const char* pMtlName) const X_FINAL;
	virtual Material* loadMaterial(const char* pMtlName) X_FINAL;

	void releaseMaterial(Material* pMat);

	Material::Tech* getTechForMaterial(Material* pMat, core::StrHash hash, render::shader::VertexFormat::Enum vrtFmt, 
		PermatationFlags permFlags = PermatationFlags()) X_FINAL;
	bool setTextureID(Material* pMat, Material::Tech* pTech, core::StrHash texNameHash, texture::TexID id) X_FINAL;

	X_INLINE virtual Material* getDefaultMaterial(void) const X_FINAL;

	// ~IMaterialManager

	void listMaterials(const char* pSearchPatten = nullptr) const;

private:
	Material::Tech* getTechForMaterial_int(Material* pMat, core::StrHash hash, render::shader::VertexFormat::Enum vrtFmt,
		PermatationFlags permFlags);

	MaterialResource* loadMaterialCompiled(const core::string& name);
	MaterialResource* createMaterial_Internal(const core::string& name);
	MaterialResource* findMaterial_Internal(const core::string& name) const;
	void releaseResources(Material* pMat);


	bool initDefaults(void);
	void freeDanglingMaterials(void);

	// ICoreEventListener
	virtual void OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam) X_FINAL;
	// ~ICoreEventListener

	// IXHotReload
	virtual void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_FINAL;
	// ~IXHotReload

private:
	void Cmd_ListMaterials(core::IConsoleCmdArgs* Cmd);


private:
	core::MemoryArenaBase* arena_;
	CBufferManager& cBufMan_;
	VariableStateManager& vsMan_;
	TechDefStateManager* pTechDefMan_;

	MaterialContainer materials_;

	Material* pDefaultMtl_;
};


X_NAMESPACE_END

#include "MaterialManager.inl"

#endif // X_3D_MATERIAL_MAN_H_