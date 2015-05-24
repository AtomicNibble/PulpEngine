#pragma once


#ifndef X_LVL_BUILDER_MAT_MANAGER_H_
#define X_LVL_BUILDER_MAT_MANAGER_H_

#include "Assets\AssertContainer.h"


X_NAMESPACE_DECLARE(engine,
class XMaterial;
struct IMaterial;
)

X_NAMESPACE_BEGIN(lvl)

class MatManager
{
public:
	MatManager();
	~MatManager();

	void Init(void);
	void ShutDown(void);

	engine::IMaterial* createMaterial(const char* MtlName);
	engine::IMaterial* findMaterial(const char* MtlName) const;
	engine::IMaterial* loadMaterial(const char* MtlName);

private:
	friend class XMaterial;

	engine::IMaterial* loadMaterialXML(const char* MtlName);
	engine::IMaterial* loadMaterialCompiled(const char* MtlName);

private:
	typedef core::XResourceContainer MaterialCon;

	MaterialCon	materials_;
	
	core::IFileSys* pFileSys_;
};


X_NAMESPACE_END

#endif // !X_LVL_BUILDER_MAT_MANAGER_H_