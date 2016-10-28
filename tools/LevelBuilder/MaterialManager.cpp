#include "stdafx.h"
#include "MaterialManager.h"


X_NAMESPACE_BEGIN(lvl)


MatManager::MatManager(core::MemoryArenaBase* arena) :
	arena_(arena),
	materials_(arena, 2048),
	pDefaultMtl_(nullptr),
	pFileSys_(nullptr)
{
}

MatManager::~MatManager()
{

}

bool MatManager::Init(void)
{
	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pFileSys);

	pFileSys_ = gEnv->pFileSys;

	return loadDefaultMaterial();
}

void MatManager::ShutDown(void)
{
	// clear everything up.

	auto it = materials_.begin();
	for (; it != materials_.end();)
	{
		MatResource* pMatRes = it->second;

		++it;

		if (!pMatRes) {
			continue;
		}

		auto pMat = pMatRes->instance();

		// tell them off?
		X_WARNING("MtlMan", "Dangling material \"%s\" refs: %" PRIi32,
			pMat->getName().c_str(), pMatRes->getRefCount());

		X_DELETE(pMatRes, g_arena);
	}
}

bool MatManager::loadDefaultMaterial(void)
{
	core::string defaultName(engine::MTL_DEFAULT_NAME);

	pDefaultMtl_ = createMatResource(defaultName);
	if (!pDefaultMtl_) {
		X_ERROR("MtlMan", "Failed to create default material");
		return false;
	}

	if (!loadMatFromFile(*pDefaultMtl_, defaultName)) {
		X_ERROR("MtlMan", "Failed to load default material");
		return false;
	}

	return true;
}

engine::Material* MatManager::getDefaultMaterial(void) const
{
	return pDefaultMtl_->instance();
}

engine::Material* MatManager::loadMaterial(const char* pMtlName)
{
	X_ASSERT_NOT_NULL(pMtlName);

	core::string name(pMtlName);

	const char* pExt;
	if ((pExt = core::strUtil::FileExtension(name.begin(), name.end())))
	{
		// engine should not make requests for materials with a extension
		X_ERROR("MtlMan", "Invalid mtl name extension was included: %s", pExt);
		return nullptr;
	}

	// try find it.
	MatResource* pMatRes = findMatResource(name);
	if (pMatRes) {
		pMatRes->addReference();
		return pMatRes->instance();
	}

	// create a new material.
	pMatRes = createMatResource(name);

	// now we need to load it.
	if (loadMatFromFile(*pMatRes, name)) {
		return pMatRes->instance();
	}

	X_ERROR("MatMan", "Failed to load material: %s", pMtlName);

	// we want to give back the real instacne and not the default instance
	// so that when we hot reload stuff tha'ts default can get updated to real.
	pMatRes->instance()->assignProps(*pDefaultMtl_->instance());

	return pMatRes->instance();
}

void MatManager::releaseMaterial(engine::Material* pMat)
{
	X_ASSERT_NOT_NULL(pMat);

	MatResource* pMatRes = reinterpret_cast<MatResource*>(pMat);
	if (pMatRes->removeReference() == 0)
	{
		materials_.erase(pMat->getName());
	}
}


bool MatManager::loadMatFromFile(MatResource& mat, const core::string& name)
{
	core::XFileScoped file;
	core::Path<char> path;
	path /= "materials/";
	path.setFileName(name);
	path.setExtension(engine::MTL_B_FILE_EXTENSION);

	if (file.openFile(path.c_str(), core::fileMode::READ | core::fileMode::SHARE))
	{
		auto pMat = mat.instance();

		if (pMat->load(file.GetFile()))
		{
			return true;
		}
	}

	X_ERROR("MatMan", "Failed to load material \"%s\"", name.c_str());
	return false;
}

MatManager::MatResource* MatManager::createMatResource(const core::string& name)
{
	X_ASSERT(materials_.find(name) == materials_.end(), "mat already exists")();

	// create a new material.
	auto pMatRes = X_NEW(MatResource, arena_, "MaterialRes");
	materials_.insert(MaterialMap::value_type(name, pMatRes));

	// check we have correct default.
	X_ASSERT(pMatRes->getRefCount() == 1, "Invalid ref count")();

	pMatRes->instance()->setName(name.c_str());

	return pMatRes;
}


MatManager::MatResource* MatManager::findMatResource(const core::string& name)
{
	auto it = materials_.find(name);
	if (it != materials_.end()) {
		return it->second;
	}

	return nullptr;
}



X_NAMESPACE_END