#include "stdafx.h"
#include "MaterialManager.h"

#include "../3DEngine/Material.h"

X_NAMESPACE_BEGIN(lvl)


MatManager::MatManager() :
materials_(g_arena, 2048)
{

}

MatManager::~MatManager()
{

}

void MatManager::Init(void)
{
	X_ASSERT_NOT_NULL(gEnv);


}

void MatManager::ShutDown(void)
{

}

engine::IMaterial* MatManager::findMaterial(const char* MtlName) const
{
	X_ASSERT_NOT_NULL(MtlName);

	engine::XMaterial* pMaterial = reinterpret_cast<engine::XMaterial*>(materials_.findAsset(MtlName));
	if (pMaterial)
		return pMaterial;

	return nullptr;
}

engine::IMaterial* MatManager::loadMaterial(const char* MtlName)
{
	X_ASSERT_NOT_NULL(MtlName);
	const char* pExt;
	engine::IMaterial* iMat;

	if ((pExt = core::strUtil::FileExtension(MtlName)))
	{
		// engine should not make requests for materials with a extensiob
		X_ERROR("MtlMan", "Invalid mtl name extension was included: %s",
			pExt);
		return nullptr;
	}

	// try find it.
	if (iMat = findMaterial(MtlName)){
		// inc ref count.
		((engine::XMaterial*)iMat)->addRef();
		return iMat;
	}

	// lets look for binary first.
	if ((iMat = loadMaterialCompiled(MtlName))) {
		return iMat;
	}

	if ((iMat = loadMaterialXML(MtlName))) {
		return iMat;
	}

	X_ERROR("MatMan", "Failed to find material: %s", MtlName);
	return nullptr;
}

// loaders.

engine::IMaterial* MatManager::loadMaterialXML(const char* MtlName)
{
	X_ASSERT_NOT_NULL(MtlName);
	
	return nullptr;
}

engine::IMaterial* MatManager::loadMaterialCompiled(const char* MtlName)
{
	X_ASSERT_NOT_NULL(MtlName);

	return nullptr;
}

X_NAMESPACE_END