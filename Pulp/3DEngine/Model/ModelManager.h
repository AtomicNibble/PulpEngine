#pragma once

#ifndef X_MODEL_MANAGER_H_
#define X_MODEL_MANAGER_H_

#include "EngineBase.h"
#include <IModel.h>

#include <Assets\AssertContainer.h>


X_NAMESPACE_DECLARE(core,
	struct IConsoleCmdArgs;
)

X_NAMESPACE_BEGIN(model)

class XModel;

class XModelManager :
	public engine::XEngineBase,
	public core::IXHotReload
{
	typedef core::AssetContainer<XModel, MODEL_MAX_LOADED, core::SingleThreadPolicy> ModelContainer;
	typedef ModelContainer::Resource ModelResource;

public:
	XModelManager();
	~XModelManager();

	bool Init(void);
	void ShutDown(void);

	// IModelManager

	XModel* findModel(const char* ModelName) const;
	// this only performs cpu loading, and currently loads all lods at once.
	// as well as resolving materials, will alwyas return a instance, but might contain default data.
	XModel* loadModel(const char* ModelName);
	XModel* loadModelSync(const char* pModelName);

	void releaseModel(XModel* pModel);

	XModel* getDefaultModel(void);

	// ~IModelManager

	// IXHotReload
	void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_OVERRIDE;
	// ~IXHotReload

	void ListModels(const char* searchPatten = nullptr) const;
	void ReloadModel(const char* pName);

private:

	void Cmd_ListModels(core::IConsoleCmdArgs* Cmd);
	void Cmd_ReloadModel(core::IConsoleCmdArgs* pCmd);

private:
	XModel*	pDefaultModel_;
	ModelContainer	models_;
};

X_NAMESPACE_END

#endif // !X_MODEL_MANAGER_H_