#pragma once

X_NAMESPACE_BEGIN(model)

struct IModel;

struct IModelManager
{
	virtual ~IModelManager() {}

	// returns null if not found, ref count unaffected
	virtual IModel* findModel(const char* pModelName) const X_ABSTRACT;
	virtual IModel* loadModel(const char* pModelName) X_ABSTRACT;

	virtual IModel* getDefaultModel(void) const X_ABSTRACT;
};




X_NAMESPACE_END
