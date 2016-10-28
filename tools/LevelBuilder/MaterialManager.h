#pragma once


#ifndef X_LVL_BUILDER_MAT_MANAGER_H_
#define X_LVL_BUILDER_MAT_MANAGER_H_

#include <Util\ReferenceCounted.h>

#include <../../tools/MaterialLib/MatLib.h>


X_NAMESPACE_BEGIN(lvl)

class MatManager
{
	typedef core::ReferenceCountedInstance<engine::Material, core::AtomicInt> MatResource;
	typedef core::HashMap<core::string, MatResource*> MaterialMap;

public:
	MatManager(core::MemoryArenaBase* arena);
	~MatManager();

	bool Init(void);
	void ShutDown(void);

	engine::Material* loadMaterial(const char* MtlName);
	void releaseMaterial(engine::Material* pMat);

	engine::Material* getDefaultMaterial(void) const;

private:
	bool loadDefaultMaterial(void);

private:
	bool loadMatFromFile(MatResource& mat, const core::string& name);

	// only call if you know don't exsists in map.
	MatResource* createMatResource(const core::string& name);
	MatResource* findMatResource(const core::string& name);


private:
	core::MemoryArenaBase* arena_;
	MaterialMap	materials_;

	MatResource* pDefaultMtl_;
	core::IFileSys* pFileSys_;
};


X_NAMESPACE_END

#endif // !X_LVL_BUILDER_MAT_MANAGER_H_