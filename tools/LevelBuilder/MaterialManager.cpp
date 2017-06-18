#include "stdafx.h"
#include "MaterialManager.h"


X_NAMESPACE_BEGIN(lvl)


MatManager::MatManager(core::MemoryArenaBase* arena) :
	arena_(arena),
	materials_(arena, sizeof(MaterialResource), core::Max<size_t>(8u, X_ALIGN_OF(MaterialResource))),
	nameOverRide_(arena, 64),
	pDefaultMtl_(nullptr)
{
}

MatManager::~MatManager()
{

}

bool MatManager::Init(void)
{
	X_ASSERT_NOT_NULL(gEnv);


	nameOverRide_.insert(std::make_pair(core::string("portal"), core::string("tool/editor/portal")));
	nameOverRide_.insert(std::make_pair(core::string("portal_nodraw"), core::string("tool/editor/portal_nodraw")));


	return loadDefaultMaterial();
}

void MatManager::ShutDown(void)
{
	// clear everything up.
	releaseMaterial(pDefaultMtl_);

	freeDanglingMaterials();
}

bool MatManager::loadDefaultMaterial(void)
{
	if (!pDefaultMtl_)
	{
		pDefaultMtl_ = createMaterial_Internal(core::string(engine::MTL_DEFAULT_NAME));

		// it's default :|
		pDefaultMtl_->setFlags(engine::MaterialFlag::DEFAULT | engine::MaterialFlag::STRUCTURAL);
		pDefaultMtl_->setCoverage(engine::MaterialCoverage::OPAQUE);
	}

	return true;
}

void MatManager::freeDanglingMaterials(void)
{
	auto it = materials_.begin();
	for (; it != materials_.end(); ++it) {
		auto matRes = it->second;
		X_WARNING("MtlMan", "\"%s\" was not deleted. refs: %" PRIi32, matRes->getName(), matRes->getRefCount());
	}

	materials_.free();
}

engine::Material* MatManager::getDefaultMaterial(void) const
{
	return pDefaultMtl_->instance();
}

engine::Material* MatManager::loadMaterial(const char* pMtlName)
{
	X_ASSERT_NOT_NULL(pMtlName);

	core::string name(pMtlName);

	auto it = nameOverRide_.find(name);
	if (it != nameOverRide_.end())
	{
		name = it->second;
	}

#if X_DEBUG
	const char* pExt;
	if ((pExt = core::strUtil::FileExtension(name.begin(), name.end())))
	{
		// engine should not make requests for materials with a extension
		X_ERROR("MtlMan", "Invalid mtl name extension was included: %s", pExt);
		return nullptr;
	}
#endif // X_DEBUG

	// try find it.
	MaterialResource* pMatRes = findMaterial_Internal(name);
	if (pMatRes) {
		pMatRes->addReference();
		return pMatRes;
	}

	// create a new material.
	pMatRes = createMaterial_Internal(name);

	// now we need to load it.
	if (loadMatFromFile(*pMatRes, name)) {
		return pMatRes;
	}

	X_ERROR("MatMan", "Failed to load material: %s", pMtlName);

	// we want to give back the real instacne and not the default instance
	// so that when we hot reload stuff tha'ts default can get updated to real.
	pMatRes->assignProps(*pDefaultMtl_);

	return pMatRes->instance();
}

void MatManager::releaseMaterial(engine::Material* pMat)
{
	X_ASSERT_NOT_NULL(pMat);

	MaterialResource* pMatRes = reinterpret_cast<MaterialResource*>(pMat);
	if (pMatRes->removeReference() == 0)
	{
		materials_.releaseAsset(pMatRes);
	}
}


bool MatManager::loadMatFromFile(MaterialResource& mat, const core::string& name)
{
	core::XFileScoped file;
	core::Path<char> path;
	path = "materials/";
	path.setFileName(name);
	path.setExtension(engine::MTL_B_FILE_EXTENSION);

	if (!file.openFile(path.c_str(), core::fileMode::READ | core::fileMode::SHARE)) {
		return false;
	}

	engine::MaterialHeader hdr;


	if (file.readObj(hdr) != sizeof(hdr)) {
		return false;
	}

	if (!hdr.isValid()) {
		return false;
	}

	if (hdr.numSamplers > engine::MTL_MAX_SAMPLERS) {
		return false;
	}

	// i might move this into material lib dunno yet.
	// not sure how simular this and runtime material loader will be.
	// still ironing out the format.
	mat.assignProps(hdr);

	mat.setStatus(core::LoadStatus::Complete);
	return true;
}


MatManager::MaterialResource* MatManager::createMaterial_Internal(core::string& name)
{
	// internal create expects you to know no duplicates
	X_ASSERT(findMaterial_Internal(name) == nullptr, "Creating a material that already exsists")();

	auto pMatRes = materials_.createAsset(name, name, arena_);

	return pMatRes;
}

MatManager::MaterialResource* MatManager::findMaterial_Internal(const core::string& name) const
{
	return materials_.findAsset(name);
}




X_NAMESPACE_END