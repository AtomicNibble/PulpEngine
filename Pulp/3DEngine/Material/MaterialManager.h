#pragma once

#ifndef X_3D_MATERIAL_MAN_H_
#define X_3D_MATERIAL_MAN_H_

#include <Containers\HashMap.h>
#include <String\StrRef.h>

#include "EngineBase.h"

#include <IDirectoryWatcher.h>

#include <../../tools/MaterialLib/MatLib.h>


X_NAMESPACE_BEGIN(engine)


class XMaterialManager : 
	public IMaterialManager, 
	public ICoreEventListener, 
	public XEngineBase,
	public core::IXHotReload
{
	template<typename T>
	struct ResourceRef : public T
	{
		const int32_t addRef(void)
		{
			++refCount_;
			return refCount_;
		}

		const int32_t removeRef(void) 
		{
			--refCount_;
			return refCount_;
		}

	private:
		core::AtomicInt refCount_;
	};

public:
	XMaterialManager();
	virtual ~XMaterialManager();

	bool Init(void);
	void ShutDown(void);

	// IMaterialManager
	virtual IMaterial* createMaterial(const char* pMtlName) X_OVERRIDE;
	virtual IMaterial* findMaterial(const char* pMtlName) const X_OVERRIDE;
	virtual IMaterial* loadMaterial(const char* pMtlName) X_OVERRIDE;

	virtual IMaterial* getDefaultMaterial() X_OVERRIDE;

	virtual void setListener(IMaterialManagerListener* pListner) X_OVERRIDE;
	// ~IMaterialManager

	// ICoreEventListener
	virtual void OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam) X_OVERRIDE;
	// ~ICoreEventListener

	// IXHotReload
	virtual void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_OVERRIDE;
	// ~IXHotReload

	void ListMaterials(const char* pSearchPatten = nullptr) const;

protected:
	friend class XMaterial;

	void unregister(IMaterial* pMat);

private:
	void InitDefaults(void);

private:
	typedef ResourceRef<engine::Material> MatResource;
	typedef core::HashMap<core::string, MatResource*> MaterialMap;


	MaterialMap	materials_;

	IMaterialManagerListener* pListner_;
	XMaterial*	pDefaultMtl_;
};


X_NAMESPACE_END


#endif // X_3D_MATERIAL_MAN_H_