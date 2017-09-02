#pragma once

X_NAMESPACE_BEGIN(model)

class XModel;

struct IModelManager
{
	virtual ~IModelManager() {}

	// returns null if not found, ref count unaffected
	virtual XModel* findModel(const char* pModelName) const X_ABSTRACT;
	virtual XModel* loadModel(const char* pModelName) X_ABSTRACT;

	virtual XModel* getDefaultModel(void) const X_ABSTRACT;

	virtual bool waitForLoad(XModel* pModel) X_ABSTRACT; // returns true if load succeed.

};


X_NAMESPACE_END
