#pragma once

#ifndef X_MODEL_MANAGER_H_
#define X_MODEL_MANAGER_H_

#include "EngineBase.h"
#include <IModel.h>

X_NAMESPACE_BEGIN(model)

class XModelManager :
	public IModelManager,
	public engine::XEngineBase,
	public core::IXHotReload
{
public:
	XModelManager();
	virtual ~XModelManager() X_OVERRIDE;

	void Init(void);
	void ShutDown(void);

	// IModelManager

	IModel* findModel(const char* ModelName) const X_FINAL;
	// if material is found adds ref and returns, if not try's to load the material file.
	// if file can't be loaded or error it return the default material.
	IModel* loadModel(const char* ModelName) X_FINAL;

	IModel* getDefaultModel(void) X_FINAL;

	// ~IModelManager

	// IXHotReload
	virtual void OnFileChange(const core::Path<char>& name) X_OVERRIDE;
	// ~IXHotReload

	void ListModels(const char* searchPatten = nullptr) const;
	void ReloadModel(const char* pName);
private:
	IModel* createModel(const char* ModelName);
	IModel* loadModelSync(const char* ModelName);

	IModel* findModel_Internal(const char* ModelName) const;

	IModel* LoadCompiledModel(const char* ModelName);

private:
	typedef core::XResourceContainer ModelCon;

	IModel*		pDefaultModel_;
	ModelCon	models_;
};

X_NAMESPACE_END

#endif // !X_MODEL_MANAGER_H_