#pragma once

#ifndef X_MODEL_MANAGER_H_
#define X_MODEL_MANAGER_H_

#include "EngineBase.h"
#include <IModel.h>

#include <Assets\AssertContainer.h>

X_NAMESPACE_BEGIN(model)

class XModel;

class XModelManager :
	public IModelManager,
	public engine::XEngineBase,
	public core::IXHotReload
{
	typedef core::AssetContainer<XModel, MODEL_MAX_LOADED, core::SingleThreadPolicy> ModelContainer;
	typedef ModelContainer::Resource ModelResource;

public:
	XModelManager();
	virtual ~XModelManager() X_OVERRIDE;

	bool Init(void);
	void ShutDown(void);

	// IModelManager

	IModel* findModel(const char* ModelName) const X_FINAL;
	// this only performs cpu loading, and currently loads all lods at once.
	// as well as resolving materials, will alwyas return a instance, but might contain default data.
	IModel* loadModel(const char* ModelName) X_FINAL;
	IModel* loadModelSync(const char* pModelName) X_FINAL;

	void releaseModel(IModel* pModel) X_FINAL;

	IModel* getDefaultModel(void) X_FINAL;

	// ~IModelManager

	// IXHotReload
	void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_OVERRIDE;
	// ~IXHotReload

	void ListModels(const char* searchPatten = nullptr) const;
	void ReloadModel(const char* pName);

private:
	ModelResource* createModel(const char* pModelName);
	ModelResource* findModel_Internal(const core::string& name) const;


private:
	IModel*	pDefaultModel_;
	ModelContainer	models_;
};

X_NAMESPACE_END

#endif // !X_MODEL_MANAGER_H_