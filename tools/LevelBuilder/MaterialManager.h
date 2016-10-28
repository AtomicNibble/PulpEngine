#pragma once


#ifndef X_LVL_BUILDER_MAT_MANAGER_H_
#define X_LVL_BUILDER_MAT_MANAGER_H_

#include "Assets\AssertContainer.h"


#include <../../tools/MaterialLib/MatLib.h>


X_NAMESPACE_BEGIN(lvl)

class MatManager
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

		const int32_t getRefCounter(void) const 
		{ 
			return refCount_;
		}


	private:
		core::AtomicInt refCount_;
	};

	typedef ResourceRef<engine::Material> MatResource;
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