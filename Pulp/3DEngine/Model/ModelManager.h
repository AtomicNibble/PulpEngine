#pragma once

#ifndef X_MODEL_MANAGER_H_
#define X_MODEL_MANAGER_H_

#include <IModel.h>

#include <Assets\AssertContainer.h>


X_NAMESPACE_DECLARE(core,
	struct IConsoleCmdArgs;
)

X_NAMESPACE_BEGIN(model)

class XModel;

class XModelManager :
	public core::IXHotReload
{
	typedef core::AssetContainer<XModel, MODEL_MAX_LOADED, core::SingleThreadPolicy> ModelContainer;
	typedef ModelContainer::Resource ModelResource;

public:
	XModelManager();
	~XModelManager();

	void registerCmds(void);
	void registerVars(void);

	bool init(void);
	void shutDown(void);

	bool asyncInitFinalize(void);

	XModel* findModel(const char* pModelName) const;
	XModel* loadModel(const char* pModelName);
	XModel* loadModelSync(const char* pModelName);
	XModel* getDefaultModel(void);

	void releaseModel(XModel* pModel);

	void reloadModel(const char* pName);
	void listModels(const char* pSearchPatten = nullptr) const;

private:
	bool initDefaults(void);
	void freeDanglingMaterials(void);

private:
	// IXHotReload
	void Job_OnFileChange(core::V2::JobSystem& jobSys, const core::Path<char>& name) X_OVERRIDE;
	// ~IXHotReload

private:
	void Cmd_ListModels(core::IConsoleCmdArgs* pCmd);
	void Cmd_ReloadModel(core::IConsoleCmdArgs* pCmd);

private:
	XModel*	pDefaultModel_;
	ModelContainer	models_;
};

X_NAMESPACE_END

#endif // !X_MODEL_MANAGER_H_