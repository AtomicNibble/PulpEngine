#pragma once

#ifndef X_3D_MATERIAL_MAN_H_
#define X_3D_MATERIAL_MAN_H_

#include <Containers\HashMap.h>
#include <String\StrRef.h>
// #include <IMaterial.h>

#include "EngineBase.h"

#include <IDirectoryWatcher.h>

struct IMaterial;

X_NAMESPACE_DECLARE(engine,
class XMaterial;
)

X_NAMESPACE_BEGIN(engine)


class XMaterialManager : 
	public IMaterialManager, 
	public ICoreEventListener, 
	public XEngineBase,
	public core::IXHotReload
{
public:
	XMaterialManager();
	virtual ~XMaterialManager();

	bool Init(void);
	void ShutDown(void);

	// IMaterialManager
	virtual IMaterial* createMaterial(const char* MtlName) X_OVERRIDE;
	virtual IMaterial* findMaterial(const char* MtlName) const X_OVERRIDE;
	virtual IMaterial* loadMaterial(const char* MtlName) X_OVERRIDE;

	virtual IMaterial* getDefaultMaterial() X_OVERRIDE;

	virtual void setListener(IMaterialManagerListener* pListner) X_OVERRIDE;
	// ~IMaterialManager

	// ICoreEventListener
	virtual void OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam) X_OVERRIDE;
	// ~ICoreEventListener

	// IXHotReload
	virtual void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_OVERRIDE;
	// ~IXHotReload

	void ListMaterials(const char* searchPatten = nullptr) const;

protected:
	friend class XMaterial;

	void unregister(IMaterial* pMat);

private:
	IMaterial* loadMaterialCompiled(const char* MtlName);

	bool saveMaterialCompiled(IMaterial* pMat);
	
	void InitDefaults(void);

private:
	typedef core::XResourceContainer MaterialCon;

	MaterialCon	materials_;

	IMaterialManagerListener* pListner_;
	XMaterial*	pDefaultMtl_;
};


X_NAMESPACE_END


#endif // X_3D_MATERIAL_MAN_H_